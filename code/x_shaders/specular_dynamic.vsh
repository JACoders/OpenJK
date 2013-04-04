;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = pVertex[i].p
;    v1  = pVertex[i].n
;    v2  = pVertex[i].t0
;    v3  = pVertex[i].basis.vTangent;
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

; Generate binormal vector
mov r7, v3
mul r8, r7.yzxw, v1.zxyw
mad r8, -r7.zxyw, v1.yzxw, r8	

; Get the light vector
add r10, c[CV_LIGHT_POSITION], -v0

; Divide each component by the range value
mul r3.xyz, r10.xyz, c[CV_ONE_OVER_LIGHT_RANGE].x

; Multiply with 0.5 and add 0.5
; Put it in tex coord set 3
mad oT3.xyz, r3.xyz, c[CV_HALF].yyyy, c[CV_HALF].yyyy

; Normalize the light vector
dp3 r11.x, r10.xyz, r10.xyz
rsq r11.xyz, r11.x
mul r10.xyz, r10.xyz, r11.xyz

; Move the light vector into tangent space
; Put into T1
dp3 oT1.x, r10, v3
dp3 oT1.y, r10, r8
dp3 oT1.z, r10, v1	

; Get the vector toward the camera
mov r2, -c[CV_CAMERA_DIRECTION]	

; Get the half angle
add r2.xyz, r2.xyz, r10.xyz

; Normalize half angle
dp3 r11.x, r2.xyz, r2.xyz
rsq r11.xyz, r11.x
mul r2.xyz, r2.xyz, r11.xyz

; Move the half angle into tangent space
dp3 oT2.x, r2, v3
dp3 oT2.y, r2, r8
dp3 oT2.z, r2, v1

; Move the bump map coordinates into t0
mov oT0.xyz, v2



