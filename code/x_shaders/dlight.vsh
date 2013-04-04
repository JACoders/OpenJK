;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = pVertex[i].p
;    v1  = pVertex[i].n
;    v2  = pVertex[i].t0
;    v3  = pVertex[i].Tangent
;------------------------------------------------------------------------------
xvs.1.1

#include "../win32/shader_constants.h"

#pragma screenspace

; Transform position for world, view, and projection matrices
m4x4 oPos, v0, c[CV_WORLDVIEWPROJ_0]

; Multiply by 1/w and add viewport offset.
; r12 is a read-only alias for oPos.
rcc r1.x, r12.w
mad oPos.xyz, r12, r1.x, c[CV_VIEWPORT_OFFSETS]

; Pass thru the base tex coords
mov oT0, v2
mov oT1, v2

; Generate binormal vector
mov r7, v3
mul r8, r7.yzxw, v1.zxyw
mad r8, -r7.zxyw, v1.yzxw, r8	
 
; Get the point light vector
add r1, c[CV_LIGHT_POSITION], -v0  

; Divide each component by the range value
mul r2.xyz, r1.xyz, c[CV_ONE_OVER_LIGHT_RANGE].x

; Multiply with 0.5 and add 0.5
; Put it in tex coord set 1
mad oT2.xyz, r2.xyz, c[CV_HALF].yyyy, c[CV_HALF].yyyy

; Move the point light vector into tangent space
dp3 r9.x, r1, v3
dp3 r9.y, r1, r8
dp3 r9.z, r1, v1

; Put the tangent space vector in tex coord set 2
mov oT3.xyz, r9.xyz



	  	  





