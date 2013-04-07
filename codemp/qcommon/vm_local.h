//rww - so that I may utilize vm debugging features WITHOUT DROPPING TO 0.1FPS
#ifndef _XBOX
#define CRAZY_SYMBOL_MAP
#endif

#ifdef CRAZY_SYMBOL_MAP
#include <map>
#endif

typedef enum {
	OP_UNDEF, 

	OP_IGNORE, 

	OP_BREAK,

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[top]
	OP_ARG,

	OP_BLOCK_COPY,

	//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} opcode_t;



typedef int	vmptr_t;

typedef struct vmSymbol_s {
	struct vmSymbol_s	*next;
	int		symValue;
	int		profileCount;
	char	symName[1];		// variable sized
} vmSymbol_t;

#define	VM_OFFSET_PROGRAM_STACK		0
#define	VM_OFFSET_SYSTEM_CALL		4

struct vm_s {
    // DO NOT MOVE OR CHANGE THESE WITHOUT CHANGING THE VM_OFFSET_* DEFINES
    // USED BY THE ASM CODE
    int			programStack;		// the vm may be recursively entered
    int			(*systemCall)( int *parms );

	//------------------------------------
   
    char		name[MAX_QPATH];

	// for dynamic linked modules
	void		*dllHandle;
	int			(QDECL *entryPoint)( int callNum, ... );

	// for interpreted modules
	qboolean	currentlyInterpreting;

	qboolean	compiled;
	byte		*codeBase;
	int			codeLength;

	int			*instructionPointers;
	int			instructionPointersLength;

	byte		*dataBase;
	int			dataMask;

	int			stackBottom;		// if programStack < stackBottom, error

	int			numSymbols;
	struct vmSymbol_s	*symbols;

	int			callLevel;			// for debug indenting
	int			breakFunction;		// increment breakCount on function entry to this
	int			breakCount;
};

#ifdef CRAZY_SYMBOL_MAP
typedef std::map<int, vmSymbol_s*> symbolMap_t;
typedef std::map<vm_t*, symbolMap_t> symbolVMMap_t;

extern symbolVMMap_t		g_vmMap;
extern symbolMap_t			*g_symbolMap;

/*
Set the symbol map based on the VM currently
being in interpreted. This is done so that we
do not have to do a map lookup for the VM with
each symbol request.
-rww
*/
inline void VM_SetSymbolMap(vm_t *vm)
{
	g_symbolMap = &g_vmMap[vm];
}
#endif


extern	vm_t	*currentVM;
extern	int		vm_debugLevel;

void VM_Compile( vm_t *vm, vmHeader_t *header );
int	VM_CallCompiled( vm_t *vm, int *args );

void VM_PrepareInterpreter( vm_t *vm, vmHeader_t *header );
int	VM_CallInterpreted( vm_t *vm, int *args );

vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value );
int VM_SymbolToValue( vm_t *vm, const char *symbol );
const char *VM_ValueToSymbol( vm_t *vm, int value );
void VM_LogSyscalls( int *args );

