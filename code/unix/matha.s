//
// math.s
// x86 assembly-language math routines.

#define GLQUAKE	1	// don't include unneeded defs
#include "qasm.h"


#if	id386

	.text

// TODO: rounding needed?
// stack parameter offset
#define	val	4

.globl C(Invert24To16)
C(Invert24To16):

	movl	val(%esp),%ecx
	movl	$0x100,%edx		// 0x10000000000 as dividend
	cmpl	%edx,%ecx
	jle		LOutOfRange

	subl	%eax,%eax
	divl	%ecx

	ret

LOutOfRange:
	movl	$0xFFFFFFFF,%eax
	ret

#endif	// id386
