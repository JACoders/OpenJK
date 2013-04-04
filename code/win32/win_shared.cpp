// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"




#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void)
{
	static int sys_timeBase = timeGetTime();
	int			sys_curtime;

	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/
static inline void CPUID( int func, unsigned int regs[4] )
{
	unsigned regEAX, regEBX, regECX, regEDX;

	__asm mov eax, func
	__asm __emit 00fh
	__asm __emit 0a2h
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx

	regs[0] = regEAX;
	regs[1] = regEBX;
	regs[2] = regECX;
	regs[3] = regEDX;
}

static int IsPentium( void )
{
	__asm 
	{
		pushfd						// save eflags
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		set21				// bit 21 is not set, so jump to set_21
		and		eax, 0xffdfffff		// clear bit 21
		push	eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		good
		jmp		err					// cpuid not supported
set21:
		or		eax, 0x00200000		// set ID bit
		push	eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop		eax
		test	eax, 0x00200000		// if bit 21 is on
		jnz		good
		jmp		err
	}

err:
	return qfalse;
good:
	return qtrue;
}

static int Is3DNOW( void )
{
	unsigned regs[4];
	char pstring[16];
	char processorString[13];

	// get name of processor
	CPUID( 0, ( unsigned int * ) pstring );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

//  REMOVED because you can have 3DNow! on non-AMD systems
//	if ( strcmp( processorString, "AuthenticAMD" ) )
//		return qfalse;

	// check AMD-specific functions
	CPUID( 0x80000000, regs );
	if ( regs[0] < 0x80000000 )
		return qfalse;

	// bit 31 of EDX denotes 3DNOW! support
	CPUID( 0x80000001, regs );
	if ( regs[3] & ( 1 << 31 ) )
		return qtrue;

	return qfalse;
}

static int IsKNI( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 25 of EDX denotes KNI existence
	if ( regs[3] & ( 1 << 25 ) )
	{
		// Ok, CPU supports this instruction, but does the OS?
		//
		// Test a KNI instruction and make sure you don't get an exception...
		//
		__try
		{
			__asm
			{
				pushad;
			//	orps xmm1,xmm1;		// Below are the op codes for this instruction
									// emits will compile w/ MSVC 5.0 compiler
									// You can comment these out and uncomment the
									// orps when using the Intel Compiler
				__emit 0x0f
				__emit 0x56
				__emit 0xc9
				popad;
			}
		}// If OS creates an exception, it doesn't support Pentium III Instructions
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return qfalse;
		}

		return qtrue;
	}

	return qfalse;
}

static int IsWIL( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 26 of EDX denotes WIL existence
	if ( regs[3] & ( 1 << 26 ) )
	{
		// Ok, CPU supports this instruction, but does the OS?
		//
		// Test a WIL instruction and make sure you don't get an exception...
		//
		__try
		{
			__asm
			{
				pushad;
			// 	xorpd xmm0,xmm0;  // Willamette New Instructions 
					__emit 0x0f
					__emit 0x56
					__emit 0xc9
				popad;
			}
		}// If OS creates an exception, it doesn't support PentiumIV Instructions
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
//			if(_exception_code()==STATUS_ILLEGAL_INSTRUCTION)	// forget it, any exception should count as fail for safety
				return qfalse; // Willamette New Instructions not supported
		}

		return qtrue;	// Williamette/P4 instructions available
	}

	return qfalse;

}


static int IsMMX( void )
{
	unsigned int regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		return qtrue;
	return qfalse;
}

int Sys_GetProcessorId( void )
{
#if defined _M_ALPHA
	return CPUID_AXP;
#elif !defined _M_IX86
	return CPUID_GENERIC;
#else

	// verify we're at least a Pentium or 486 w/ CPUID support
	if ( !IsPentium() )
		return CPUID_INTEL_UNSUPPORTED;

	// check for MMX
	if ( !IsMMX() )
	{
		// Pentium or PPro
		return CPUID_INTEL_PENTIUM;
	}

	// see if we're an AMD 3DNOW! processor
	if ( Is3DNOW() )
	{
		return CPUID_AMD_3DNOW;
	}

	// see if we're an Intel Katmai
	if ( IsKNI() )
	{
		// if we are, see if we're a Williamette as well...
		//
		if ( IsWIL() )
		{
			return CPUID_INTEL_WILLIAMETTE;
		}
		return CPUID_INTEL_KATMAI;
	}

	// by default we're functionally a vanilla Pentium/MMX or P2/MMX
	return CPUID_INTEL_MMX;

#endif
}

//============================================

char *Sys_GetCurrentUser( void )
{
#ifdef _XBOX
	return NULL;
#else
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if ( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
#endif
}