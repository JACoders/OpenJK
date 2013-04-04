;
; Sys_SnapVector NASM code (Andrew Henderson)
; See win32/win_shared.c for the Win32 equivalent
; This code is provided to ensure that the
;  rounding behavior (and, if necessary, the
;  precision) of DLL and QVM code are identical
;  e.g. for network-visible operations.
; See ftol.nasm for operations on a single float,
;  as used in compiled VM and DLL code that does
;  not use this system trap.
;


segment .data

fpucw	dd	0
cw037F  dd 0x037F   ; Rounding to nearest (even). 

segment .text

; void Sys_SnapVector( float *v )
global Sys_SnapVector
Sys_SnapVector:
	push 	eax
	push 	ebp
	mov	ebp, esp

	fnstcw	[fpucw]
	mov	eax, dword [ebp + 12]
	fldcw	[cw037F]	
	fld	dword [eax]
	fistp	dword [eax]
	fild	dword [eax]
	fstp	dword [eax]
	fld	dword [eax + 4]
	fistp	dword [eax + 4]
	fild	dword [eax + 4]
	fstp	dword [eax + 4]
	fld	dword [eax + 8]
	fistp	dword [eax + 8]
	fild	dword [eax + 8]
	fstp	dword [eax + 8]
	fldcw	[fpucw]
	
	pop ebp
	pop eax
	ret
	
; void Sys_SnapVectorCW( float *v, unsigned short int cw )
global Sys_SnapVectorCW
Sys_SnapVector_cw:
	push 	eax
	push 	ebp
	mov	ebp, esp

	fnstcw	[fpucw]
	mov	eax, dword [ebp + 12]
	fldcw	[ebp + 16]	
	fld	dword [eax]
	fistp	dword [eax]
	fild	dword [eax]
	fstp	dword [eax]	
	fld	dword [eax + 4]
	fistp	dword [eax + 4]
	fild	dword [eax + 4]
	fstp	dword [eax + 4]
	fld	dword [eax + 8]
	fistp	dword [eax + 8]
	fild	dword [eax + 8]
	fstp	dword [eax + 8]
	fldcw	[fpucw]
	
	pop ebp
	pop eax
	ret