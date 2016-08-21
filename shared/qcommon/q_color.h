#pragma once

#include "q_math.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a

#define Q_COLOR_ESCAPE	'^'
#define Q_COLOR_BITS 0xF // was 7

// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '9' && *((p)+1) >= '0' )
#define Q_IsColorStringExt(p)	((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) >= '0' && *((p)+1) <= '9') // ^[0-9]

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define COLOR_ORANGE	'8'
#define COLOR_GREY		'9'
#define ColorIndex(c)	( ( (c) - '0' ) & Q_COLOR_BITS )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_ORANGE	"^8"
#define S_COLOR_GREY	"^9"

typedef enum ct_table_e
{
	CT_NONE,
	CT_BLACK,
	CT_RED,
	CT_GREEN,
	CT_BLUE,
	CT_YELLOW,
	CT_MAGENTA,
	CT_CYAN,
	CT_WHITE,
	CT_LTGREY,
	CT_MDGREY,
	CT_DKGREY,
	CT_DKGREY2,

	CT_VLTORANGE,
	CT_LTORANGE,
	CT_DKORANGE,
	CT_VDKORANGE,

	CT_VLTBLUE1,
	CT_LTBLUE1,
	CT_DKBLUE1,
	CT_VDKBLUE1,

	CT_VLTBLUE2,
	CT_LTBLUE2,
	CT_DKBLUE2,
	CT_VDKBLUE2,

	CT_VLTBROWN1,
	CT_LTBROWN1,
	CT_DKBROWN1,
	CT_VDKBROWN1,

	CT_VLTGOLD1,
	CT_LTGOLD1,
	CT_DKGOLD1,
	CT_VDKGOLD1,

	CT_VLTPURPLE1,
	CT_LTPURPLE1,
	CT_DKPURPLE1,
	CT_VDKPURPLE1,

	CT_VLTPURPLE2,
	CT_LTPURPLE2,
	CT_DKPURPLE2,
	CT_VDKPURPLE2,

	CT_VLTPURPLE3,
	CT_LTPURPLE3,
	CT_DKPURPLE3,
	CT_VDKPURPLE3,

	CT_VLTRED1,
	CT_LTRED1,
	CT_DKRED1,
	CT_VDKRED1,
	CT_VDKRED,
	CT_DKRED,

	CT_VLTAQUA,
	CT_LTAQUA,
	CT_DKAQUA,
	CT_VDKAQUA,

	CT_LTPINK,
	CT_DKPINK,
	CT_LTCYAN,
	CT_DKCYAN,
	CT_LTBLUE3,
	CT_BLUE3,
	CT_DKBLUE3,

	CT_HUD_GREEN,
	CT_HUD_RED,
	CT_ICON_BLUE,
	CT_NO_AMMO_RED,
	CT_HUD_ORANGE,

	CT_TITLE,

	CT_MAX
} ct_table_t;

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorLtBlue;
extern vec4_t colorDkBlue;

extern vec4_t g_color_table[Q_COLOR_BITS+1];
extern vec4_t colorTable[CT_MAX];

unsigned ColorBytes3 (float r, float g, float b);
unsigned ColorBytes4 (float r, float g, float b, float a);
float NormalizeColor( const vec3_t in, vec3_t out );

#if defined(__cplusplus)
} // extern "C"
#endif
