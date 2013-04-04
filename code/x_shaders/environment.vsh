;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = pVertex[i].p
;    v1  = pVertex[i].n
;------------------------------------------------------------------------------
xvs.1.1

#include "../win32/shader_constants.h"

#pragma screenspace

; Get the view vector
add r2, c[CV_CAMERA_DIRECTION], -v0

; Normalize the view vector
dp3 r11.x, r2.xyz, r2.xyz
rsq r11.xyz, r11.x
mul r2.xyz, r2.xyz, r11.xyz

; Get the dot product of the view vector
; and the vertex normal
dp3 r3.x, r2, v1

; Add the offsets
mul r4.x, v1.x, r3.x
mul r4.y, c[CV_HALF].x, r2.x
sub oT0.x, r4.x, r4.y

mul r4.x, v1.y, r3.x
mul r4.y, c[CV_HALF].x, r2.y
sub oT0.y, r4.x, r4.y

mov oT0.z, c[CV_ONE].z	

; Transform position for world, view, and projection matrices
m4x4 oPos, v0, c[CV_WORLDVIEWPROJ_0]

; Multiply by 1/w and add viewport offset.
; r12 is a read-only alias for oPos.
rcc r1.x, r12.w
mad oPos.xyz, r12, r1.x, c[CV_VIEWPORT_OFFSETS]

; Set the color
mov oD0, v2

; Transform vertex to view space
m4x4 r0, v0, c[CV_VIEW_0]

; Use distance from vertex to eye as fog factor
dp3 r0.w, r0, r0
rsq r1.w, r0.w
mul oFog.x, r0.w, r1.w

