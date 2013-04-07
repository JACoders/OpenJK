;
; qftol -- fast floating point to long conversion.
;

segment .data

temp    dd 0.0
fpucw   dd 0

; Precision Control Field , 2 bits / 0x0300 
; PC24 0x0000   Single precision (24 bits). 
; PC53 0x0200   Double precision (53 bits). 
; PC64 0x0300   Extended precision (64 bits).

; Rounding Control Field, 2 bits / 0x0C00
; RCN  0x0000   Rounding to nearest (even). 
; RCD  0x0400   Rounding down (directed, minus). 
; RCU  0x0800   Rounding up (directed plus).
; RC0  0x0C00   Rounding towards zero (chop mode).


; rounding towards nearest (even)
cw027F  dd 0x027F ; double precision
cw037F  dd 0x037F ; extended precision

; rounding towards zero (chop mode)
cw0E7F  dd 0x0E7F ; double precision
cw0F7F  dd 0x0F7F ; extended precision


segment .text

;
; int qftol( void ) - default control word
;

global qftol

qftol: 
        fistp dword [temp]
        mov eax, [temp]
        ret


;
; int qftol027F( void ) - DirectX FPU 
;

global qftol027F

qftol027F:
	fnstcw [fpucw]
	fldcw  [cw027F]  
        fistp dword [temp]
	fldcw  [fpucw]
        mov eax, [temp]
        ret

;
; int qftol037F( void ) - Linux FPU 
;

global qftol037F

qftol037F:
	fnstcw [fpucw]
	fldcw  [cw037F]  
        fistp dword [temp]
	fldcw  [fpucw]
        mov eax, [temp]
        ret


;
; int qftol0F7F( void ) - ANSI
;

global qftol0F7F

qftol0F7F:
	fnstcw [fpucw]
	fldcw  [cw0F7F]  
        fistp dword [temp]
	fldcw  [fpucw]
        mov eax, [temp]
        ret

;
; int qftol0E7F( void ) 
;

global qftol0E7F

qftol0E7F:
	fnstcw [fpucw]
	fldcw  [cw0E7F]  
        fistp dword [temp]
	fldcw  [fpucw]
        mov eax, [temp]
        ret



;
; long Q_ftol( float q )
;

global Q_ftol

Q_ftol:
        fld dword [esp+4]  
        fistp dword [temp]
        mov eax, [temp]
        ret


;
; long qftol0F7F( float q ) - Linux FPU 
;

global Q_ftol0F7F

Q_ftol0F7F:
	fnstcw [fpucw]
	fld dword [esp+4]  
	fldcw  [cw0F7F]  
        fistp dword [temp]
	fldcw  [fpucw]
        mov eax, [temp]
        ret

