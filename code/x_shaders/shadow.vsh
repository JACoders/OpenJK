;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = pVertex[i].p
;    v1  = extrusion determinant
;------------------------------------------------------------------------------
xvs.1.1
     
#include "../win32/shader_constants.h"  

#pragma screenspace

; Determine the distance to the ground
add r4, v0, c[CV_SHADOW_FACTORS]
sub r5, r4, c[CV_SHADOW_PLANE]

; Factor in the extrusion determinant
; r3 will either be the distance to the ground, or 0
mul r3, v1.x, -r5
  
; Extrude the vertex if necessary
mad r0, r3.z, c[CV_LIGHT_DIRECTION].xyz, v0.xyz
mov r0.w, v0.w
 
; transform to hclip space
m4x4 oPos, r0, c[CV_WORLDVIEWPROJ_0]
 
; Multiply by 1/w and add viewport offset.
; r12 is a read-only alias for oPos.
rcc r1.x, r12.w
mad oPos.xyz, r12, r1.x, c[CV_VIEWPORT_OFFSETS]   