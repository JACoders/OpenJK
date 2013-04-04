;------------------------------------------------------------------------------
; Vertex components (as specified in the vertex DECL)
;    v0  = vPosition
;    v1  = vFields
;    v2  = vTex
;------------------------------------------------------------------------------
xvs.1.1

#define LEFT		r6
#define DOWN		r7
#define TEMPPOS		r8
#define TEMP		r9
#define TEMPALPHA	r10

#define CV_ONE				1
#define CV_WORLDVIEWPROJ_0	2
#define CV_WORLDVIEWPROJ_1	3
#define CV_WORLDVIEWPROJ_2	4
#define CV_WORLDVIEWPROJ_3	5
#define CV_ALPHA			20
#define CV_FADEALPHA		21
#define CV_LEFT				26
#define CV_DOWN				27


; get possible alpha source
; alpha = mAlpha * (pos.y / -item->pos.z);
;rcp r10.x, -v0.z
;mul r10.x, v2.z, r10.x
;mul r10.x, c[CV_ALPHA].w, r10.x

; if (alpha > mAlpha) alpha = mAlpha
;min r10.x, r10.x, c[CV_ALPHA].w

; set the constant color to add alpha to
mov oD0, c[CV_ONE]

; set the alpha fade
;mov r11.w, c[CV_ALPHA].w
;mul oD0.w, c[CV_FADEALPHA].w, r10.x
mov oD0.w, v2.z

; Pass thru the tex coords
mov oT0.xy, v2.xy

; Add in other angles based on the field
mul LEFT, v1.y, c[CV_LEFT]
mul DOWN, v1.z, c[CV_DOWN]

; Set the final position
add r4, v0, LEFT
add r4, DOWN, r4

; Transform position to clip space
dp4 oPos.x, r4, c[CV_WORLDVIEWPROJ_0]
dp4 oPos.y, r4, c[CV_WORLDVIEWPROJ_1]
dp4 oPos.z, r4, c[CV_WORLDVIEWPROJ_2]
dp4 oPos.w, r4, c[CV_WORLDVIEWPROJ_3]

