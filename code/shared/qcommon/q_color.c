#include "q_color.h"

vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorOrange = {1, 0.5, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

vec4_t		colorLtBlue	= {0.367f, 0.261f, 0.722f, 1};
vec4_t		colorDkBlue	= {0.199f, 0.0f,   0.398f, 1};

vec4_t g_color_table[Q_COLOR_BITS+1] = {
	{ 0.0, 0.0, 0.0, 1.0 },	// black
	{ 1.0, 0.0, 0.0, 1.0 },	// red
	{ 0.0, 1.0, 0.0, 1.0 },	// green
	{ 1.0, 1.0, 0.0, 1.0 },	// yellow
	{ 0.0, 0.0, 1.0, 1.0 },	// blue
	{ 0.0, 1.0, 1.0, 1.0 },	// cyan
	{ 1.0, 0.0, 1.0, 1.0 },	// magenta
	{ 1.0, 1.0, 1.0, 1.0 },	// white
	{ 1.0, 0.5, 0.0, 1.0 }, // orange
	{ 0.5, 0.5, 0.5, 1.0 },	// md.grey
};

vec4_t colorTable[CT_MAX] =
{
	{0, 0, 0, 0},			// CT_NONE
	{0, 0, 0, 1},			// CT_BLACK
	{1, 0, 0, 1},			// CT_RED
	{0, 1, 0, 1},			// CT_GREEN
	{0, 0, 1, 1},			// CT_BLUE
	{1, 1, 0, 1},			// CT_YELLOW
	{1, 0, 1, 1},			// CT_MAGENTA
	{0, 1, 1, 1},			// CT_CYAN
	{1, 1, 1, 1},			// CT_WHITE
	{0.75f, 0.75f, 0.75f, 1},	// CT_LTGREY
	{0.50f, 0.50f, 0.50f, 1},	// CT_MDGREY
	{0.25f, 0.25f, 0.25f, 1},	// CT_DKGREY
	{0.15f, 0.15f, 0.15f, 1},	// CT_DKGREY2

	{0.992f, 0.652f, 0.0f,  1},	// CT_VLTORANGE -- needs values
	{0.810f, 0.530f, 0.0f,  1},	// CT_LTORANGE
	{0.610f, 0.330f, 0.0f,  1},	// CT_DKORANGE
	{0.402f, 0.265f, 0.0f,  1},	// CT_VDKORANGE

	{0.503f, 0.375f, 0.996f, 1},	// CT_VLTBLUE1
	{0.367f, 0.261f, 0.722f, 1},	// CT_LTBLUE1
	{0.199f, 0.0f,   0.398f, 1},	// CT_DKBLUE1
	{0.160f, 0.117f, 0.324f, 1},	// CT_VDKBLUE1

	{0.300f, 0.628f, 0.816f, 1},	// CT_VLTBLUE2 -- needs values
	{0.300f, 0.628f, 0.816f, 1},	// CT_LTBLUE2
	{0.191f, 0.289f, 0.457f, 1},	// CT_DKBLUE2
	{0.125f, 0.250f, 0.324f, 1},	// CT_VDKBLUE2

	{0.796f, 0.398f, 0.199f, 1},	// CT_VLTBROWN1 -- needs values
	{0.796f, 0.398f, 0.199f, 1},	// CT_LTBROWN1
	{0.558f, 0.207f, 0.027f, 1},	// CT_DKBROWN1
	{0.328f, 0.125f, 0.035f, 1},	// CT_VDKBROWN1

	{0.996f, 0.796f, 0.398f, 1},	// CT_VLTGOLD1 -- needs values
	{0.996f, 0.796f, 0.398f, 1},	// CT_LTGOLD1
	{0.605f, 0.441f, 0.113f, 1},	// CT_DKGOLD1
	{0.386f, 0.308f, 0.148f, 1},	// CT_VDKGOLD1

	{0.648f, 0.562f, 0.784f, 1},	// CT_VLTPURPLE1 -- needs values
	{0.648f, 0.562f, 0.784f, 1},	// CT_LTPURPLE1
	{0.437f, 0.335f, 0.597f, 1},	// CT_DKPURPLE1
	{0.308f, 0.269f, 0.375f, 1},	// CT_VDKPURPLE1

	{0.816f, 0.531f, 0.710f, 1},	// CT_VLTPURPLE2 -- needs values
	{0.816f, 0.531f, 0.710f, 1},	// CT_LTPURPLE2
	{0.566f, 0.269f, 0.457f, 1},	// CT_DKPURPLE2
	{0.343f, 0.226f, 0.316f, 1},	// CT_VDKPURPLE2

	{0.929f, 0.597f, 0.929f, 1},	// CT_VLTPURPLE3
	{0.570f, 0.371f, 0.570f, 1},	// CT_LTPURPLE3
	{0.355f, 0.199f, 0.355f, 1},	// CT_DKPURPLE3
	{0.285f, 0.136f, 0.230f, 1},	// CT_VDKPURPLE3

	{0.953f, 0.378f, 0.250f, 1},	// CT_VLTRED1
	{0.953f, 0.378f, 0.250f, 1},	// CT_LTRED1
	{0.593f, 0.121f, 0.109f, 1},	// CT_DKRED1
	{0.429f, 0.171f, 0.113f, 1},	// CT_VDKRED1
	{.25f, 0, 0, 1},				// CT_VDKRED
	{.70f, 0, 0, 1},				// CT_DKRED

	{0.717f, 0.902f, 1.0f,   1},	// CT_VLTAQUA
	{0.574f, 0.722f, 0.804f, 1},	// CT_LTAQUA
	{0.287f, 0.361f, 0.402f, 1},	// CT_DKAQUA
	{0.143f, 0.180f, 0.201f, 1},	// CT_VDKAQUA

	{0.871f, 0.386f, 0.375f, 1},	// CT_LTPINK
	{0.435f, 0.193f, 0.187f, 1},	// CT_DKPINK
	{	  0,    .5f,    .5f, 1},	// CT_LTCYAN
	{	  0,   .25f,   .25f, 1},	// CT_DKCYAN
	{   .179f, .51f,   .92f, 1},	// CT_LTBLUE3
	{   .199f, .71f,   .92f, 1},	// CT_LTBLUE3
	{   .5f,   .05f,    .4f, 1},	// CT_DKBLUE3

	{   0.0f,   .613f,  .097f, 1},	// CT_HUD_GREEN
	{   0.835f, .015f,  .015f, 1},	// CT_HUD_RED
	{	.567f,	.685f,	1.0f,	.75f},	// CT_ICON_BLUE
	{	.515f,	.406f,	.507f,	1},	// CT_NO_AMMO_RED

	{   1.0f,   .658f,  .062f, 1},	// CT_HUD_ORANGE
	{	0.549f, .854f,  1.0f,  1.0f},	//	CT_TITLE
};

unsigned ColorBytes3 (float r, float g, float b) {
	unsigned i;

	( (byte *)&i )[0] = (byte)(r * 255);
	( (byte *)&i )[1] = (byte)(g * 255);
	( (byte *)&i )[2] = (byte)(b * 255);

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned i;

	( (byte *)&i )[0] = (byte)(r * 255);
	( (byte *)&i )[1] = (byte)(g * 255);
	( (byte *)&i )[2] = (byte)(b * 255);
	( (byte *)&i )[3] = (byte)(a * 255);

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;

	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}
