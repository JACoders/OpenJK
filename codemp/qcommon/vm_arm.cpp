#include <sys/mman.h>
#include "vm_local.h"

// ==============================================
// r3       = instructionPointers
// r4       = instructionCount
// r5       = dataBase
// r6       = programStack pointer
// r7       = opStack
// r8       = dataMask
// r9       = codeBase
// r10      = opStackIndex
//
// vm->code:
// 0:  VM_CompiledSyscall
// 4:  VM_ARM_VOp
// 8:  VM_CompiledErrorJump
// 12: code
// ==============================================

#define MAX_CONSTANTS_POOL_SIZE 32

typedef enum {
    R0  =  0 << 12,
    R1  =  1 << 12,
    R2  =  2 << 12,
    R3  =  3 << 12,
    R4  =  4 << 12,
    R5  =  5 << 12,
    R6  =  6 << 12,
    R7  =  7 << 12,
    R8  =  8 << 12,
    R9  =  9 << 12,
    R10 = 10 << 12
} r_t;

typedef enum {
    S0, S1
} s_t;

typedef struct {
    qboolean movwt;
    qboolean idiva;
    qboolean mls;
    qboolean vfp;
} cpuFeatures_t;

cpuFeatures_t cpu;
byte *nativeCode;
size_t nativeCodeSize, nativeCodeLength;

uint32_t localConstantPool[MAX_CONSTANTS_POOL_SIZE]; size_t localConstantPoolLength;
byte **localConstantPoolPointers; size_t localConstantCurrentPool;
byte *firstLocalConstantPtr;

#define Emit32(X) EmitInteger(0x##X)
void EmitInteger(uint32_t num) {
	if (nativeCodeLength + sizeof(uint32_t) >= nativeCodeSize) {
        Com_Error(ERR_DROP, "native code too big for buffer");
    }

    *(uint32_t *)(&nativeCode[nativeCodeLength]) = num;
    nativeCodeLength += sizeof(uint32_t);
}

int CalculateConstant12(uint32_t *num) {
    uint32_t rot;
    uint32_t immediate = *num;

    for (rot = 0; rot < 32; rot += 2) {
        if (!(immediate & ~0xFFU)) {
            *num = 0;
            *num = immediate;
            *num |= (rot / 2) << 8;
            return 0;
        }

        immediate = (immediate << 2) | (immediate >> 30);
    }

    return 1;
}

void EmitLocalConstantPool(qboolean endOfProcedure) {
    if (localConstantPoolLength) {
        if (!endOfProcedure) {
            byte *sourceAddr = &nativeCode[nativeCodeLength];
            byte *destAddr = sourceAddr + sizeof(uint32_t) + localConstantPoolLength * sizeof(uint32_t);

            uint32_t jmpdiff;
            if (destAddr >= sourceAddr + 8) {
                jmpdiff = (destAddr - (sourceAddr + 8)) / 4;
            } else {
                jmpdiff = 0x00FFFFFFU - (sourceAddr + 4 - destAddr) / 4;
            }

            if (jmpdiff & 0xFF000000) {
                Com_Error(ERR_DROP, "jump target out of range");
            }

            EmitInteger(0xEA000000|jmpdiff);
        }

        localConstantPoolPointers[localConstantCurrentPool++] = &nativeCode[nativeCodeLength];
        for (size_t i = 0; i < localConstantPoolLength; i++) {
            EmitInteger(localConstantPool[i]);
        }

        localConstantPoolLength = 0;
    }
}

void EmitTOS2Reg(r_t r, qboolean pop) {
    EmitInteger(0xE797010A | (uint32_t)r); // ldr rX, [r7, r10, LSL #2]

    if (pop) {
        Emit32(E24AA001); // sub r10, #1
        Emit32(E20AA0FF); // and r10, #255
    }
}

void EmitTOS2SReg(s_t s, qboolean pop) {
    Emit32(E087010A); // add r0, r7, r10, LSL #2
    switch (s) {
        case S0: Emit32(ED900A00); break; // flds s0, [r0]
        case S1: Emit32(EDD00A00); break; // flds s1, [r0]
    }

    if (pop) {
        Emit32(E24AA001); // sub r10, #1
        Emit32(E20AA0FF); // and r10, #255
    }
}

void EmitReg2TOS(r_t r, qboolean push) {
    if (push) {
        Emit32(E28AA001); // add r10, #1
        Emit32(E20AA0FF); // and r10, #255
    }

    EmitInteger(0xE787010A | (uint32_t)r); // str rX, [r7, r10, LSL #2]
}

void EmitSReg2TOS(s_t s, qboolean push) {
    if (push) {
        Emit32(E28AA001); // add r10, #1
        Emit32(E20AA0FF); // and r10, #255
    }

    Emit32(E087010A); // add r0, r7, r10, LSL #2
    switch (s) {
        case S0: Emit32(ED800A00); break; // fsts s0, [r0]
        case S1: Emit32(EDC00A00); break; // fsts s1, [r0]
    }
}

void EmitRegisterLoadConst(r_t r, uint32_t constant) {
    uint32_t immediate = constant;
    if ( !CalculateConstant12(&immediate) ) {
        uint32_t opcode = 0xE3A00000; // mov rX, #immediate
        opcode |= (uint32_t)r;
        opcode |= immediate;

        EmitInteger(opcode);
        return;
    }

    immediate = ~immediate;
    if ( !CalculateConstant12(&immediate) ) {
        uint32_t opcode = 0xE3E00000; // movn rX, #immediate
        opcode |= (uint32_t)r;
        opcode |= immediate;

        EmitInteger(opcode);
        return;
    }

    // at this point the constant needs to be loaded either by movw and movt (which is >= ARMv7)
    // or by loading it from a memory address near to the current pc
    if (cpu.movwt) {
        uint32_t w = (constant & 0x00000FFF) | ((constant & 0x0000F000) << 4);
        EmitInteger(0xE3000000 | (uint32_t)r | w); // movw rX, #16bit immediate

        uint32_t t = (constant >> 16);
        t = (t & 0x00000FFF) | ((t & 0x0000F000) << 4);
        EmitInteger(0xE3400000 | (uint32_t)r | t); // movt rX, #16bit immediate
    } else {
        if (localConstantPoolLength >= MAX_CONSTANTS_POOL_SIZE) {
            EmitLocalConstantPool(qfalse);
        }

        if (!firstLocalConstantPtr) firstLocalConstantPtr = &nativeCode[nativeCodeLength];
        if (localConstantPoolPointers[localConstantCurrentPool]) {
            byte *constantPtr = localConstantPoolPointers[localConstantCurrentPool] + localConstantPoolLength * sizeof(uint32_t);
            intptr_t ptrdiff = -8 + constantPtr - &nativeCode[nativeCodeLength];

            uint32_t opcode = 0;
            if (ptrdiff <= 0) {
                opcode |= 0xE51F0000; // ldr rX, [pc, #-X]
                ptrdiff = -ptrdiff;
            } else {
                opcode |= 0xE59F0000; // ldr rX, [pc, #+X]
            }

            if (ptrdiff > 4095) {
                // too far away. should never happen.
                Com_Error(ERR_DROP, "constant relative address out of range.");
            }

            opcode |= (uint32_t)r;
            opcode |= (uint32_t)ptrdiff;
            EmitInteger(opcode);
        } else {
            // address is still unknown (first pass)
            Emit32(00000000);
        }

        localConstantPool[localConstantPoolLength++] = constant;
    }
}

void EmitConstantJumpInstruction(vm_t *vm, opcode_t opCode, uint32_t destinationOpCodeNum) {
    size_t sourceAddr = nativeCodeLength;

    if (destinationOpCodeNum >= (uint32_t)vm->instructionCount) {
        Com_Error(ERR_DROP, "jump violation detected.");
    }

    size_t destAddr = vm->instructionPointers[destinationOpCodeNum];
    if (!destAddr) {
        // address is still unknown (first pass)
        Emit32(00000000);
        return;
    }

    uint32_t jmpdiff;
    if (destAddr >= sourceAddr + 8) {
        jmpdiff = (destAddr - (sourceAddr + 8)) / 4;
    } else {
        jmpdiff = 0x00FFFFFFU - (sourceAddr + 4 - destAddr) / 4;
    }

    if (jmpdiff & 0xFF000000) {
        Com_Error(ERR_DROP, "jump target out of range.");
    }

    switch (opCode) {
        case OP_EQ:  case OP_EQF: EmitInteger(0x0A000000|jmpdiff); break; // (BEQ) Check equality (integer or float) (compares NIS vs TOS, jump to $PARM if true).
        case OP_NE:  case OP_NEF: EmitInteger(0x1A000000|jmpdiff); break; // (BNE) Check inequality (integer or float) (NIS vs TOS, jump to $PARM if true).
        case OP_LTI: case OP_LTF: EmitInteger(0xBA000000|jmpdiff); break; // (BLT) Check less-than (signed integer or float) (NIS vs TOS, jump to $PARM if true).
        case OP_LEI: case OP_LEF: EmitInteger(0xDA000000|jmpdiff); break; // (BLE) Check less-than or equal-to (signed integer) (NIS vs TOS, jump to $PARM if true).
        case OP_GTI:              EmitInteger(0xCA000000|jmpdiff); break; // (BGT) Check greater-than (signed integer) (NIS vs TOS), jump to $PARM if true.
        case OP_GEI:              EmitInteger(0xAA000000|jmpdiff); break; // (BGE) Check greater-than or equal-to (signed integer) (NIS vs TOS), jump to $PARM if true.
        case OP_LTU:              EmitInteger(0x3A000000|jmpdiff); break; // (BCC) Check less-than (unsigned integer) (NIS vs TOS), jump to $PARM if true.
        case OP_LEU:              EmitInteger(0x9A000000|jmpdiff); break; // (BLS) Check less-than or equal-to (unsigned integer) (NIS vs TOS), jump to $PARM if true.
        case OP_GTU: case OP_GTF: EmitInteger(0x8A000000|jmpdiff); break; // (BHI) Check greater-than (unsigned integer or float) (NIS vs TOS), jump to $PARM if true.
        case OP_GEU: case OP_GEF: EmitInteger(0x2A000000|jmpdiff); break; // (BCS) Check greater-than or equal-to (unsigned integer or float) (NIS vs TOS), jump to $PARM if true.
        default: Com_Error(ERR_DROP, "OpCode implementation missing for %u", opCode);
    }
}

void __attribute__((optimize("O0"))) VM_ARM_VOp() {
    int32_t nis, tos, op;
    uint32_t parm;

    __asm__ volatile(
        "str r0, %[nis]\n"
        "str r1, %[tos]\n"
        "str r2, %[op]\n"
        "str r3, %[parm]\n"
         : [nis] "=m" (nis),
           [tos] "=m" (tos),
           [op] "=m" (op),
           [parm] "=m" (parm)
        :: "r0", "r1", "r2", "r3"
    );

    switch (op) {
        case OP_BLOCK_COPY:
            VM_BlockCopy(nis, tos, parm);
            return;
        case OP_DIVI:
            tos = nis / tos;
            break;
        case OP_DIVU:
            tos = (uint32_t)nis / (uint32_t)tos;
            break;
        case OP_MODI:
            tos = nis % tos;
            break;
        case OP_MODU:
            tos = (uint32_t)nis % (uint32_t)tos;
            break;

        case OP_EQF:
            tos = *(float *)&nis == *(float *)&tos;
            break;
        case OP_NEF:
            tos = *(float *)&nis != *(float *)&tos;
            break;
        case OP_LTF:
            tos = *(float *)&nis < *(float *)&tos;
            break;
        case OP_LEF:
            tos = *(float *)&nis <= *(float *)&tos;
            break;
        case OP_GTF:
            tos = *(float *)&nis > *(float *)&tos;
            break;
        case OP_GEF:
            tos = *(float *)&nis >= *(float *)&tos;
            break;

        case OP_NEGF:
            *(float *)&tos = -*(float *)&tos;
            break;
        case OP_ADDF:
            *(float *)&tos = *(float *)&nis + *(float *)&tos;
            break;
        case OP_SUBF:
            *(float *)&tos = *(float *)&nis - *(float *)&tos;
            break;
        case OP_DIVF:
            *(float *)&tos = *(float *)&nis / *(float *)&tos;
            break;
        case OP_MULF:
            *(float *)&tos = *(float *)&nis * *(float *)&tos;
            break;
        case OP_CVIF:
            *(float *)&tos = tos;
            break;
        case OP_CVFI:
            tos = *(float *)&tos;
            break;
    }

    __asm__ volatile(
        "ldr r0, %[tos]\n"
        :: [tos] "m" (tos)
         : "r0"
    );
}

void __attribute__((optimize("O0"))) VM_CompiledSyscall() {
    int *opStack, *sp;
    int nic, opStackIndex;
    vm_t *savedvm;

    __asm__ volatile(
        "str r0, %[nic]\n"
        "str r6, %[programStackPtr]\n"
        "str r7, %[opStack]\n"
        "str r10, %[opStackIndex]\n"
         : [nic] "=m" (nic),
           [programStackPtr] "=m" (sp),
           [opStack] "=m" (opStack),
           [opStackIndex] "=m" (opStackIndex)
        :: "r0", "r6", "r7", "r10"
    );

    currentVM->programStack = ((byte *)sp - currentVM->dataBase);

    intptr_t args[MAX_VMSYSCALL_ARGS];
    args[0] = ~nic;
    int index;
    for(index = 1; index < MAX_VMSYSCALL_ARGS; index++)
        args[index] = sp[1 + index];

    savedvm = currentVM;
    opStack += opStackIndex;
    *opStack = currentVM->legacy.syscall(args);
    currentVM = savedvm;
}

void VM_CompiledErrorJump() {
    Com_Error(ERR_DROP, "program tried to execute code outside VM");
}

void VM_Compile(vm_t *vm, vmHeader_t *header) {
    size_t i, pass;
	int ic;

#ifdef __linux__
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");

    char *arg = 0; size_t size = 0;
    int extnum = 0; char extstr[128];
    extstr[0] = '\0';
    while (getline(&arg, &size, cpuinfo) != -1) {
        if (!strncmp(arg, "model name", 10)) {
            if (strstr(arg, "ARMv7") && !cpu.movwt && !cpu.mls) {
                cpu.movwt = cpu.mls = qtrue;
                Q_strcat(extstr, sizeof(extstr), " movw/movt mls"); extnum++;
            }
        }

       if (!strncmp(arg, "Features", 8)) {
            if (strstr(arg, " idiva") && !cpu.idiva) {
                cpu.idiva = qtrue;
                Q_strcat(extstr, sizeof(extstr), " idiva"); extnum++;
            }

            if (strstr(arg, " vfp") && !cpu.vfp) {
                cpu.vfp = qtrue;
                Q_strcat(extstr, sizeof(extstr), " vfp"); extnum++;
            }
       }
    }

    if (extnum) {
        Com_Printf("using instruction set extensions:%s\n", extstr);
    }

    free(arg); fclose(cpuinfo);
#else
    // TODO
#endif

    nativeCodeSize = (size_t)vm->codeLength * 10;
    nativeCode = (byte *)Z_Malloc(nativeCodeSize, TAG_VM, qfalse);
    localConstantPoolPointers = (byte **)Z_Malloc(vm->instructionCount * sizeof(byte *), TAG_VM, qtrue);

    for (pass = 0; pass < 2; pass++) {
        nativeCodeLength = 0;
        localConstantCurrentPool = 0;
        localConstantPoolLength = 0;

        // Function Pointers
        EmitInteger((uint32_t)VM_CompiledSyscall);
        EmitInteger((uint32_t)VM_ARM_VOp);
        EmitInteger((uint32_t)VM_CompiledErrorJump);

        // Init registers code
        EmitRegisterLoadConst(R3, (uint32_t)vm->instructionPointers);
        EmitRegisterLoadConst(R4, vm->instructionCount);
        EmitRegisterLoadConst(R5, (uint32_t)vm->dataBase);
        EmitRegisterLoadConst(R8, (uint32_t)vm->dataMask);

        for (i = 0, ic = 0; ic < vm->instructionCount; i++, ic++) {
            byte *instructionptr = (byte *)header + header->codeOffset + i;
            opcode_t currOp = (opcode_t)*instructionptr;
            byte *arg = instructionptr + 1;

            if (localConstantPoolLength) {
                ptrdiff_t constoffsetlen = ((ptrdiff_t)-8 + &nativeCode[nativeCodeLength] + sizeof(uint32_t) + localConstantPoolLength * sizeof(uint32_t)) - firstLocalConstantPtr;
                if (constoffsetlen >= (ptrdiff_t)(4095 - 15 * sizeof(uint32_t))) {
                    EmitLocalConstantPool(qfalse);
                }
            }

            vm->instructionPointers[ic] = nativeCodeLength;

            switch (currOp) {
            /* ---------------------------------------------------------------------------- */
            /* ---------------------------------- BASICS ---------------------------------- */
            /* ---------------------------------------------------------------------------- */
            case OP_IGNORE: {
                Emit32(E1A00000);                                   // nop
                break;
            } case OP_ENTER: {
                // Begin procedure body, adjust stack $PARM octets for frame (always at least 8 (i.e. 2 words)).
                // Frame contains all local storage/variables and arguments space for any calls within this procedure.
                Emit32(E52DE004);                                   // push {lr}
                EmitRegisterLoadConst(R0, *(uint32_t *)arg);
                Emit32(E0466000);                                   // sub r6, r6, r0
                i += sizeof(uint32_t);
                break;
            } case OP_LEAVE: {
                // End procedure body, $PARM is same as that of the matching ENTER.
                EmitRegisterLoadConst(R0, *(uint32_t *)arg);
                Emit32(E0866000);                                   // add r6, r6, r0
                Emit32(E49DF004);                                   // pop {pc}

                EmitLocalConstantPool(qtrue);
                i += sizeof(uint32_t);
                break;
            } case OP_LOCAL: {
                // Get address of local storage (local variable or argument) (TOS <- (frame + $PARM)).
                EmitRegisterLoadConst(R1, *(uint32_t *)arg);
                Emit32(E0860001);                                   // add r0, r6, r1
                Emit32(E0400005);                                   // sub r0, r0, r5

                EmitReg2TOS(R0, qtrue);
                i += sizeof(uint32_t);
                break;
            } case OP_CONST: {
                // Push literal value onto stack (TOS <- $PARM).
                EmitRegisterLoadConst(R0, *(uint32_t *)arg);
                EmitReg2TOS(R0, qtrue);
                i += sizeof(uint32_t);
                break;
            } case OP_LOAD1: {
                // Load 1-octet value from address in TOS (TOS <- [TOS]).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0000008);                                   // and r0, r0, r8

                Emit32(E7D50000);                                   // ldrb r0, [r5, r0]
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_LOAD2: {
                // Load 2-octet value from address in TOS (TOS <- [TOS]).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0000008);                                   // and r0, r0, r8
                Emit32(E3C00001);                                   // bic r0, r0, #1

                Emit32(E19500B0);                                   // ldrh r0, [r5, r0]
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_LOAD4: {
                // Load 4-octet value from address in TOS (TOS <- [TOS]).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0000008);                                   // and r0, r0, r8
                Emit32(E3C00003);                                   // bic r0, r0, #3

                Emit32(E7950000);                                   // ldr r0, [r5, r0]
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_STORE1: {
                // TOS is 1-octet value to store, destination address in next-in-stack ([NIS] <- TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qtrue);
                Emit32(E0000008);                                   // and r0, r0, r8

                Emit32(E7C51000);                                   // strb r1, [r5, r0]
                break;
            } case OP_STORE2: {
                // TOS is 2-octet value to store, destination address in next-in-stack ([NIS] <- TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qtrue);
                Emit32(E0000008);                                   // and r0, r0, r8
                Emit32(E3C00001);                                   // bic r0, r0, #1

                Emit32(E18510B0);                                   // strh r1, [r5, r0]
                break;
            } case OP_STORE4: {
                // TOS is 4-octet value to store, destination address in next-in-stack ([NIS] <- TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qtrue);
                Emit32(E0000008);                                   // and r0, r0, r8
                Emit32(E3C00003);                                   // bic r0, r0, #3

                Emit32(E7851000);                                   // str r1, [r5, r0]
                break;
            } case OP_PUSH: {
                // Push nonsense (void) value to opstack (TOS <- 0).
                Emit32(E28AA001);                                   // add r10, #1
                Emit32(E20AA0FF);                                   // and r10, #255
                break;
            } case OP_POP: {
                // Pop a value from stack (remove TOS, decrease stack by 1).
                Emit32(E24AA001);                                   // sub r10, #1
                Emit32(E20AA0FF);                                   // and r10, #255
                break;
            } case OP_ARG: {
                // TOS is 4-octet value to store into arguments-marshalling space of the indicated octet offset (ARGS[offset] <- TOS).
                Emit32(E0460005);                                   // sub r0, r6, r5
                EmitRegisterLoadConst(R1, *(uint8_t *)arg);
                Emit32(E0800001);                                   // add r0, r0, r1
                Emit32(E0000008);                                   // and r0, r0, r8
                Emit32(E3C00003);                                   // bic r0, r0, #3

                EmitTOS2Reg(R1, qtrue);
                Emit32(E7851000);                                   // str r1, [r5, r0]
                i += sizeof(uint8_t);
                break;
            } case OP_BLOCK_COPY: {
                // Copy $PARM bytes from [TOS] to [NIS]
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qtrue);
                Emit32(E92D07F8);                                   // push {r3-r10}
                EmitRegisterLoadConst(R2, currOp);
                EmitRegisterLoadConst(R3, *(uint32_t *)arg);
                Emit32(E5995004);                                   // ldr r5, [r9, #4] (VM_ARM_VOp)
                Emit32(E12FFF35);                                   // blx r5
                Emit32(E8BD07F8);                                   // pop {r3-r10}
                i += sizeof(uint32_t);
                break;
            /* ---------------------------------------------------------------------------- */
            /* --------------------------------- BRANCHES --------------------------------- */
            /* ---------------------------------------------------------------------------- */
            } case OP_CALL: {
                // Make call to procedure (code address <- TOS).
                EmitTOS2Reg(R0, qtrue);
                Emit32(E3500000);                                   // cmp r0, #0
                Emit32(BA000006);                                   // blt dosyscall
                Emit32(E1500004);                                   // cmp r0, r4
                Emit32(2A000002);                                   // bcs errjmp

                // in-vm-call:
                Emit32(E7931100);                                   // ldr r1, [r3, r0, lsl #2]
                Emit32(E12FFF31);                                   // blx r1
                Emit32(EA000007);                                   // b outjmp

                // errjmp:
                Emit32(E5991008);                                   // ldr r1, [r9, #8] (VM_CompiledErrorJump)
                Emit32(E12FFF31);                                   // blx r1

                // dosyscall:
                Emit32(E28AA001);                                   // add r10, #1
                Emit32(E20AA0FF);                                   // and r10, #255
                Emit32(E92D07F8);                                   // push {r3-r10}
                Emit32(E5991000);                                   // ldr r1, [r9] (VM_CompiledSyscall)
                Emit32(E12FFF31);                                   // blx r1
                Emit32(E8BD07F8);                                   // pop {r3-r10}

                // outjmp:
                break;
            } case OP_JUMP: {
                // Branch (code address <- TOS)
                EmitTOS2Reg(R0, qtrue);
                Emit32(E1500004);                                   // cmp r0, r4
                Emit32(3A000001);                                   // bcc dojmp

                // errjmp:
                Emit32(E5991008);                                   // ldr r1, [r9, #8] (VM_CompiledErrorJump)
                Emit32(E12FFF31);                                   // blx r1

                // dojmp:
                Emit32(E7931100);                                   // ldr r1, [r3, r0, lsl #2]
                Emit32(E12FFF11);                                   // bx r1
                break;
            } case OP_EQ:
              case OP_NE:
              case OP_LTI:
              case OP_LEI:
              case OP_GTI:
              case OP_GEI:
              case OP_LTU:
              case OP_LEU:
              case OP_GTU:
              case OP_GEU: {
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qtrue);
                Emit32(E1500001);                                   // cmp r0, r1
                EmitConstantJumpInstruction(vm, currOp, *(uint32_t *)arg);
                i += sizeof(uint32_t);
                break;
            } case OP_EQF:
              case OP_NEF:
              case OP_LTF:
              case OP_LEF:
              case OP_GTF:
              case OP_GEF: {
                if (cpu.vfp) {
                    EmitTOS2SReg(S1, qtrue);
                    EmitTOS2SReg(S0, qtrue);
                    Emit32(EEB40AE0);                               // fcmpes s0, s1
                    Emit32(EEF1FA10);                               // fmstat
                    EmitConstantJumpInstruction(vm, currOp, *(uint32_t *)arg);
                } else {
                    EmitTOS2Reg(R1, qtrue);
                    EmitTOS2Reg(R0, qtrue);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, currOp);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}

                    Emit32(E3500001);                               // cmp r0, #1
                    EmitConstantJumpInstruction(vm, OP_EQ, *(uint32_t *)arg);
                }
                i += sizeof(uint32_t);
                break;
            /* ---------------------------------------------------------------------------- */
            /* ---------------------------- INTEGER OPERATIONS ---------------------------- */
            /* ---------------------------------------------------------------------------- */
            } case OP_SEX8: {
                // Sign-extend 8-bit (TOS <- TOS).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E6AF0070);                                   // sxtb r0, r0
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_SEX16: {
                // Sign-extend 16-bit (TOS <- TOS).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E6BF0070);                                   // sxth r0, r0
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_NEGI: {
                // Negate signed integer (TOS <- -TOS).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E2600000);                                   // neg r0, r0
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_ADD: {
                // Add integer-wise (TOS <- NIS + TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0800001);                                   // add r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_SUB: {
                // Subtract integer-wise (TOS <- NIS - TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0400001);                                   // sub r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_DIVI: {
                // Divide (signed integer) (TOS <- NIS / TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);

                if (cpu.idiva) {
                    Emit32(E710F110);                               // sdiv r0, r0, r1
                } else {
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_DIVI);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                }

                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_DIVU: {
                // Divide (unsigned integer) (TOS <- NIS / TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);

                if (cpu.idiva) {
                    Emit32(E730F110);                               // udiv r0, r0, r1
                } else {
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_DIVU);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                }

                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_MODI: {
                // Modulo (signed integer) (TOS <- NIS mod TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);

                if (cpu.idiva && cpu.mls) {
                    Emit32(E712F110);                               // sdiv r2, r0, r1
                    Emit32(E0600291);                               // mls r0, r1, r2, r0
                } else {
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_MODI);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                }

                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_MODU: {
                // Modulo (unsigned integer) (TOS <- NIS mod TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);

                if (cpu.idiva && cpu.mls) {
                    Emit32(E732F110);                               // udiv r2, r0, r1
                    Emit32(E0600291);                               // mls r0, r1, r2, r0
                } else {
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_MODU);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                }

                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_MULI:
              case OP_MULU: {
                // Multiply (signed/unsigned integer) (TOS <- NIS * TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0000190);                                   // mul r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_BAND: {
                // Bitwise AND (TOS <- NIS & TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0000001);                                   // and r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_BOR: {
                // Bitwise OR (TOS <- NIS | TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E1800001);                                   // orr r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_BXOR: {
                // Bitwise XOR (TOS <- NIS ^ TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E0200001);                                   // eor r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_BCOM: {
                // Bitwise complement (TOS <- ~TOS).
                EmitTOS2Reg(R0, qfalse);
                Emit32(E1E00000);                                   // mvn r0, r0
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_LSH: {
                // Bitwise left-shift (TOS <- NIS << TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E1A00110);                                   // lsl r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_RSHI: {
                // Algebraic (signed) right-shift (TOS <- NIS >> TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E1A00150);                                   // asr r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            } case OP_RSHU: {
                // Bitwise (unsigned) right-shift (TOS <- NIS >> TOS).
                EmitTOS2Reg(R1, qtrue);
                EmitTOS2Reg(R0, qfalse);
                Emit32(E1A00130);                                   // lsr r0, r0, r1
                EmitReg2TOS(R0, qfalse);
                break;
            /* ---------------------------------------------------------------------------- */
            /* ----------------------------- FLOAT OPERATIONS ----------------------------- */
            /* ---------------------------------------------------------------------------- */
            } case OP_NEGF: {
                // Negate float value (TOS <- -TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EEB10A40);                               // fnegs s0, s0
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_NEGF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_ADDF: {
                // Add integer-wise (TOS <- NIS + TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S1, qtrue);
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EE300A20);                               // fadds s0, s0, s1
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qtrue);
                    EmitTOS2Reg(R0, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_ADDF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_SUBF: {
                // Subtract floats (TOS <- NIS - TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S1, qtrue);
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EE300A60);                               // fsubs s0, s0, s1
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qtrue);
                    EmitTOS2Reg(R0, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_SUBF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_DIVF: {
                // Divide floats (TOS <- NIS / TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S1, qtrue);
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EE800A20);                               // fdivs s0, s0, s1
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qtrue);
                    EmitTOS2Reg(R0, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_DIVF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_MULF: {
                // Multiply floats (TOS <- NIS x TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S1, qtrue);
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EE200A20);                               // fmuls s0, s0, s1
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qtrue);
                    EmitTOS2Reg(R0, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_MULF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_CVIF: {
                // Convert signed integer to float (TOS <- TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EEB80AC0);                               // fsitos s0, s0
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_CVIF);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } case OP_CVFI: {
                // Convert float to signed integer (TOS <- TOS).
                if (cpu.vfp) {
                    EmitTOS2SReg(S0, qfalse);
                    Emit32(EEBD0AC0);                               // ftosizs s0, s0
                    EmitSReg2TOS(S0, qfalse);
                } else {
                    EmitTOS2Reg(R1, qfalse);
                    Emit32(E92D07F8);                               // push {r3-r10}
                    EmitRegisterLoadConst(R2, OP_CVFI);
                    Emit32(E5995004);                               // ldr r5, [r9, #4] (VM_ARM_VOp)
                    Emit32(E12FFF35);                               // blx r5
                    Emit32(E8BD07F8);                               // pop {r3-r10}
                    EmitReg2TOS(R0, qfalse);
                }
                break;
            } default:
                Com_Error(ERR_FATAL, "unknown instruction %#02x!", currOp);
            }
        }
    }

    // copy to an exact sized buffer with the appropriate permission bits
    vm->codeLength = nativeCodeLength;

    vm->codeBase = (byte *)mmap(NULL, nativeCodeLength, PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if(vm->codeBase == MAP_FAILED) {
        Com_Error(ERR_FATAL, "VM_CompileARM: can't mmap memory");
    }

    Com_Memcpy( vm->codeBase, nativeCode, nativeCodeLength );

    if(mprotect(vm->codeBase, nativeCodeLength, PROT_READ|PROT_EXEC)) {
        Com_Error(ERR_FATAL, "VM_CompileARM: mprotect failed");
    }

    Z_Free(nativeCode);
    Z_Free(localConstantPoolPointers);

    Com_Printf( "VM file %s compiled to %zu bytes of code\n", vm->name, nativeCodeLength );

    // offset all the instruction pointers for the new location
    for ( int i = 0 ; i < header->instructionCount ; i++ ) {
        vm->instructionPointers[i] += (intptr_t)vm->codeBase;
    }
}

int __attribute__((optimize("O0"))) VM_CallCompiled(vm_t *vm, int *args) {
    int     stack[OPSTACK_SIZE / sizeof(int)];
    unsigned int stackIndex;
    int		programStack, stackOnEntry;
    int     *programStackPtr;
    byte	*image;
    int     *stackPtr;
    int		arg;

    currentVM = vm;

    // interpret the code
    vm->currentlyInterpreting = qtrue;

    // we might be called recursively, so this might not be the very top
    programStack = stackOnEntry = vm->programStack;

    // set up the stack frame
    image = vm->dataBase;

    programStack -= ( 8 + 4 * MAX_VMMAIN_ARGS );

    for ( arg = 0; arg < MAX_VMMAIN_ARGS; arg++ )
        *(int *)&image[ programStack + 8 + arg * 4 ] = args[ arg ];

    *(int *)&image[ programStack + 4 ] = 0;	// return stack
    *(int *)&image[ programStack ] = -1;	// will terminate the loop on return

    programStackPtr = (int *)&image[ programStack ];

    // set up opStack
    *(unsigned int *)stack = 0xDEADC0DEu;
    stackPtr = stack;
    stackIndex = 0;

    // off we go into generated code...
    __asm__ volatile (
        "push {r0-r10,lr}\n"

        "ldr r6,  %[programStack]\n"
        "ldr r7,  %[stack]\n"
        "ldr r9,  %[codeAddress]\n"
        "ldr r10, %[stackIndex]\n"

        "mov r0, r9\n"
        "add r0, #12\n"
        "blx r0\n"

        "str r6, %[programStackOut]\n"
        "str r10, %[stackIndexOut]\n"

        "pop {r0-r10,lr}"
        : [programStackOut] "=m" (programStackPtr),
          [stackIndexOut] "=m" (stackIndex)
        : [codeAddress] "m" (vm->codeBase),
          [programStack] "m" (programStackPtr),
          [stack] "m" (stackPtr),
          [stackIndex] "m" (stackIndex)
        : "r0", "r6", "r7", "r9", "r10"
    );

    if (*(unsigned int *)stack != 0xDEADC0DEu || stackIndex != 1) {
        Com_Error(ERR_DROP, "opStack corrupted in VM_CallCompiled");
    }

    if (programStackPtr != (int *)&image[programStack]) {
        Com_Error(ERR_DROP, "programStack corrupted in VM_CallCompiled");
    }

    vm->programStack = stackOnEntry;

    return stack[stackIndex];
}
