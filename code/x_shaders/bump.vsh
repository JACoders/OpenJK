;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = pVertex[i].p
;    v1  = pVertex[i].n
;    v2  = pVertex[i].t0
;	 v3  = pVertex[i].t1
;    v4  = pVertex[i].basis.vTangent;
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
mov oT1, v3

; Generate binormal vector
mov r7, v4
mul r8, r7.yzxw, v1.zxyw
mad r8, -r7.zxyw, v1.yzxw, r8	

; Get the light vector  
mov r1, c[CV_LIGHT_DIRECTION]

; Move the point light vector into tangent space
; Put in tex coord set 2
dp3 r3.x, r1, v4
dp3 r3.y, r1, r8
dp3 r3.z, r1, v1

; Multiply with 0.5 and add 0.5
; Put it in diffuse
mad oD0.xyz, r3.xyz, c[CV_HALF].yyyy, c[CV_HALF].yyyy

; Get the vector toward the camera
mov r2, -c[CV_CAMERA_DIRECTION]	

; Get the half angle
add r2.xyz, r2.xyz, r1.xyz

; Normalize half angle
dp3 r11.x, r2.xyz, r2.xyz
rsq r11.xyz, r11.x
mul r2.xyz, r2.xyz, r11.xyz

; Move the half angle into tangent space
; Put in tex coord set 3
dp3 r3.x, r2, v4
dp3 r3.y, r2, r8
dp3 r3.z, r2, v1

; Multiply with 0.5 and add 0.5
; Put it in specular
mad oD1.xyz, r3.xyz, c[CV_HALF].yyyy, c[CV_HALF].yyyy
