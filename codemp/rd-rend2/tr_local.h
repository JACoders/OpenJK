/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "qcommon/q_shared.h"
#include "qcommon/qfiles.h"
#include "qcommon/qcommon.h"
#include "rd-common/tr_public.h"
#include "rd-common/tr_common.h"
#include "tr_allocator.h"
#include "tr_extratypes.h"
#include "tr_extramath.h"
#include "tr_fbo.h"
#include "tr_postprocess.h"
#include "iqm.h"
#include "qgl.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef unsigned int glIndex_t;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS	14
#define MAX_SHADERS		(1<<SHADERNUM_BITS)

#define	MAX_FBOS      256
#define MAX_VISCOUNTS 5
#define MAX_VBOS      4096
#define MAX_IBOS      4096
#define MAX_G2_BONES  72

#define MAX_CALC_PSHADOWS    64
#define MAX_DRAWN_PSHADOWS    32 // do not increase past 32, because bit flags are used on surfaces
#define PSHADOW_MAP_SIZE      1024
#define DSHADOW_MAP_SIZE      512
#define CUBE_MAP_MIPS      8
#define CUBE_MAP_ROUGHNESS_MIPS CUBE_MAP_MIPS - 2
#define CUBE_MAP_SIZE      (1 << CUBE_MAP_MIPS)

/*
=====================================================

Renderer-side Cvars
In Q3, these are defined in tr_common.h, which isn't very logical really
In JA, we define these in the tr_local.h, which is much more logical

=====================================================
*/

extern cvar_t	*r_verbose;
extern cvar_t	*r_ignore;

extern cvar_t	*r_detailTextures;

extern cvar_t	*r_znear;
extern cvar_t	*r_zproj;
extern cvar_t	*r_stereoSeparation;

extern cvar_t	*r_skipBackEnd;

extern cvar_t	*r_stereo;
extern cvar_t	*r_anaglyphMode;

extern cvar_t	*r_greyscale;

extern cvar_t	*r_ignorehwgamma;
extern cvar_t	*r_measureOverdraw;

extern cvar_t	*r_inGameVideo;
extern cvar_t	*r_fastsky;
extern cvar_t	*r_drawSun;
extern cvar_t	*r_dynamiclight;

extern cvar_t	*r_lodbias;
extern cvar_t	*r_lodscale;
extern cvar_t	*r_autolodscalevalue;

extern cvar_t	*r_norefresh;
extern cvar_t	*r_drawentities;
extern cvar_t	*r_drawworld;
extern cvar_t	*r_drawfog;
extern cvar_t	*r_speeds;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_novis;
extern cvar_t	*r_nocull;
extern cvar_t	*r_facePlaneCull;
extern cvar_t	*r_showcluster;
extern cvar_t	*r_nocurves;

extern cvar_t	*r_allowExtensions;

extern cvar_t	*r_ext_compressed_textures;
extern cvar_t	*r_ext_multitexture;
extern cvar_t	*r_ext_compiled_vertex_array;
extern cvar_t	*r_ext_texture_env_add;
extern cvar_t	*r_ext_texture_filter_anisotropic;

extern cvar_t  *r_ext_draw_range_elements;
extern cvar_t  *r_ext_multi_draw_arrays;
extern cvar_t  *r_ext_texture_float;
extern cvar_t  *r_arb_half_float_pixel;
extern cvar_t  *r_ext_framebuffer_multisample;
extern cvar_t  *r_arb_seamless_cube_map;

extern cvar_t  *r_cameraExposure;

extern cvar_t  *r_hdr;

extern cvar_t  *r_toneMap;
extern cvar_t  *r_forceToneMap;
extern cvar_t  *r_forceToneMapMin;
extern cvar_t  *r_forceToneMapAvg;
extern cvar_t  *r_forceToneMapMax;

extern cvar_t  *r_autoExposure;
extern cvar_t  *r_forceAutoExposure;
extern cvar_t  *r_forceAutoExposureMin;
extern cvar_t  *r_forceAutoExposureMax;

extern cvar_t  *r_depthPrepass;
extern cvar_t  *r_ssao;

extern cvar_t  *r_normalMapping;
extern cvar_t  *r_specularMapping;
extern cvar_t  *r_deluxeMapping;
extern cvar_t  *r_deluxeSpecular;
extern cvar_t  *r_parallaxMapping;
extern cvar_t  *r_normalAmbient;
extern cvar_t  *r_dlightMode;
extern cvar_t  *r_pshadowDist;
extern cvar_t  *r_imageUpsample;
extern cvar_t  *r_imageUpsampleMaxSize;
extern cvar_t  *r_imageUpsampleType;
extern cvar_t  *r_genNormalMaps;
extern cvar_t  *r_forceSun;
extern cvar_t  *r_forceSunMapLightScale;
extern cvar_t  *r_forceSunLightScale;
extern cvar_t  *r_forceSunAmbientScale;
extern cvar_t  *r_sunlightMode;
extern cvar_t  *r_drawSunRays;
extern cvar_t  *r_sunShadows;
extern cvar_t  *r_shadowFilter;
extern cvar_t  *r_shadowMapSize;
extern cvar_t  *r_shadowCascadeZNear;
extern cvar_t  *r_shadowCascadeZFar;
extern cvar_t  *r_shadowCascadeZBias;
extern cvar_t  *r_ignoreDstAlpha;

extern cvar_t	*r_ignoreGLErrors;
extern cvar_t	*r_logFile;

extern cvar_t	*r_stencilbits;
extern cvar_t	*r_depthbits;
extern cvar_t	*r_colorbits;
extern cvar_t	*r_texturebits;
extern cvar_t  *r_ext_multisample;

extern cvar_t	*r_drawBuffer;
extern cvar_t	*r_lightmap;
extern cvar_t	*r_vertexLight;
extern cvar_t	*r_uiFullScreen;
extern cvar_t	*r_shadows;
extern cvar_t	*r_flares;
extern cvar_t	*r_mode;
extern cvar_t	*r_nobind;
extern cvar_t	*r_singleShader;
extern cvar_t	*r_roundImagesDown;
extern cvar_t	*r_colorMipLevels;
extern cvar_t	*r_picmip;
extern cvar_t	*r_showtris;
extern cvar_t	*r_showsky;
extern cvar_t	*r_shownormals;
extern cvar_t	*r_finish;
extern cvar_t	*r_clear;
extern cvar_t	*r_swapInterval;
extern cvar_t	*r_markcount;
extern cvar_t	*r_textureMode;
extern cvar_t	*r_offsetFactor;
extern cvar_t	*r_offsetUnits;
extern cvar_t	*r_shadowOffsetFactor;
extern cvar_t	*r_shadowOffsetUnits;
extern cvar_t	*r_gamma;
extern cvar_t	*r_intensity;
extern cvar_t	*r_lockpvs;
extern cvar_t	*r_noportals;
extern cvar_t	*r_portalOnly;

extern cvar_t	*r_subdivisions;
extern cvar_t	*r_lodCurveError;

extern cvar_t	*r_fullscreen;
extern cvar_t  *r_noborder;

extern cvar_t	*r_customwidth;
extern cvar_t	*r_customheight;
extern cvar_t	*r_customPixelAspect;

extern cvar_t	*r_overBrightBits;
extern cvar_t	*r_mapOverBrightBits;

extern cvar_t	*r_debugSurface;
extern cvar_t	*r_simpleMipMaps;

extern cvar_t	*r_showImages;

extern cvar_t	*r_ambientScale;
extern cvar_t	*r_directedScale;
extern cvar_t	*r_debugLight;
extern cvar_t	*r_debugSort;
extern cvar_t	*r_printShaders;
extern cvar_t	*r_saveFontData;

extern cvar_t	*r_marksOnTriangleMeshes;

extern cvar_t	*r_aviMotionJpegQuality;
extern cvar_t	*r_screenshotJpegQuality;
extern cvar_t	*r_surfaceSprites;

extern cvar_t	*r_maxpolys;
extern int		max_polys;
extern cvar_t	*r_maxpolyverts;
extern int		max_polyverts;

extern	cvar_t	*r_aspectCorrectFonts;

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
extern cvar_t	*r_noPrecacheGLA;
#endif

extern cvar_t	*r_noServerGhoul2;
extern cvar_t	*r_Ghoul2AnimSmooth;
extern cvar_t	*r_Ghoul2UnSqashAfterSmooth;
//extern cvar_t	*r_Ghoul2UnSqash;
//extern cvar_t	*r_Ghoul2TimeBase=0; from single player
//extern cvar_t	*r_Ghoul2NoLerp;
//extern cvar_t	*r_Ghoul2NoBlend;
//extern cvar_t	*r_Ghoul2BlendMultiplier=0;

extern cvar_t	*broadsword;
extern cvar_t	*broadsword_kickbones;
extern cvar_t	*broadsword_kickorigin;
extern cvar_t	*broadsword_playflop;
extern cvar_t	*broadsword_dontstopanim;
extern cvar_t	*broadsword_waitforshot;
extern cvar_t	*broadsword_smallbbox;
extern cvar_t	*broadsword_extra1;
extern cvar_t	*broadsword_extra2;

extern cvar_t	*broadsword_effcorr;
extern cvar_t	*broadsword_ragtobase;
extern cvar_t	*broadsword_dircap;

/*
Ghoul2 Insert End
*/

extern cvar_t	*r_patchStitching;

/*
End Cvars
*/

typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_SRGB           = 0x0080,
	IMGFLAG_GENNORMALMAP   = 0x0100,
	IMGFLAG_MUTABLE        = 0x0200,
	IMGFLAG_HDR            = 0x0400,
	IMGFLAG_HDR_LIGHTMAP   = 0x0800,
	IMGFLAG_2D_ARRAY       = 0x1000,
	IMGFLAG_3D             = 0x2000,
	IMGLFAG_SHADOWCOMP     = 0x4000,
} imgFlags_t;

typedef enum
{
	ANIMMAP_NORMAL,
	ANIMMAP_CLAMP,
	ANIMMAP_ONESHOT
} animMapType_t;

enum
{
	ATTR_INDEX_POSITION,
	ATTR_INDEX_TEXCOORD0,
	ATTR_INDEX_TEXCOORD1,
	ATTR_INDEX_TEXCOORD2,
	ATTR_INDEX_TEXCOORD3,
	ATTR_INDEX_TEXCOORD4,
	ATTR_INDEX_TANGENT,
	ATTR_INDEX_NORMAL,
	ATTR_INDEX_COLOR,
	ATTR_INDEX_LIGHTDIRECTION,
	ATTR_INDEX_BONE_INDEXES,
	ATTR_INDEX_BONE_WEIGHTS,

	// GPU vertex animations and some extra sprite info
	ATTR_INDEX_POSITION2,
#ifdef REND2_SP
	ATTR_INDEX_TANGENT2,
	ATTR_INDEX_NORMAL2,
#endif // REND2_SP

	ATTR_INDEX_MAX
};

enum
{
	XFB_VAR_POSITION,
	XFB_VAR_VELOCITY,

	XFB_VAR_COUNT
};
static const int NO_XFB_VARS = 0;

typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	int			width, height, layers;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;					// gl texture binding

	int			frameUsed;			// for texture usage in frame statistics

	int			internalFormat;
	int			TMU;				// only needed for voodoo2

	imgType_t   type;
	int			flags;

	struct image_s *next;
	struct image_s *poolNext;
} image_t;

typedef struct cubemap_s {
	char name[MAX_QPATH];
	vec3_t origin;
	float parallaxRadius;
	image_t *image;
} cubemap_t;

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct trRefEntity_s {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	qboolean	mirrored;		// mirrored matrix, needs reversed culling
	vec3_t		lightDir;		// normalized direction towards light, in world space
	vec3_t      modelLightDir;  // normalized direction towards light, in model space
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
} trRefEntity_t;


typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelViewMatrix[16];
	float		modelMatrix[16];
} orientationr_t;

void R_SetOrientationOriginAndAxis(
	orientationr_t& orientation,
	const vec3_t origin,
	const vec3_t forward,
	const vec3_t left,
	const vec3_t up);
void R_SetOrientationOriginAndAxis(
	orientationr_t& orientation,
	const vec3_t origin,
	const matrix3_t axis);

typedef enum
{
	VBO_USAGE_STATIC,
	VBO_USAGE_DYNAMIC,
	VBO_USAGE_XFB
} vboUsage_t;

typedef struct VBO_s
{
	uint32_t        vertexesVBO;
	int             vertexesSize;	// amount of memory data allocated for all vertices in bytes

	uint32_t		offsets[ATTR_INDEX_MAX];
	uint32_t		strides[ATTR_INDEX_MAX];
	uint32_t		sizes[ATTR_INDEX_MAX];
} VBO_t;

typedef struct IBO_s
{
	uint32_t        indexesVBO;
	int             indexesSize;	// amount of memory data allocated for all triangles in bytes
//  uint32_t        ofsIndexes;
} IBO_t;

//===============================================================================

typedef enum {
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_INSIDE,			// inside body parts (i.e. heart)
	SS_MID_INSIDE,
	SS_MIDDLE,
	SS_MID_OUTSIDE,
	SS_OUTSIDE,			// outside body parts (i.e. ribs)

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH,
	GF_INVERSE_SAWTOOTH,

	GF_NOISE,
	GF_RAND

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_BULGE_UNIFORM,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7,
	DEFORM_DISINTEGRATION
} deform_t;

// deformVertexes types that can be handled by the GPU
typedef enum
{
	// do not edit: same as genFunc_t

	DGEN_NONE,
	DGEN_WAVE_SIN,
	DGEN_WAVE_SQUARE,
	DGEN_WAVE_TRIANGLE,
	DGEN_WAVE_SAWTOOTH,
	DGEN_WAVE_INVERSE_SAWTOOTH,
	DGEN_WAVE_NOISE,

	// do not edit until this line

	DGEN_BULGE,
	DGEN_NORMALS,
	DGEN_MOVE,
} deformGen_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
	CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_LIGHTING_DIFFUSE_ENTITY, // diffuse lighting * entity
	CGEN_FOG,				// standard fog
	CGEN_CONST,				// fixed color
	CGEN_LIGHTMAPSTYLE,		// lightmap style
	CGEN_DISINTEGRATION_1,
	CGEN_DISINTEGRATION_2
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_LIGHTMAP1,
	TCGEN_LIGHTMAP2,
	TCGEN_LIGHTMAP3,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;

enum surfaceSpriteType_t
{
	SURFSPRITE_NONE,
	SURFSPRITE_VERTICAL,
	SURFSPRITE_ORIENTED,
	SURFSPRITE_EFFECT,
	SURFSPRITE_WEATHERFX,
	SURFSPRITE_FLATTENED,
};

enum surfaceSpriteOrientation_t
{
	SURFSPRITE_FACING_NORMAL,
	SURFSPRITE_FACING_UP,
	SURFSPRITE_FACING_DOWN,
	SURFSPRITE_FACING_ANY,
};

struct SurfaceSpriteBlock
{
	vec2_t fxGrow;
	float fxDuration;
	float fadeStartDistance;
	float fadeEndDistance;
	float fadeScale;
	float wind;
	float windIdle;
	float fxAlphaStart;
	float fxAlphaEnd;
	float pad0[2];
};

struct CameraBlock
{
	matrix_t viewProjectionMatrix;
	vec4_t viewInfo;
	vec3_t viewOrigin;
	float pad0;
	vec3_t viewForward;
	float pad1;
	vec3_t viewLeft;
	float pad2;
	vec3_t viewUp;
	float pad3;
};

struct SceneBlock
{
	vec4_t primaryLightOrigin;
	vec3_t primaryLightAmbient;
	int	   globalFogIndex;
	vec3_t primaryLightColor;
	float primaryLightRadius;
	float currentTime;
	float frameTime;
	float pad0[2];
};

struct LightsBlock
{
	struct Light
	{
		vec4_t origin;
		vec3_t color;
		float radius;
	};

	matrix_t shadowVP1;
	matrix_t shadowVP2;
	matrix_t shadowVP3;

	int numLights;
	float pad0[3];

	Light lights[MAX_DLIGHTS];
};

struct FogsBlock
{
	struct Fog
	{
		vec4_t plane;
		vec4_t color;
		float depthToOpaque;
		int hasPlane;
		float pad1[2];
	};

	int numFogs;
	float pad0[3];
	Fog fogs[16];
};

struct EntityBlock
{
	matrix_t modelMatrix;
	vec4_t lightOrigin;
	vec3_t ambientLight;
	float lightRadius;
	vec3_t directedLight;
	float fxVolumetricBase;
	vec3_t modelLightDir;
	float vertexLerp;
	vec3_t localViewOrigin;
	float entityTime;
};

struct ShaderInstanceBlock
{
	vec4_t deformParams0;
	vec4_t deformParams1;
	float time;
	float portalRange;
	int deformType;
	int deformFunc;
};

struct SkeletonBoneMatricesBlock
{
	mat3x4_t matrices[MAX_G2_BONES];
};

struct surfaceSprite_t
{
	surfaceSpriteType_t type;

	float width;
	float height;
	float density;
	float wind;
	float windIdle;
	float fadeDist;
	float fadeMax;
	float fadeScale;
	float fxAlphaStart;
	float fxAlphaEnd;
	float fxDuration;
	float vertSkew;

	vec2_t variance;
	vec2_t fxGrow;
	surfaceSpriteOrientation_t facing;

	int spriteUboOffset;
};

#define	MAX_IMAGE_ANIMATIONS	(32)

typedef struct {
	image_t			*image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		oneShotAnimMap;
	qboolean		isVideoMap;
} textureBundle_t;

enum
{
	TB_COLORMAP    = 0,
	TB_DIFFUSEMAP  = 0,
	TB_LIGHTMAP    = 1,
	TB_LEVELSMAP   = 1,
	TB_COLORMAP2   = 1,
	TB_NORMALMAP   = 2,
	TB_DELUXEMAP   = 3,
	TB_SPECULARMAP = 4,
	TB_ORMSMAP     = 4,
	TB_SHADOWMAP   = 5,
	TB_CUBEMAP     = 6,
	TB_ENVBRDFMAP  = 7,
	TB_SHADOWMAPARRAY  = 8,
	TB_SSAOMAP     = 9,
	NUM_TEXTURE_BUNDLES = 10
};

typedef enum
{
	// material shader stage types
	ST_COLORMAP = 0,			// vanilla Q3A style shader treatening
	ST_DIFFUSEMAP = 0,          // treat color and diffusemap the same
	ST_GLSL
} stageType_t;

typedef enum
{
	SPEC_NONE,		// no specular found
	SPEC_SPECGLOSS,	// Specular Gloss
	SPEC_RMO,		// calculate spec from rmo  texture with a specular of 0.04 for dielectric materials
	SPEC_RMOS,		// calculate spec from rmos texture with a specular of 0.0 - 0.08 from input
	SPEC_MOXR,		// calculate spec from moxr texture with a specular of 0.04 for dielectric materials
	SPEC_MOSR,		// calculate spec from mosr texture with a specular of 0.0 - 0.08 from input
	SPEC_ORM,		// calculate spec from orm  texture with a specular of 0.04 for dielectric materials
	SPEC_ORMS,		// calculate spec from orms texture with a specular of 0.0 - 0.08 from input
} specularType_t;

enum AlphaTestType
{
	ALPHA_TEST_NONE,
	ALPHA_TEST_GT0,
	ALPHA_TEST_LT128,
	ALPHA_TEST_GE128,
	ALPHA_TEST_GE192,
};

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_EXTERNAL	-5
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

typedef struct {
	qboolean		active;
	qboolean		isDetail;
	qboolean		glow;
	qboolean		cloth;

	AlphaTestType	alphaTestType;

	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	float			constantColor[4];			// for CGEN_CONST and AGEN_CONST

	uint32_t		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	int				lightmapStyle;

	stageType_t     type;
	specularType_t  specularType;
	struct shaderProgram_s *glslShaderGroup;
	int glslShaderIndex;

	vec4_t normalScale;
	vec4_t specularScale;
	float  parallaxBias;

	surfaceSprite_t	*ss;

} shaderStage_t;

struct shaderCommands_s;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6];
} skyParms_t;

typedef struct {
	vec3_t	color;
	float	depthForOpaque;
} fogParms_t;


typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex[MAXLIGHTMAPS];	// for a shader to match, both name and all lightmapIndex must match
	byte		styles[MAXLIGHTMAPS];

	int			index;					// this shader == tr.shaders[index]
	int			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	qboolean	entityMergable;			// merge across entites optimizable (smoke, blood)

	qboolean	isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at
	qboolean	isPortal;

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean	polygonOffset;			// set for decals and other items that must be offset
	qboolean	noMipMaps;				// for console fonts, 2D elements, etc.
	qboolean	noPicMip;				// for images that must always be full resolution
	qboolean	noTC;					// for images that don't want to be texture compressed (eg skies)

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	int         vertexAttribs;          // not all shaders will need all data to be gathered

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int			numUnfoggedPasses;
	int			numSurfaceSpriteStages;
	GLuint		spriteUbo;
	int			ShaderInstanceUboOffset;

	shaderStage_t	*stages[MAX_SHADER_STAGES];

	void		(*optimalStageIteratorFunc)( void );
	qboolean	isHDRLit;
	qboolean	useSimpleDepthShader;
	qboolean	useDistortion;

	float clampTime;                                  // time this shader is clamped to
	float timeOffset;                                 // current time offset for this shader

	struct shader_s *remappedShader;                  // current shader this one is remapped too

	struct	shader_s	*next;
} shader_t;

QINLINE qboolean ShaderRequiresCPUDeforms(const shader_t * shader)
{
	if ( shader->numDeforms > 1 )
	{
		return qtrue;
	}

	if ( shader->numDeforms > 0 )
	{
		switch (shader->deforms[0].deformation)
		{
			case DEFORM_NONE:
			case DEFORM_NORMALS:
			case DEFORM_WAVE:
			case DEFORM_BULGE:
			case DEFORM_MOVE:
			case DEFORM_PROJECTION_SHADOW:
				return qfalse;

			default:
				return qtrue;
		}
	}

	assert( shader->numDeforms == 0 );

	return qfalse;
}

enum
{
	GLS_SRCBLEND_ZERO					= (1 << 0),
	GLS_SRCBLEND_ONE					= (1 << 1),
	GLS_SRCBLEND_DST_COLOR				= (1 << 2),
	GLS_SRCBLEND_ONE_MINUS_DST_COLOR	= (1 << 3),
	GLS_SRCBLEND_SRC_ALPHA				= (1 << 4),
	GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 5),
	GLS_SRCBLEND_DST_ALPHA				= (1 << 6),
	GLS_SRCBLEND_ONE_MINUS_DST_ALPHA	= (1 << 7),
	GLS_SRCBLEND_ALPHA_SATURATE			= (1 << 8),

	GLS_SRCBLEND_BITS					= GLS_SRCBLEND_ZERO
											| GLS_SRCBLEND_ONE
											| GLS_SRCBLEND_DST_COLOR
											| GLS_SRCBLEND_ONE_MINUS_DST_COLOR
											| GLS_SRCBLEND_SRC_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_SRCBLEND_DST_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
											| GLS_SRCBLEND_ALPHA_SATURATE,

	GLS_DSTBLEND_ZERO					= (1 << 9),
	GLS_DSTBLEND_ONE					= (1 << 10),
	GLS_DSTBLEND_SRC_COLOR				= (1 << 11),
	GLS_DSTBLEND_ONE_MINUS_SRC_COLOR	= (1 << 12),
	GLS_DSTBLEND_SRC_ALPHA				= (1 << 13),
	GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 14),
	GLS_DSTBLEND_DST_ALPHA				= (1 << 15),
	GLS_DSTBLEND_ONE_MINUS_DST_ALPHA	= (1 << 16),

	GLS_DSTBLEND_BITS					= GLS_DSTBLEND_ZERO
											| GLS_DSTBLEND_ONE
											| GLS_DSTBLEND_SRC_COLOR
											| GLS_DSTBLEND_ONE_MINUS_SRC_COLOR
											| GLS_DSTBLEND_SRC_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_DSTBLEND_DST_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,

	GLS_DEPTHMASK_TRUE					= (1 << 17),

	GLS_POLYMODE_LINE					= (1 << 18),

	GLS_DEPTHTEST_DISABLE				= (1 << 19),

	GLS_DEPTHFUNC_LESS					= (1 << 20),
	GLS_DEPTHFUNC_EQUAL					= (1 << 21),
	GLS_DEPTHFUNC_GREATER				= (1 << 22),

	GLS_DEPTHFUNC_BITS					= GLS_DEPTHFUNC_LESS
											| GLS_DEPTHFUNC_EQUAL
											| GLS_DEPTHFUNC_GREATER,

	GLS_REDMASK_FALSE					= (1 << 23),
	GLS_GREENMASK_FALSE					= (1 << 24),
	GLS_BLUEMASK_FALSE					= (1 << 25),
	GLS_ALPHAMASK_FALSE					= (1 << 26),

	GLS_COLORMASK_BITS					= GLS_REDMASK_FALSE
											| GLS_GREENMASK_FALSE
											| GLS_BLUEMASK_FALSE
											| GLS_ALPHAMASK_FALSE,

	GLS_STENCILTEST_ENABLE				= (1 << 27),

	GLS_POLYGON_OFFSET_FILL				= (1 << 28),

	GLS_DEFAULT							= GLS_DEPTHMASK_TRUE
};

struct Attribute
{
	int numComponents;
	bool integerAttribute;
	GLenum type;
	bool normalize;
	int offset;
};

const int MAX_ATTRIBUTES = 8;
struct VertexFormat
{
	Attribute attributes[MAX_ATTRIBUTES];
};

enum
{
	ATTR_POSITION		= 0x0001,
	ATTR_TEXCOORD0		= 0x0002,
	ATTR_TEXCOORD1		= 0x0004,
	ATTR_TEXCOORD2		= 0x0008,
	ATTR_TEXCOORD3		= 0x0010,
	ATTR_TEXCOORD4		= 0x0020,
	ATTR_TANGENT		= 0x0040,
	ATTR_NORMAL			= 0x0080,
	ATTR_COLOR			= 0x0100,
	ATTR_LIGHTDIRECTION = 0x0200,
	ATTR_BONE_INDEXES	= 0x0400,
	ATTR_BONE_WEIGHTS	= 0x0800,

	// for .md3 interpolation and some sprite data
	ATTR_POSITION2		= 0x1000,
#ifdef REND2_SP
	ATTR_TANGENT2		= 0x2000,
	ATTR_NORMAL2		= 0x4000,
#endif // REND2_SP

	ATTR_DEFAULT		= ATTR_POSITION,
	ATTR_BITS			= ATTR_POSITION |
							ATTR_TEXCOORD0 |
							ATTR_TEXCOORD1 |
							ATTR_TEXCOORD2 |
							ATTR_TEXCOORD3 |
							ATTR_TEXCOORD4 |
							ATTR_TANGENT |
							ATTR_NORMAL |
							ATTR_COLOR |
							ATTR_LIGHTDIRECTION |
							ATTR_BONE_INDEXES |
							ATTR_BONE_WEIGHTS |
							ATTR_POSITION2
#ifdef REND2_SP
							|
							ATTR_TANGENT2 |
							ATTR_NORMAL2
#endif // REND2_SP
};

enum
{
	GENERICDEF_USE_DEFORM_VERTEXES 		= 0x0001,
	GENERICDEF_USE_TCGEN_AND_TCMOD 		= 0x0002,
	GENERICDEF_USE_FOG             		= 0x0004,
	GENERICDEF_USE_RGBAGEN         		= 0x0008,
	GENERICDEF_USE_SKELETAL_ANIMATION	= 0x0010,
	GENERICDEF_USE_GLOW_BUFFER      	= 0x0020,
	GENERICDEF_USE_ALPHA_TEST			= 0x0040,
#ifdef REND2_SP
	GENERICDEF_USE_VERTEX_ANIMATION		= 0x0080,
	GENERICDEF_ALL						= 0x00FF,
#else
	GENERICDEF_ALL						= 0x007F,
#endif // REND2_SP

	GENERICDEF_COUNT                	= GENERICDEF_ALL + 1,
};

enum
{
	FOGDEF_USE_DEFORM_VERTEXES  		= 0x0001,
	FOGDEF_USE_SKELETAL_ANIMATION 		= 0x0002,
	FOGDEF_USE_ALPHA_TEST				= 0x0004,
	FOGDEF_USE_FALLBACK_GLOBAL_FOG		= 0x0008,
#ifdef REND2_SP
	FOGDEF_USE_VERTEX_ANIMATION			= 0x0010,
	FOGDEF_ALL							= 0x001F,
#else
	FOGDEF_ALL							= 0x000F,
#endif // REND2_SP

	FOGDEF_COUNT                		= FOGDEF_ALL + 1,
};

enum
{
	REFRACTIONDEF_USE_DEFORM_VERTEXES		= 0x0001,
	REFRACTIONDEF_USE_TCGEN_AND_TCMOD		= 0x0002,
	REFRACTIONDEF_USE_RGBAGEN				= 0x0004,
	REFRACTIONDEF_USE_SKELETAL_ANIMATION	= 0x0008,
	REFRACTIONDEF_USE_ALPHA_TEST			= 0x0010,
	REFRACTIONDEF_USE_SRGB_TRANSFORM		= 0x0020,
#ifdef REND2_SP
	REFRACTIONDEF_USE_VERTEX_ANIMATION		= 0x0040,
	REFRACTIONDEF_ALL						= 0x007F,
#else
	REFRACTIONDEF_ALL						= 0x003F,
#endif // REND2_SP

	REFRACTIONDEF_COUNT						= REFRACTIONDEF_ALL + 1,
};

enum
{
	LIGHTDEF_USE_LIGHTMAP        		= 0x0001,
	LIGHTDEF_USE_LIGHT_VECTOR    		= 0x0002,
	LIGHTDEF_USE_LIGHT_VERTEX    		= 0x0003,
	LIGHTDEF_USE_TCGEN_AND_TCMOD 		= 0x0004,
	LIGHTDEF_USE_PARALLAXMAP     		= 0x0008,
//	LIGHTDEF_USE_SHADOWMAP       		= 0x0010,
	LIGHTDEF_USE_SKELETAL_ANIMATION 	= 0x0010,
	LIGHTDEF_USE_GLOW_BUFFER     		= 0x0020,
	LIGHTDEF_USE_ALPHA_TEST		 		= 0x0040,
	LIGHTDEF_USE_CLOTH_BRDF				= 0x0080,
	LIGHTDEF_USE_SPEC_GLOSS				= 0x0100,

	LIGHTDEF_LIGHTTYPE_MASK      		= LIGHTDEF_USE_LIGHTMAP |
										  LIGHTDEF_USE_LIGHT_VECTOR |
										  LIGHTDEF_USE_LIGHT_VERTEX,

#ifdef REND2_SP
	LIGHTDEF_USE_VERTEX_ANIMATION		= 0x0200,
	LIGHTDEF_ALL						= 0x03FF,
#else
	LIGHTDEF_ALL						= 0x01FF,
#endif // REND2_SP

	LIGHTDEF_COUNT               		= LIGHTDEF_ALL + 1
};

enum
{
	SSDEF_FACE_CAMERA					= 0x01,
	SSDEF_ALPHA_TEST					= 0x02,
	SSDEF_FACE_UP						= 0x04,
	SSDEF_FX_SPRITE						= 0x08,
	SSDEF_USE_FOG						= 0x10,
	SSDEF_FOG_MODULATE					= 0x20,
	SSDEF_ADDITIVE						= 0x40,
	SSDEF_FLATTENED						= 0x80,

	SSDEF_ALL							= 0xFF,
	SSDEF_COUNT							= SSDEF_ALL + 1
};

enum
{
	GLSL_INT,
	GLSL_FLOAT,
	GLSL_VEC2,
	GLSL_VEC3,
	GLSL_VEC4,
	GLSL_MAT4x3,
	GLSL_MAT4x4,
};

enum uniformBlock_t
{
	UNIFORM_BLOCK_CAMERA,
	UNIFORM_BLOCK_SCENE,
	UNIFORM_BLOCK_LIGHTS,
	UNIFORM_BLOCK_FOGS,
	UNIFORM_BLOCK_ENTITY,
	UNIFORM_BLOCK_SHADER_INSTANCE,
	UNIFORM_BLOCK_BONES,
	UNIFORM_BLOCK_SURFACESPRITE,
	UNIFORM_BLOCK_COUNT
};

struct uniformBlockInfo_t
{
	int slot;
	const char *name;
	size_t size;
};
extern const uniformBlockInfo_t uniformBlocksInfo[UNIFORM_BLOCK_COUNT];

#define MAX_BLOCKS (32)
#define MAX_BLOCK_NAME_LEN (32)
struct Block
{
	const char *blockText;
	size_t blockTextLength;
	int blockTextFirstLine;

	const char *blockHeaderTitle;
	size_t blockHeaderTitleLength;

	const char *blockHeaderText;
	size_t blockHeaderTextLength;
};

enum GPUShaderType
{
	GPUSHADER_VERTEX,
	GPUSHADER_FRAGMENT,
	GPUSHADER_GEOMETRY,
	GPUSHADER_TYPE_COUNT
};

struct GPUShaderDesc
{
	GPUShaderType type;
	const char *source;
	int firstLineNumber;
};

struct GPUProgramDesc
{
	size_t numShaders;
	GPUShaderDesc *shaders;
};

typedef enum
{
	UNIFORM_DIFFUSEMAP = 0,
	UNIFORM_LIGHTMAP,
	UNIFORM_NORMALMAP,
	UNIFORM_DELUXEMAP,
	UNIFORM_SPECULARMAP,
	UNIFORM_SSAOMAP,

	UNIFORM_TEXTUREMAP,
	UNIFORM_LEVELSMAP,
	UNIFORM_CUBEMAP,
	UNIFORM_ENVBRDFMAP,

	UNIFORM_SCREENIMAGEMAP,
	UNIFORM_SCREENDEPTHMAP,

	UNIFORM_SHADOWMAP,
	UNIFORM_SHADOWMAP2,

	UNIFORM_SHADOWMVP,
	UNIFORM_SHADOWMVP2,
	UNIFORM_SHADOWMVP3,

	UNIFORM_ENABLETEXTURES,

	UNIFORM_DIFFUSETEXMATRIX,
	UNIFORM_DIFFUSETEXOFFTURB,

	UNIFORM_TCGEN0,
	UNIFORM_TCGEN0VECTOR0,
	UNIFORM_TCGEN0VECTOR1,
	UNIFORM_TCGEN1,

	UNIFORM_COLORGEN,
	UNIFORM_ALPHAGEN,
	UNIFORM_COLOR,
	UNIFORM_BASECOLOR,
	UNIFORM_VERTCOLOR,

	UNIFORM_DLIGHTINFO,
	UNIFORM_LIGHTFORWARD,
	UNIFORM_LIGHTUP,
	UNIFORM_LIGHTRIGHT,
	UNIFORM_LIGHTORIGIN,
	UNIFORM_MODELLIGHTDIR,
	UNIFORM_LIGHTRADIUS,
	UNIFORM_AMBIENTLIGHT,
	UNIFORM_DIRECTEDLIGHT,
	UNIFORM_DISINTEGRATION,
	UNIFORM_LIGHTMASK,
	UNIFORM_FOGINDEX,

	UNIFORM_FOGCOLORMASK,

	UNIFORM_MODELMATRIX,
	UNIFORM_MODELVIEWPROJECTIONMATRIX,

	UNIFORM_TIME,
	UNIFORM_VERTEXLERP,
	UNIFORM_NORMALSCALE,
	UNIFORM_SPECULARSCALE,
	UNIFORM_PARALLAXBIAS,

	UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
	UNIFORM_VIEWORIGIN,
	UNIFORM_LOCALVIEWORIGIN,
	UNIFORM_VIEWFORWARD,
	UNIFORM_VIEWLEFT,
	UNIFORM_VIEWUP,

	UNIFORM_INVTEXRES,
	UNIFORM_AUTOEXPOSUREMINMAX,
	UNIFORM_TONEMINAVGMAXLINEAR,

	UNIFORM_CUBEMAPINFO,

	UNIFORM_ALPHA_TEST_TYPE,

	UNIFORM_FX_VOLUMETRIC_BASE,
	UNIFORM_MAPZEXTENTS,
	UNIFORM_ZONEOFFSET,
	UNIFORM_ENVFORCE,
	UNIFORM_RANDOMOFFSET,
	UNIFORM_CHUNK_PARTICLES,

	UNIFORM_COUNT
} uniform_t;

struct UniformData
{
	uniform_t index;
	int numElements;

	// uniform data follows immediately afterwards
	//char data[1];
};

// shaderProgram_t represents a collection of GLSL shaders which form a
// GLSL shader program
typedef struct shaderProgram_s
{
	char *name;

	GLuint program;
	uint32_t attribs; // vertex array attributes
	uint32_t xfbVariables; // transform feedback variables

	// uniform parameters
	GLint *uniforms;
	short *uniformBufferOffsets;
	char  *uniformBuffer;

	// uniform blocks
	uint32_t uniformBlocks;
} shaderProgram_t;

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	stereoFrame_t	stereoFrame;

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0
	float		frameTime;			// delta last frame to frame now
	float		lastTime;			// last frame time

	float		blurFactor;

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			fistDrawSurf;
	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;

	int         num_pshadows;
	struct pshadow_s *pshadows;

	float       sunShadowMvp[3][16];
	float       sunDir[4];
	float       sunCol[4];
	float       sunAmbCol[4];
	float       colorScale;

	float       autoExposureMinMax[2];
	float       toneMinAvgMaxLinear[3];
} trRefdef_t;


//=================================================================================


typedef struct {
	int			originalBrushNumber;
	vec3_t		bounds[2];

	vec4_t		color;
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
} fog_t;

enum viewParmFlag_t {
	VPF_NOVIEWMODEL     = 0x01, // Don't render the view model
	VPF_DEPTHSHADOW     = 0x02, // Rendering depth-only
	VPF_DEPTHCLAMP      = 0x04, // Perform depth clamping when rendering z pass
	VPF_ORTHOGRAPHIC    = 0x08, // Use orthographic projection
	VPF_USESUNLIGHT     = 0x10,
	VPF_FARPLANEFRUSTUM = 0x20, // Use far clipping plane
	VPF_NOCUBEMAPS      = 0x40, // Don't render cubemaps
	VPF_POINTSHADOW		= 0x80,// Rendering pointlight shadow
	VPF_SHADOWCASCADES	= 0x100,// Rendering sun shadow cascades
	VPF_NOCLEAR			= 0x200,
};
using viewParmFlags_t = uint32_t;

enum viewParmType_t {
	VPT_SKYPORTAL,
	VPT_SUN_SHADOWS,
	VPT_PLAYER_SHADOWS,
	VPT_POINT_SHADOWS,
	VPT_PORTAL,
	VPT_MAIN,
	VPT_ALL
};

typedef struct {
	orientationr_t	ori;
	orientationr_t	world;
	vec3_t			pvsOrigin;			// may be different than or.origin for portals
	qboolean		isPortal;			// true if this view is through a portal
	qboolean		isMirror;			// the portal is a mirror, invert the face culling
	qboolean		isSkyPortal;
	int				flags;
	int				frameSceneNum;		// copied from tr.frameSceneNum
	int				frameCount;			// copied from tr.frameCount
	cplane_t		portalPlane;		// clip anything behind this if mirroring
	int				viewportX, viewportY, viewportWidth, viewportHeight;
	int				scissorX, scissorY, scissorWidth, scissorHeight;
	FBO_t			*targetFbo;
	int				targetFboLayer;
	float			fovX, fovY;
	float			projectionMatrix[16];
	cplane_t		frustum[5];
	vec3_t			visBounds[2];
	float			zFar;
	float			zNear;
	stereoFrame_t	stereoFrame;
	int				currentViewParm;
	viewParmType_t	viewParmType;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/
typedef byte color4ub_t[4];

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum surfaceType_e
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MDV,
	SF_MDR,
	SF_IQM,
	SF_MDX,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_VBO_MESH,
	SF_VBO_MDVMESH,
	SF_SPRITES,
	SF_WEATHER,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

/*
the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process
*/
#define	QSORT_CUBEMAP_SHIFT		0
#define QSORT_CUBEMAP_BITS		6
#define QSORT_CUBEMAP_MASK		((1 << QSORT_CUBEMAP_BITS) - 1)

#define QSORT_ENTITYNUM_SHIFT	(QSORT_CUBEMAP_SHIFT + QSORT_CUBEMAP_BITS)
#define QSORT_ENTITYNUM_BITS	REFENTITYNUM_BITS
#define QSORT_ENTITYNUM_MASK	((1 << QSORT_ENTITYNUM_BITS) - 1)

#define	QSORT_SHADERNUM_SHIFT	(QSORT_ENTITYNUM_SHIFT + QSORT_ENTITYNUM_BITS)
#define QSORT_SHADERNUM_BITS	SHADERNUM_BITS
#define QSORT_SHADERNUM_MASK	((1 << QSORT_SHADERNUM_BITS) - 1)

#define QSORT_POSTRENDER_SHIFT	(QSORT_SHADERNUM_SHIFT + QSORT_SHADERNUM_BITS)
#define QSORT_POSTRENDER_BITS	1
#define QSORT_POSTRENDER_MASK	((1 << QSORT_POSTRENDER_BITS) - 1)

#if QSORT_POSTRENDER_SHIFT >= 32
	#error "Sort field needs to be expanded"
#endif

typedef struct drawSurf_s {
	uint32_t sort; // bit combination for fast compares
	uint32_t dlightBits;
	surfaceType_t *surface; // any of surface*_t
	int fogIndex;
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	struct srfPoly_s *next;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;

typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
	shader_t		*shader;
	bool			portal_ranged;
} srfFlare_t;

struct vertexAttribute_t;
struct srfSprites_t
{
	surfaceType_t surfaceType;

	shader_t *shader;
	const surfaceSprite_t *sprite;
	int baseVertex;
	int numSprites;
	int numIndices;
	VBO_t *vbo;
	IBO_t *ibo;

	int fogIndex;
	AlphaTestType alphaTestType;

	int numAttributes;
	vertexAttribute_t *attributes;
};

struct srfWeather_t
{
	surfaceType_t surfaceType;
};

typedef struct
{
	vec3_t          xyz;
	vec2_t          st;
	vec2_t          lightmap[MAXLIGHTMAPS];
	vec3_t          normal;
	vec4_t          tangent;
	vec3_t          lightdir;
	vec4_t			vertexColors[MAXLIGHTMAPS];

#if DEBUG_OPTIMIZEVERTICES
	unsigned int    id;
#endif
} srfVert_t;

#ifdef _G2_GORE
typedef struct
{
	vec3_t			position;
	uint32_t		normal;
	vec2_t			texCoords;
	byte			bonerefs[4];
	byte			weights[4];
	uint32_t		tangents;
} g2GoreVert_t;

typedef struct srfG2GoreSurface_s
{
	surfaceType_t   surfaceType;

	// indexes
	int             numIndexes;
	glIndex_t      *indexes;

	// vertexes
	int             numVerts;
	g2GoreVert_t    *verts;

	// BSP VBO offsets
	int             firstVert;
	int             firstIndex;

} srfG2GoreSurface_t;
#endif

// srfBspSurface_t covers SF_GRID, SF_TRIANGLES, SF_POLY, and SF_VBO_MESH
typedef struct srfBspSurface_s
{
	surfaceType_t   surfaceType;

	// dynamic lighting information
	int				dlightBits;
	int             pshadowBits;

	// culling information
	vec3_t			cullBounds[2];
	vec3_t			cullOrigin;
	float			cullRadius;
	cplane_t        cullPlane;

	// indexes
	int             numIndexes;
	glIndex_t      *indexes;

	// vertexes
	int             numVerts;
	srfVert_t      *verts;

	// BSP VBO offsets
	int             firstVert;
	int             firstIndex;
	glIndex_t       minIndex;
	glIndex_t       maxIndex;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;

	// SF_GRID specific variables after here

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
} srfBspSurface_t;

// inter-quake-model
typedef struct {
	int		num_vertexes;
	int		num_triangles;
	int		num_frames;
	int		num_surfaces;
	int		num_joints;
	int		num_poses;
	struct srfIQModel_s	*surfaces;

	float		*positions;
	float		*texcoords;
	float		*normals;
	float		*tangents;
	byte		*blendIndexes;
	union {
		float	*f;
		byte	*b;
	} blendWeights;
	byte		*colors;
	int		*triangles;

	// depending upon the exporter, blend indices and weights might be int/float
	// as opposed to the recommended byte/byte, for example Noesis exports
	// int/float whereas the official IQM tool exports byte/byte
	byte blendWeightsType; // IQM_UBYTE or IQM_FLOAT

	int		*jointParents;
	float		*jointMats;
	float		*poseMats;
	float		*bounds;
	char		*names;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s {
	surfaceType_t	surfaceType;
	char		name[MAX_QPATH];
	shader_t	*shader;
	iqmData_t	*data;
	int		first_vertex, num_vertexes;
	int		first_triangle, num_triangles;
} srfIQModel_t;

typedef struct srfVBOMDVMesh_s
{
	surfaceType_t   surfaceType;

	struct mdvModel_s *mdvModel;
	struct mdvSurface_s *mdvSurface;

	// backEnd stats
	int				indexOffset;
	int             numIndexes;
	int             numVerts;
	glIndex_t       minIndex;
	glIndex_t       maxIndex;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;
} srfVBOMDVMesh_t;

extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

SHADOWS

==============================================================================
*/

typedef struct pshadow_s
{
	float sort;

	int    numEntities;
	int    entityNums[8];
	vec3_t entityOrigins[8];
	float  entityRadiuses[8];

	float viewRadius;
	vec3_t viewOrigin;

	vec3_t lightViewAxis[3];
	vec3_t lightOrigin;
	float  lightRadius;
	cplane_t cullPlane;
} pshadow_t;


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define CULLINFO_NONE   0
#define CULLINFO_BOX    1
#define CULLINFO_SPHERE 2
#define CULLINFO_PLANE  4

typedef struct cullinfo_s {
	int             type;
	vec3_t          bounds[2];
	vec3_t			localOrigin;
	float			radius;
	cplane_t        plane;
} cullinfo_t;

typedef struct msurface_s {
	struct shader_s		*shader;
	int					fogIndex;
	int                 cubemapIndex;
	cullinfo_t          cullinfo;

	int					numSurfaceSprites;
	srfSprites_t		*surfaceSprites;

	surfaceType_t		*data;			// any of srf*_t
} msurface_t;


#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int             visCounts[MAX_VISCOUNTS];	// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];

	// leaf specific
	int			cluster;
	int			area;

	int         firstmarksurface;
	int			nummarksurfaces;
} mnode_t;

typedef struct {
	vec3_t		bounds[2];		// for culling
	int			worldIndex;
	int			firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct
{
	byte		ambientLight[MAXLIGHTMAPS][3];
	byte		directLight[MAXLIGHTMAPS][3];
	byte		styles[MAXLIGHTMAPS];
	byte		latLong[2];
//	byte		pad[2];								// to align to a cache line
} mgrid_t;

typedef struct {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	int			numBModels;
	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int         numWorldSurfaces;

	int			numsurfaces;
	msurface_t	*surfaces;
	int         *surfacesViewCount;
	int         *surfacesDlightBits;
	int			*surfacesPshadowBits;

	int			numMergedSurfaces;
	msurface_t	*mergedSurfaces;
	int         *mergedSurfacesViewCount;
	int         *mergedSurfacesDlightBits;
	int			*mergedSurfacesPshadowBits;

	int			nummarksurfaces;
	int         *marksurfaces;
	int         *viewSurfaces;

	int			numfogs;
	fog_t		*fogs;
	const fog_t	*globalFog;
	int			globalFogIndex;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	float		*hdrLightGrid;
	int			lightGridOffsets[8];

	vec3_t		lightGridStep;

	mgrid_t		*lightGridData;
	word		*lightGridArray;
	int			numGridArrayElements;


	int			skyboxportal;
	int			numClusters;
	int			clusterBytes;
	const byte	*vis;			// may be passed in by CM_LoadMap to save space
	byte		*novis; // clusterBytes of 0xff (everything is visible)

	char		*entityString;
	char		*entityParsePoint;

} world_t;


/*
==============================================================================
MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
==============================================================================
*/
typedef struct
{
	float           bounds[2][3];
	float           localOrigin[3];
	float           radius;
} mdvFrame_t;

typedef struct
{
	float           origin[3];
	float           axis[3][3];
} mdvTag_t;

typedef struct
{
	char            name[MAX_QPATH];	// tag name
} mdvTagName_t;

typedef struct
{
	vec3_t          xyz;
	vec3_t          normal;
	vec3_t          tangent;
	vec3_t          bitangent;
} mdvVertex_t;

typedef struct
{
	float           st[2];
} mdvSt_t;

typedef struct mdvSurface_s
{
	surfaceType_t   surfaceType;

	char            name[MAX_QPATH];	// polyset name

	int             numShaderIndexes;
	int				*shaderIndexes;

	int             numVerts;
	mdvVertex_t    *verts;
	mdvSt_t        *st;

	int             numIndexes;
	glIndex_t      *indexes;

	struct mdvModel_s *model;
} mdvSurface_t;

typedef struct mdvModel_s
{
	int             numFrames;
	mdvFrame_t     *frames;

	int             numTags;
	mdvTag_t       *tags;
	mdvTagName_t   *tagNames;

	int             numSurfaces;
	mdvSurface_t   *surfaces;

	int             numVBOSurfaces;
	srfVBOMDVMesh_t  *vboSurfaces;

	int             numSkins;
} mdvModel_t;


//======================================================================

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MDR,
	MOD_IQM,
/*
Ghoul2 Insert Start
*/
   	MOD_MDXM,
	MOD_MDXA
/*
Ghoul2 Insert End
*/
} modtype_t;

typedef struct mdxmVBOMesh_s
{
	surfaceType_t surfaceType;

	int indexOffset;
	int minIndex;
	int maxIndex;
	int numIndexes;
	int numVertexes;

	VBO_t *vbo;
	IBO_t *ibo;
} mdxmVBOMesh_t;

typedef struct mdxmVBOModel_s
{
	int numVBOMeshes;
	mdxmVBOMesh_t *vboMeshes;

	VBO_t *vbo;
	IBO_t *ibo;
} mdxmVBOModel_t;

typedef struct mdxmData_s
{
	mdxmHeader_t *header;

	// int numLODs; // available in header->numLODs
	mdxmVBOModel_t *vboModels;
} mdxmData_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;		// model = tr.models[model->index]

	int			dataSize;	// just for listing purposes
	union
	{
		bmodel_t		*bmodel;			// type == MOD_BRUSH
		mdvModel_t		*mdv[MD3_MAX_LODS];	// type == MOD_MESH
		mdrHeader_t		*mdr;				// type == MOD_MDR
		iqmData_t		*iqm;				// type == MOD_IQM
		mdxmData_t		*glm;				// type == MOD_MDXM
		mdxaHeader_t	*gla;				// type == MOD_MDXA
	} data;

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);

model_t		*R_GetModelByHandle( qhandle_t hModel );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame,
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);

//====================================================

#define	MAX_DRAWIMAGES			2048
#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

extern	int gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)

struct vertexAttribute_t
{
	VBO_t *vbo;
	int index;
	int numComponents;
	GLboolean integerAttribute;
	GLenum type;
	GLboolean normalize;
	int stride;
	int offset;
	int stepRate;
};

#define MAX_UBO_BINDINGS (16)
struct bufferBinding_t
{
	GLuint buffer;
	int offset;
	int size;
};

// the renderer front end should never modify glstate_t
typedef struct glstate_s {
	int			currenttextures[NUM_TEXTURE_BUNDLES];
	int			currenttmu;
	int			texEnv[2];
	int			faceCulling;
	float		minDepth;
	float		maxDepth;
	uint32_t	glStateBits;
	uint32_t		vertexAttribsState;
	vertexAttribute_t currentVaoAttribs[ATTR_INDEX_MAX];
	uint32_t        vertexAttribsNewFrame;
	uint32_t        vertexAttribsOldFrame;
	float           vertexAttribsInterpolation;
	int				vertexAttribsTexCoordOffset[2];
	qboolean        vertexAnimation;
	qboolean		skeletalAnimation;
	qboolean		genShadows;
	shaderProgram_t *currentProgram;
	FBO_t          *currentFBO;
	VBO_t          *currentVBO;
	IBO_t          *currentIBO;
	bufferBinding_t currentXFBBO;
	GLuint			currentGlobalUBO;
	bufferBinding_t currentUBOs[MAX_UBO_BINDINGS];
	matrix_t        modelview;
	matrix_t        projection;
	matrix_t		modelviewProjection;
} glstate_t;

typedef enum {
	MI_NONE,
	MI_NVX,
	MI_ATI
} memInfo_t;

typedef enum {
	TCR_NONE = 0x0000,
	TCR_LATC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

typedef enum {
	IHV_UNKNOWN,

	IHV_NVIDIA,
	IHV_AMD,
	IHV_INTEL
} gpuIhv_t;

// We can't change glConfig_t without breaking DLL/vms compatibility, so
// store extensions we have here.
typedef struct {
	int glslMajorVersion;
	int glslMinorVersion;

	gpuIhv_t hardwareVendor;

	memInfo_t   memInfo;

	int maxRenderbufferSize;
	int maxColorAttachments;

	int textureCompression;
	int uniformBufferOffsetAlignment;
	int maxUniformBlockSize;
	int maxUniformBufferBindings;

	qboolean immutableTextures;
	qboolean immutableBuffers;

	qboolean debugContext;
	qboolean timerQuery;

	qboolean floatLightmap;
} glRefConfig_t;

enum
{
	TRI_BIN_0_19,
	TRI_BIN_20_49,
	TRI_BIN_50_99,
	TRI_BIN_100_299,
	TRI_BIN_300_599,
	TRI_BIN_600_999,
	TRI_BIN_1000_1499,
	TRI_BIN_1500_1999,
	TRI_BIN_2000_2999,
	TRI_BIN_3000_PLUS,

	NUM_TRI_BINS,
};

typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	int     c_surfBatches;
	float	c_overDraw;

	int		c_vboVertexBuffers;
	int		c_vboIndexBuffers;
	int		c_vboVertexes;
	int		c_vboIndexes;

	int     c_staticVboDraws;
	int     c_dynamicVboDraws;
	int		c_dynamicVboTotalSize;

	int     c_multidraws;
	int     c_multidrawsMerged;

	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int     c_glslShaderBinds;
	int     c_genericDraws;
	int     c_lightallDraws;
	int     c_fogDraws;
	int     c_dlightDraws;

	int		c_triangleCountBins[NUM_TRI_BINS];

	int		msec;			// total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	ori;
	backEndCounters_t	pc;
	trRefEntity_t	*currentEntity;
	qboolean	skyRenderedThisView;	// flag for drawing sun

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	float		color2D[4];
	trRefEntity_t	entity2D;		// currentEntity will point at this when doing 2D rendering
	trRefEntity_t	entityFlare;	// currentEntity will point at this when doing flare rendering

	FBO_t *last2DFBO;
	qboolean    colorMask[4];
	qboolean    framePostProcessed;
	qboolean    depthFill;
	qboolean    refractionFill;
} backEndState_t;

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
struct weatherSystem_t;
typedef struct trGlobals_s {
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	window_t				window;

	fileHandle_t			debugFile;
	int						numFramesToCapture;

	int						visIndex;
	int						visClusters[MAX_VISCOUNTS];
	int						visCounts[MAX_VISCOUNTS];	// incremented every time a new vis cluster is entered

	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	GLuint					globalVao;

	qboolean				worldMapLoaded;
	qboolean				worldInternalLightmapping; // qtrue indicates lightmap atlasing
	qboolean				worldDeluxeMapping;
	qboolean				worldInternalDeluxeMapping;
	vec2_t                  autoExposureMinMax;
	vec3_t                  toneMinAvgMaxLevel;
	world_t					*world;
	char					worldName[MAX_QPATH];

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xff
	image_t					*identityLightImage;	// full of tr.identityLightByte

	image_t					*renderImage;
	image_t					*glowImage;
	image_t					*glowImageScaled[6];
	image_t					*sunRaysImage;
	image_t					*renderDepthImage;
	image_t					*pshadowArrayImage;
	image_t					*textureScratchImage[2];
	image_t                 *quarterImage[2];
	image_t					*calcLevelsImage;
	image_t					*targetLevelsImage;
	image_t					*fixedLevelsImage;
	image_t					*sunShadowArrayImage;
	image_t					*pointShadowArrayImage;
	image_t                 *screenShadowImage;
	image_t                 *screenSsaoImage;
	image_t					*hdrDepthImage;
	image_t                 *renderCubeImage;
	image_t                 *renderCubeDepthImage;
	image_t					*envBrdfImage;
	image_t					*textureDepthImage;
	image_t					*weatherDepthImage;

	FBO_t					*renderFbo;
	FBO_t					*glowFboScaled[6];
	FBO_t					*msaaResolveFbo;
	FBO_t					*sunRaysFbo;
	FBO_t					*depthFbo;
	FBO_t					*pshadowFbos[MAX_DRAWN_PSHADOWS];
	FBO_t					*shadowCubeFbo[MAX_DLIGHTS*6];
	FBO_t					*textureScratchFbo[2];
	FBO_t                   *quarterFbo[2];
	FBO_t					*calcLevelsFbo;
	FBO_t					*targetLevelsFbo;
	FBO_t					*sunShadowFbo[3];
	FBO_t					*screenShadowFbo;
	FBO_t					*screenSsaoFbo;
	FBO_t					*hdrDepthFbo;
	FBO_t                   *renderCubeFbo[6];
	FBO_t                   *filterCubeFbo;
	FBO_t					*weatherDepthFbo;

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*distortionShader;
	shader_t				*projectionShadowShader;
	shader_t				*weatherInternalShader;

	shader_t				*flareShader;
	shader_t				*sunShader;
	shader_t				*sunFlareShader;

	int						numLightmaps;
	int						lightmapSize;
	image_t					**lightmaps;
	image_t					**deluxemaps;

	qboolean				hdrLighting;

	vec2i_t					lightmapAtlasSize;
	vec2i_t					lightmapsPerAtlasSide;

	int                     numCubemaps;
	cubemap_t               *cubemaps;

	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	model_t					*currentModel;

	weatherSystem_t			*weatherSystem;

	//
	// GPU shader programs
	//
	shaderProgram_t splashScreenShader;
	shaderProgram_t genericShader[GENERICDEF_COUNT];
	shaderProgram_t refractionShader[REFRACTIONDEF_COUNT];
	shaderProgram_t textureColorShader;
	shaderProgram_t fogShader[FOGDEF_COUNT];
	shaderProgram_t lightallShader[LIGHTDEF_COUNT];
	shaderProgram_t pshadowShader;
	shaderProgram_t volumeShadowShader;
	shaderProgram_t down4xShader;
	shaderProgram_t bokehShader;
	shaderProgram_t tonemapShader[2];
	shaderProgram_t calclevels4xShader[2];
	shaderProgram_t ssaoShader;
	shaderProgram_t depthBlurShader[2];
	shaderProgram_t testcubeShader;
	shaderProgram_t prefilterEnvMapShader;
	shaderProgram_t gaussianBlurShader[2];
	shaderProgram_t glowCompositeShader;
	shaderProgram_t dglowDownsample;
	shaderProgram_t dglowUpsample;
	shaderProgram_t spriteShader[SSDEF_COUNT];
	shaderProgram_t weatherUpdateShader;
	shaderProgram_t weatherShader;

	GLuint staticUbo;
	GLuint spriteUbos[MAX_SUB_BSP + 1];
	GLuint shaderInstanceUbo;
	int shaderInstanceUboWriteOffset;
	int entity2DUboOffset;
	int camera2DUboOffset;
	int entityFlareUboOffset;
	int cameraFlareUboOffset;
	int defaultLightsUboOffset;
	int defaultSceneUboOffset;
	int defaultFogsUboOffset;
	int defaultShaderInstanceUboOffset;

	int cameraUboOffsets[3 + MAX_DLIGHTS * 6 + 3 + MAX_DRAWN_PSHADOWS];
	int sceneUboOffset;
	int lightsUboOffset;
	int fogsUboOffset;
	int skyEntityUboOffset;
	int entityUboOffsets[REFENTITYNUM_WORLD + 1];
	int animationBoneUboOffset;

	// -----------------------------------------

	viewParms_t				viewParms;
	viewParms_t				cachedViewParms[3 + MAX_DLIGHTS * 6 + 3 + MAX_DRAWN_PSHADOWS];
	int						numCachedViewParms;

	viewParms_t				skyPortalParms;
	byte					skyPortalAreaMask[MAX_MAP_AREA_BYTES];
	int						skyPortalEntities;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			ori;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	float                   mapLightScale;
	float                   sunShadowScale;

	qboolean                sunShadows;
	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	int						numTimedBlocks;

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	world_t					*bspModels[MAX_SUB_BSP];
	int						numBspModels;

	int						numImages;
	image_t					*images;
	image_t					*imagesFreeList;

	int						numFBOs;
	FBO_t					*fbos[MAX_FBOS];

	int						numVBOs;
	VBO_t					*vbos[MAX_VBOS];

	int						numIBOs;
	IBO_t					*ibos[MAX_IBOS];

#ifdef _G2_GORE
	VBO_t					*goreVBO;
	int						goreVBOCurrentIndex;
	IBO_t					*goreIBO;
	int						goreIBOCurrentIndex;
#endif

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	GLuint					sunFlareQuery[2];
	int						sunFlareQueryIndex;
	qboolean				sunFlareQueryActive[2];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];

	float					rangedFog;
	float					distanceCull, distanceCullSquared; //rwwRMG - added

	// Specific to Jedi Academy
	int						numBSPModels;
	int						currentLevel;

	bool					explicitToneMap;
} trGlobals_t;

struct glconfigExt_t
{
	glconfig_t *glConfig;

	const char *originalExtensionString;
};

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init
extern glRefConfig_t glRefConfig;
extern window_t		window;

//
// cvars
//
extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_ignore;				// used for debugging anything
extern cvar_t	*r_verbose;				// used for verbose debug spew

extern cvar_t	*r_znear;				// near Z clip plane
extern cvar_t	*r_zproj;				// z distance of projection plane
extern cvar_t	*r_stereoSeparation;			// separation of cameras for stereo rendering

extern cvar_t	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern cvar_t	*r_lodbias;				// push/pull LOD transitions
extern cvar_t	*r_lodscale;

extern cvar_t	*r_inGameVideo;				// controls whether in game video should be draw
extern cvar_t	*r_fastsky;				// controls whether sky should be cleared or drawn
extern cvar_t	*r_drawSun;				// controls drawing of sun quad
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled

extern	cvar_t	*r_norefresh;			// bypasses the ref rendering
extern	cvar_t	*r_drawentities;		// disable/enable entity rendering
extern	cvar_t	*r_drawworld;			// disable/enable world rendering
extern	cvar_t	*r_speeds;				// various levels of information display
extern  cvar_t	*r_detailTextures;		// enables/disables detail texturing stages
extern	cvar_t	*r_novis;				// disable/enable usage of PVS
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	cvar_t	*r_nocurves;
extern	cvar_t	*r_showcluster;

extern cvar_t	*r_gamma;

extern  cvar_t  *r_ext_draw_range_elements;
extern  cvar_t  *r_ext_multi_draw_arrays;
extern  cvar_t  *r_ext_framebuffer_object;
extern  cvar_t  *r_ext_texture_float;
extern  cvar_t  *r_arb_half_float_pixel;
extern  cvar_t  *r_ext_framebuffer_multisample;
extern  cvar_t  *r_arb_seamless_cube_map;
extern  cvar_t  *r_arb_vertex_type_2_10_10_10_rev;
extern	cvar_t	*r_arb_buffer_storage;

extern	cvar_t	*r_nobind;						// turns off binding to appropriate textures
extern	cvar_t	*r_singleShader;				// make most world faces use default shader
extern	cvar_t	*r_roundImagesDown;
extern	cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern	cvar_t	*r_picmip;						// controls picmip values
extern	cvar_t	*r_finish;
extern	cvar_t	*r_textureMode;
extern	cvar_t	*r_offsetFactor;
extern	cvar_t	*r_offsetUnits;

extern	cvar_t	*r_fullbright;					// avoid lightmap pass
extern	cvar_t	*r_lightmap;					// render lightmaps only
extern	cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern	cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern	cvar_t	*r_logFile;						// number of frames to emit GL logs
extern	cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern	cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern	cvar_t	*r_shownormals;					// draws wireframe normals
extern	cvar_t	*r_clear;						// force screen clear every frame

extern	cvar_t	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	cvar_t	*r_flares;						// light flares

extern	cvar_t	*r_intensity;

extern	cvar_t	*r_lockpvs;
extern	cvar_t	*r_noportals;
extern	cvar_t	*r_portalOnly;

extern	cvar_t	*r_subdivisions;
extern	cvar_t	*r_lodCurveError;
extern	cvar_t	*r_skipBackEnd;

extern	cvar_t	*r_anaglyphMode;

extern  cvar_t  *r_mergeMultidraws;
extern  cvar_t  *r_mergeLeafSurfaces;

extern	cvar_t	*r_externalGLSL;

extern  cvar_t  *r_hdr;
extern  cvar_t  *r_floatLightmap;

extern  cvar_t  *r_toneMap;
extern  cvar_t  *r_forceToneMap;
extern  cvar_t  *r_forceToneMapMin;
extern  cvar_t  *r_forceToneMapAvg;
extern  cvar_t  *r_forceToneMapMax;

extern  cvar_t  *r_autoExposure;
extern  cvar_t  *r_forceAutoExposure;
extern  cvar_t  *r_forceAutoExposureMin;
extern  cvar_t  *r_forceAutoExposureMax;

extern  cvar_t  *r_cameraExposure;

extern  cvar_t  *r_depthPrepass;
extern  cvar_t  *r_ssao;

extern  cvar_t  *r_normalMapping;
extern  cvar_t  *r_specularMapping;
extern  cvar_t  *r_deluxeMapping;
extern  cvar_t  *r_parallaxMapping;
extern  cvar_t  *r_forceParallaxBias;
extern  cvar_t  *r_cubeMapping;
extern  cvar_t  *r_cubeMappingBounces;
extern  cvar_t  *r_baseNormalX;
extern  cvar_t  *r_baseNormalY;
extern  cvar_t  *r_baseParallax;
extern  cvar_t  *r_baseSpecular;
extern  cvar_t  *r_dlightMode;
extern  cvar_t  *r_pshadowDist;
extern  cvar_t  *r_recalcMD3Normals;
extern  cvar_t  *r_imageUpsample;
extern  cvar_t  *r_imageUpsampleMaxSize;
extern  cvar_t  *r_imageUpsampleType;
extern  cvar_t  *r_genNormalMaps;
extern  cvar_t  *r_forceSun;
extern  cvar_t  *r_forceSunMapLightScale;
extern  cvar_t  *r_forceSunLightScale;
extern  cvar_t  *r_forceSunAmbientScale;
extern  cvar_t  *r_sunlightMode;
extern  cvar_t  *r_drawSunRays;
extern  cvar_t  *r_sunShadows;
extern  cvar_t  *r_shadowFilter;
extern  cvar_t  *r_shadowMapSize;
extern  cvar_t  *r_shadowCascadeZNear;
extern  cvar_t  *r_shadowCascadeZFar;
extern  cvar_t  *r_shadowCascadeZBias;

extern	cvar_t	*r_greyscale;

extern	cvar_t	*r_ignoreGLErrors;

extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_simpleMipMaps;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;

extern cvar_t	*r_marksOnTriangleMeshes;

extern cvar_t	*r_dynamicGlow;
extern cvar_t	*r_dynamicGlowPasses;
extern cvar_t	*r_dynamicGlowDelta;
extern cvar_t	*r_dynamicGlowIntensity;
extern cvar_t	*r_dynamicGlowSoft;
extern cvar_t	*r_dynamicGlowWidth;
extern cvar_t	*r_dynamicGlowHeight;

extern cvar_t	*r_debugContext;
extern cvar_t	*r_debugWeather;

//====================================================================

struct packedVertex_t
{
	vec3_t position;
	uint32_t normal;
	uint32_t tangent;
	vec2_t texcoords[1 + MAXLIGHTMAPS];
	vec4_t colors[MAXLIGHTMAPS];
	uint32_t lightDirection;
};

struct packedTangentSpace_t
{
	vec4_t tangentAndSign;
};

void R_GenerateDrawSurfs( viewParms_t *viewParms, trRefdef_t *refdef );
void R_SetupViewParmsForOrthoRendering(
	int viewportWidth,
	int viewportHeight,
	FBO_t *fbo,
	viewParmFlags_t viewParmsFlags,
	const orientationr_t& orientation,
	const vec3_t viewBounds[2]);
void R_SortAndSubmitDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs );

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );
void R_RenderDlightCubemaps(const refdef_t *fd);
void R_SetupPshadowMaps(const refdef_t *fd);
void R_RenderCubemapSide( int cubemapIndex, int cubemapSide, bool bounce);
void R_GatherFrameViews(trRefdef_t *refdef);

void R_AddMD3Surfaces( trRefEntity_t *e, int entityNum );
void R_AddPolygonSurfaces( const trRefdef_t *refdef );

void R_DecomposeSort( uint32_t sort, int *entityNum, shader_t **shader, int *cubemap, int *postRender );
uint32_t R_CreateSortKey(int entityNum, int sortedShaderIndex, int cubemapIndex, int postRender);
void R_AddDrawSurf( surfaceType_t *surface, int entityNum, shader_t *shader,
				   int fogIndex, int dlightMap, int postRender, int cubemap );
bool R_IsPostRenderEntity ( const trRefEntity_t *refEntity );

void R_CalcMikkTSpaceBSPSurface(int numSurfaces, packedVertex_t *vertices, glIndex_t *indices);
void R_CalcMikkTSpaceMD3Surface(int numSurfaces, mdvVertex_t *verts, uint32_t *tangents, mdvSt_t *texcoords, glIndex_t *indices);
void R_CalcMikkTSpaceGlmSurface(int numSurfaces, mdxmVertex_t *vertices, mdxmVertexTexCoord_t *textureCoordinates, uint32_t *tangents, glIndex_t *indices);

void R_CalcTexDirs(vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2,
					const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3);
void R_CalcTbnFromNormalAndTexDirs(vec3_t tangent, vec3_t bitangent, vec3_t normal, vec3_t sdir, vec3_t tdir);
qboolean R_CalcTangentVectors(srfVert_t * dv[3]);

#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (const vec3_t local, vec3_t world);
void R_LocalPointToWorld (const vec3_t local, vec3_t world);
int R_CullBox (vec3_t bounds[2]);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadiusEx( const vec3_t origin, float radius, const cplane_t* frustum, int numPlanes );
int R_CullPointAndRadius( const vec3_t origin, float radius );
int R_CullLocalPointAndRadius( const vec3_t origin, float radius );

void R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum);
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *ori );
void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu );

/*
** GL wrapper/helper functions
*/
void	GL_Bind( image_t *image );
void	GL_BindToTMU( image_t *image, int tmu );
void	GL_SetDefaultState (void);
void	GL_SelectTexture( int unit );
void	GL_TextureMode( const char *string );
void	GL_CheckErrs( const char *file, int line );
#define GL_CheckErrors(...) GL_CheckErrs(__FILE__, __LINE__)
void	GL_State( uint32_t stateVector );
void    GL_SetProjectionMatrix(matrix_t matrix);
void    GL_SetModelviewMatrix(matrix_t matrix);
void	GL_Cull( int cullType );
void	GL_DepthRange( float min, float max );
void	GL_VertexAttribPointers(size_t numAttributes,
								vertexAttribute_t *attributes);
void	GL_DrawIndexed(GLenum primitiveType, int numIndices, GLenum indexType,
						int offset, int numInstances, int baseVertex);
void	GL_MultiDrawIndexed(GLenum primitiveType, int *numIndices,
							glIndex_t **offsets, int numDraws);
void	GL_Draw( GLenum primitiveType, int firstVertex, int numVertices, int numInstances );

#define LERP( a, b, w ) ( ( a ) * ( 1.0f - ( w ) ) + ( b ) * ( w ) )
#define LUMA( red, green, blue ) ( 0.2126f * ( red ) + 0.7152f * ( green ) + 0.0722f * ( blue ) )

extern glconfig_t  glConfig;
extern glconfigExt_t	glConfigExt;

typedef _skinSurface_t skinSurface_t;

void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_SetRangedFog ( float range );


void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterServerModel( const char *name );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterServerSkin( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void		RE_Shutdown(qboolean destroyWindow, qboolean restarting);
world_t		*R_LoadBSP(const char *name, int *bspIndex = nullptr);

qboolean	R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );
void		R_UpdateSubImage( image_t *image, byte *pic, int x, int y, int width, int height );

void		R_SetColorMappings( void );
void		R_GammaCorrect( byte *buffer, int bufSize );

void	R_ImageList_f( void );
void	R_SkinList_f( void );
void	R_FontList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd( const void *data );

void R_SaveScreenshot(struct screenshotReadback_t *screenshotReadback);

void	R_ScreenShotTGA_f( void );
void	R_ScreenShotPNG_f( void );
void	R_ScreenShotJPEG_f( void );

void	R_InitFogTable( void );
float	R_FogFactor( float s, float t );
void	R_InitImagesPool();
void	R_InitImages( void );
void	R_LoadHDRImage(const char *filename, byte **data, int *width, int *height);
void	R_DeleteTextures( void );
int		R_SumOfUsedImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );

int R_ComputeLOD( trRefEntity_t *ent );

const void *RB_TakeVideoFrameCmd( const void *data );
void RE_HunkClearCrap(void);

//
// tr_shader.c
//
extern const int lightmapsNone[MAXLIGHTMAPS];
extern const int lightmaps2d[MAXLIGHTMAPS];
extern const int lightmapsVertex[MAXLIGHTMAPS];
extern const int lightmapsFullBright[MAXLIGHTMAPS];
extern const byte stylesDefault[MAXLIGHTMAPS];

shader_t	*R_FindShader( const char *name, const int *lightmapIndexes, const byte *styles, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t *R_FindShaderByName( const char *name );
void		R_InitShaders( qboolean server );
void		R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);
shader_t *R_CreateShaderFromTextureBundle(
		const char *name,
		const textureBundle_t *bundle,
		uint32_t stateBits);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

QINLINE void GLimp_LogComment( char *comment ) {}
void GLimp_InitExtensions();
void GLimp_InitCoreFunctions();

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

typedef struct stageVars
{
	color4ub_t	colors[SHADER_MAX_VERTEXES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

#define MAX_MULTIDRAW_PRIMITIVES	16384

const int NUM_TESS_TEXCOORDS = 1 + MAXLIGHTMAPS;
struct shaderCommands_s
{
	glIndex_t	indexes[SHADER_MAX_INDEXES] QALIGN(16);
	vec4_t		xyz[SHADER_MAX_VERTEXES] QALIGN(16);
	uint32_t	normal[SHADER_MAX_VERTEXES] QALIGN(16);
	uint32_t	tangent[SHADER_MAX_VERTEXES] QALIGN(16);
	vec2_t		texCoords[SHADER_MAX_VERTEXES][NUM_TESS_TEXCOORDS] QALIGN(16);
	vec4_t		vertexColors[SHADER_MAX_VERTEXES] QALIGN(16);
	uint32_t    lightdir[SHADER_MAX_VERTEXES] QALIGN(16);
	//int			vertexDlightBits[SHADER_MAX_VERTEXES] QALIGN(16);

	IBO_t		*externalIBO;
	qboolean    useInternalVBO;

	stageVars_t	svars QALIGN(16);

	//color4ub_t	constantColor255[SHADER_MAX_VERTEXES] QALIGN(16);

	shader_t	*shader;
	float		shaderTime;
	int			fogNum;
	int         cubemapIndex;
#ifdef REND2_SP_MAYBE
	bool		scale;		// uses texCoords[input->firstIndex] for storage
	bool		fade;		// uses svars.colors[input->firstIndex] for storage
#endif
	int			dlightBits;	// or together of all vertexDlightBits
	int         pshadowBits;

	int			firstIndex;
	int			numIndexes;
	int			numVertexes;
	glIndex_t   minIndex;
	glIndex_t   maxIndex;

	int         multiDrawPrimitives;
	GLsizei     multiDrawNumIndexes[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t  *multiDrawFirstIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t  *multiDrawLastIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t   multiDrawMinIndex[MAX_MULTIDRAW_PRIMITIVES];
	glIndex_t   multiDrawMaxIndex[MAX_MULTIDRAW_PRIMITIVES];

	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;
};

struct drawState_t
{
	uint32_t stateBits;
};

#ifdef _WIN32
	typedef __declspec(align(16)) shaderCommands_s	shaderCommands_t;
#else
	typedef struct shaderCommands_s  shaderCommands_t;
#endif
extern	shaderCommands_t	tess;
extern	color4ub_t	styleColors[MAX_LIGHT_STYLES];

void RB_BeginSurface(shader_t *shader, int fogNum, int cubemapIndex );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void R_DrawElementsVBO( int numIndexes, glIndex_t firstIndex, glIndex_t minIndex, glIndex_t maxIndex );
void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 );
void RB_InstantQuad( vec4_t quadVerts[4] );
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4]);
void RB_InstantTriangle();

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/
world_t *R_GetWorld(int worldIndex);
void R_AddBrushModelSurfaces( trRefEntity_t *e, int entityNum );
void R_AddWorldSurfaces( viewParms_t *viewParms, trRefdef_t *refdef );
void R_MarkLeaves(void);
void R_RecursiveWorldNode(mnode_t *node, int planeBits, int dlightBits, int pshadowBits);
qboolean R_inPVS( const vec3_t p1, const vec3_t p2, byte *mask );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal );
void RB_AddDlightFlares( void );
void RB_RenderFlares (void);

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( bmodel_t *bmodel, trRefEntity_t *ent );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *ori );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
int R_LightDirForPoint( vec3_t point, vec3_t lightDir, vec3_t normal, world_t *world );
int R_DLightsForPoint(const vec3_t point, const float radius);
int R_CubemapForPoint( const vec3_t point );

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData( shaderCommands_t *shader );
void R_InitSkyTexCoords( float cloudLayerHeight );
void RB_DrawSun( float scale, shader_t *shader );
void RB_ClipSkyPolygons( shaderCommands_t *shader );

/*
============================================================

CURVE TESSELATION

============================================================
*/

srfBspSurface_t *R_SubdividePatchToGrid( int width, int height,
								srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfBspSurface_t *R_GridInsertColumn( srfBspSurface_t *grid, int column, int row, vec3_t point, float loderror );
srfBspSurface_t *R_GridInsertRow( srfBspSurface_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfBspSurface_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

VERTEX BUFFER OBJECTS

============================================================
*/

struct VertexArraysProperties
{
	size_t vertexDataSize;
	int numVertexArrays;

	int enabledAttributes[ATTR_INDEX_MAX];
	int offsets[ATTR_INDEX_MAX];
	int sizes[ATTR_INDEX_MAX];
	int strides[ATTR_INDEX_MAX];
	int streamStrides[ATTR_INDEX_MAX];
	void *streams[ATTR_INDEX_MAX];
};

uint32_t R_VboPackTangent(vec4_t v);
uint32_t R_VboPackNormal(vec3_t v);
void R_VboUnpackTangent(vec4_t v, uint32_t b);
void R_VboUnpackNormal(vec3_t v, uint32_t b);

VBO_t          *R_CreateVBO(byte * vertexes, int vertexesSize, vboUsage_t usage);
IBO_t          *R_CreateIBO(byte * indexes, int indexesSize, vboUsage_t usage);

void            R_BindVBO(VBO_t * vbo);
void            R_BindNullVBO(void);

void            R_BindIBO(IBO_t * ibo);
void            R_BindNullIBO(void);

void			R_InitGPUBuffers(void);
void            R_DestroyGPUBuffers(void);
void            R_VBOList_f(void);

void            RB_UpdateVBOs(unsigned int attribBits);
#ifdef _G2_GORE
void			RB_UpdateGoreVBO(srfG2GoreSurface_t *goreSurface);
#endif
void			RB_CommitInternalBufferData();

void			RB_BindUniformBlock(GLuint ubo, uniformBlock_t block, int offset);
int				RB_BindAndUpdateFrameUniformBlock(uniformBlock_t block, void *data);
void			RB_AddShaderToShaderInstanceUBO(shader_t *shader);
int				RB_AddShaderInstanceBlock(void *data);
void			RB_UpdateConstants(const trRefdef_t *refdef);
void			RB_BeginConstantsUpdate(struct gpuFrame_t *frame);
void			RB_EndConstantsUpdate(const struct gpuFrame_t *frame);
int				RB_AppendConstantsData(struct gpuFrame_t *frame, const void *data, size_t dataSize);
void			CalculateVertexArraysProperties(uint32_t attributes, VertexArraysProperties *properties);
void			CalculateVertexArraysFromVBO(uint32_t attributes, const VBO_t *vbo, VertexArraysProperties *properties);

/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd(shaderCommands_t *input, const VertexArraysProperties *vertexArrays);
void RB_ShadowFinish(void);
void RB_ProjectionShadowDeform(void);

/*
============================================================

GLSL

============================================================
*/

void GLSL_InitSplashScreenShader();
void GLSL_LoadGPUShaders();
void GLSL_ShutdownGPUShaders(void);
void GLSL_VertexAttribsState(uint32_t stateBits, VertexArraysProperties *vertexArrays);
void GLSL_VertexAttribPointers(const VertexArraysProperties *vertexArrays);
void GL_VertexArraysToAttribs( vertexAttribute_t *attribs,
	size_t attribsCount, const VertexArraysProperties *vertexArrays );
void GLSL_BindProgram(shaderProgram_t * program);
void GLSL_BindNullProgram(void);

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value);
void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value);
void GLSL_SetUniformFloatN(shaderProgram_t *program, int uniformNum, const float *v, int numFloats);
void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v);
void GLSL_SetUniformVec2N(shaderProgram_t *program, int uniformNum, const float *v, int numVec2s);
void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v);
void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v);
void GLSL_SetUniformMatrix4x3(shaderProgram_t *program, int uniformNum, const float *matrix, int numElements = 1);
void GLSL_SetUniformMatrix4x4(shaderProgram_t *program, int uniformNum, const float *matrix, int numElements = 1);
void GLSL_SetUniforms( shaderProgram_t *program, UniformData *uniformData );

shaderProgram_t *GLSL_GetGenericShaderProgram(int stage);

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitNextFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddMiniRefEntityToScene( const miniRefEntity_t *miniRefEnt );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_BeginScene( const refdef_t *fd );
void RE_RenderScene( const refdef_t *fd );
void RE_EndScene( void );

/*
=============================================================

UNCOMPRESSING BONES

=============================================================
*/

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

void MC_UnCompress(float mat[3][4],const unsigned char * comp);

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MDRAddAnimSurfaces( trRefEntity_t *ent, int entityNum );
void RB_MDRSurfaceAnim( mdrSurface_t *surface );
qboolean R_LoadIQM (model_t *mod, void *buffer, int filesize, const char *name );
void R_AddIQMSurfaces( trRefEntity_t *ent, int entityNum );
void RB_IQMSurfaceAnim( surfaceType_t *surface );
int R_IQMLerpTag( orientation_t *tag, iqmData_t *data,
                  int startFrame, int endFrame,
                  float frac, const char *tagName );

/*
Ghoul2 Insert Start
*/
#ifdef _MSC_VER
#pragma warning (disable: 4512)	//default assignment operator could not be gened
#endif
class CRenderableSurface
{
public:
	// ident of this surface - required so the materials renderer knows what
	// sort of surface this refers to
	int ident;

	CBoneCache *boneCache;
	mdxmVBOMesh_t *vboMesh;

	// tell the renderer to render shadows for this surface
	qboolean genShadows;
	int dlightBits;
	int pshadowBits;

	// pointer to surface data loaded into file - only used by client renderer
	// DO NOT USE IN GAME SIDE - if there is a vid restart this will be out of
	// wack on the game
	mdxmSurface_t *surfaceData;

#ifdef _G2_GORE
	// alternate texture coordinates
	srfG2GoreSurface_t *alternateTex;
	void *goreChain;

	float scale;
	float fade;

	// this is a number between 0 and 1 that dictates the progression of the
	// bullet impact
	float impactTime;
#endif

	CRenderableSurface& operator =( const CRenderableSurface& src )
	{
		ident = src.ident;
		boneCache = src.boneCache;
		surfaceData = src.surfaceData;
#ifdef _G2_GORE
		alternateTex = src.alternateTex;
		goreChain = src.goreChain;
#endif
		vboMesh = src.vboMesh;

		return *this;
	}

	CRenderableSurface()
		: ident(SF_MDX)
		, boneCache(nullptr)
		, vboMesh(nullptr)
		, surfaceData(nullptr)
#ifdef _G2_GORE
		, alternateTex(nullptr)
		, goreChain(nullptr)
		, scale(1.0f)
		, fade(0.0f)
		, impactTime(0.0f)
#endif
	{
	}

	void Init()
	{
		ident = SF_MDX;
		boneCache = nullptr;
		surfaceData = nullptr;
#ifdef _G2_GORE
		alternateTex = nullptr;
		goreChain = nullptr;
#endif
		vboMesh = nullptr;
		genShadows = qfalse;
	}
};

void R_AddGhoulSurfaces( trRefEntity_t *ent, int entityNum );
void RB_SurfaceGhoul( CRenderableSurface *surf );
void RB_TransformBones(const trRefEntity_t *ent, const trRefdef_t *refdef, int currentFrameNum, gpuFrame_t *frame);
int RB_GetBoneUboOffset(CRenderableSurface *surf);
void RB_SetBoneUboOffset(CRenderableSurface *surf, int offset, int currentFrameNum);
void RB_FillBoneBlock(CRenderableSurface *surf, mat3x4_t *outMatrices);
/*
Ghoul2 Insert End
*/

/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelViewMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcFogTexCoords( float *dstTexCoords );

void	RB_CalcScaleTexMatrix( const float scale[2], float *matrix );
void	RB_CalcScrollTexMatrix( const float scrollSpeed[2], float *matrix );
void	RB_CalcRotateTexMatrix( float degsPerSecond, float *matrix );
void	RB_CalcTurbulentFactors( const waveForm_t *wf, float *amplitude, float *now );
void	RB_CalcTransformTexMatrix( const texModInfo_t *tmi, float *matrix  );
void	RB_CalcStretchTexMatrix( const waveForm_t *wf, float *matrix );

void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
float	RB_CalcWaveAlphaSingle( const waveForm_t *wf );
float	RB_CalcWaveColorSingle( const waveForm_t *wf );

/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_ExecuteRenderCommands( const void *data );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define	MAX_RENDER_COMMANDS	0x80000

typedef struct renderCommandList_s {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct setColorCommand_s {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct drawBufferCommand_s {
	int		commandId;
	int		buffer;
} drawBufferCommand_t;

typedef struct subImageCommand_s {
	int		commandId;
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct swapBuffersCommand_s {
	int		commandId;
} swapBuffersCommand_t;

typedef struct endFrameCommand_s {
	int		commandId;
	int		buffer;
} endFrameCommand_t;

typedef struct stretchPicCommand_s {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct rotatePicCommand_s {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
	float	a;
} rotatePicCommand_t;

typedef struct drawSurfsCommand_s {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef enum {
	SSF_JPEG,
	SSF_TGA,
	SSF_PNG
} screenshotFormat_t;

typedef struct screenShotCommand_s {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	screenshotFormat_t format;
} screenshotCommand_t;

typedef struct videoFrameCommand_s {
	int						commandId;
	int						width;
	int						height;
	byte					*captureBuffer;
	byte					*encodeBuffer;
	qboolean			motionJpeg;
} videoFrameCommand_t;

typedef struct colorMaskCommand_s {
	int commandId;

	GLboolean rgba[4];
} colorMaskCommand_t;

typedef struct clearDepthCommand_s {
	int commandId;
} clearDepthCommand_t;

typedef struct convolveCubemapCommand_s {
	int			commandId;
	cubemap_t	*cubemap;
	int			cubemapId;
} convolveCubemapCommand_t;

typedef struct postProcessCommand_s {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
} postProcessCommand_t;

typedef struct beginTimedBlockCommand_s {
	int commandId;
	qhandle_t timerHandle;
	const char *name;
} beginTimedBlockCommand_t;

typedef struct endTimedBlockCommand_s {
	int commandId;
	qhandle_t timerHandle;
} endTimedBlockCommand_t;

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_ROTATE_PIC,
	RC_ROTATE_PIC2,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT,
	RC_VIDEOFRAME,
	RC_COLORMASK,
	RC_CLEARDEPTH,
	RC_CONVOLVECUBEMAP,
	RC_POSTPROCESS,
	RC_BEGIN_TIMED_BLOCK,
	RC_END_TIMED_BLOCK
} renderCommand_t;

struct gpuTimer_t
{
	const char *name;
	GLuint queryName;
};

struct gpuTimedBlock_t
{
	const char *name;
	GLuint beginTimer;
	GLuint endTimer;
};

struct screenshotReadback_t
{
	GLuint pbo;
	int strideInBytes;
	int rowInBytes;
	int width;
	int height;
	screenshotFormat_t format;
	char filename[MAX_QPATH];
};

#define MAX_GPU_TIMERS (512)
struct gpuFrame_t
{
	GLsync sync;
	GLuint ubo;
	size_t uboWriteOffset;
	size_t uboSize;
	size_t uboMapBase;
	void *uboMemory;

	screenshotReadback_t screenshotReadback;

	VBO_t *dynamicVbo;
	void *dynamicVboMemory;
	size_t dynamicVboWriteOffset;
	size_t dynamicVboCommitOffset;

	IBO_t *dynamicIbo;
	void *dynamicIboMemory;
	size_t dynamicIboWriteOffset;
	size_t dynamicIboCommitOffset;

	int numTimers;
	int numTimedBlocks;

	gpuTimer_t timers[MAX_GPU_TIMERS];
	gpuTimedBlock_t timedBlocks[MAX_GPU_TIMERS / 2]; // Each block will need 2 timer queries.
};

// all of the information needed by the back end must be
// contained in a backEndData_t.
#define MAX_FRAMES (2)
#define PER_FRAME_MEMORY_BYTES (32 * 1024 * 1024)
class Allocator;
struct Pass;
typedef struct backEndData_s {
	unsigned realFrameNumber;
	gpuFrame_t frames[MAX_FRAMES];
	gpuFrame_t *currentFrame;
	Allocator *perFrameMemory;
	Pass *currentPass;

	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_REFENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	pshadow_t pshadows[MAX_CALC_PSHADOWS];
	renderCommandList_t	commands;
} backEndData_t;

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData;


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_IssuePendingRenderCommands( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );
void R_AddConvolveCubemapCmd(cubemap_t *cubemap, int cubemapId);
void R_AddPostProcessCmd (void);
qhandle_t R_BeginTimedBlockCmd( const char *name );
void R_EndTimedBlockCmd( qhandle_t timerHandle );


void RE_SetColor( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_RotatePic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a, qhandle_t hShader );
void RE_RotatePic2 ( float x, float y, float w, float h, float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void R_NewFrameSync();
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

// tr_ghoul2.cpp
void Mat3x4_Multiply(mdxaBone_t *out, const mdxaBone_t *in2, const mdxaBone_t *in);
void Mat3x4_Scale( mdxaBone_t *result, const mdxaBone_t *lhs, const float scale );
void Mat3x4_Lerp(
	mdxaBone_t *result,
	const mdxaBone_t *lhs,
	const mdxaBone_t *rhs,
	const float t );
const mdxaBone_t operator +( const mdxaBone_t& lhs, const mdxaBone_t& rhs );
const mdxaBone_t operator -( const mdxaBone_t& lhs, const mdxaBone_t& rhs );
const mdxaBone_t operator *( const mdxaBone_t& lhs, const mdxaBone_t& rhs );
const mdxaBone_t operator *( const mdxaBone_t& lhs, const float scale );
const mdxaBone_t operator *( const float scale, const mdxaBone_t& rhs );

qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *name, qboolean &bAlreadyCached );
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *name, qboolean &bAlreadyCached );
void RE_InsertModelIntoHash( const char *name, model_t *mod );
void ResetGhoul2RenderableSurfaceHeap();

void R_InitDecals( void );
void RE_ClearDecals( void );
void RE_AddDecalToScene ( qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary );
void R_AddDecals( void );

image_t	*R_FindImageFile( const char *name, imgType_t type, int flags );
void R_LoadPackedMaterialImage(shaderStage_t *stage, const char *packedImageName, int flags);
image_t *R_BuildSDRSpecGlossImage(shaderStage_t *stage, const char *specImageName, int flags);
qhandle_t RE_RegisterShader( const char *name );
qhandle_t RE_RegisterShaderNoMip( const char *name );
const char		*RE_ShaderNameFromIndex(int index);
image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgType_t type, int flags, int internalFormat );

float ProjectRadius( float r, vec3_t location );
void RE_RegisterModels_StoreShaderRequest(const char *psModelFileName, const char *psShaderName, int *piShaderIndexPoke);
qboolean ShaderHashTableExists(void);
void R_ImageLoader_Init(void);

class Allocator;
GPUProgramDesc ParseProgramSource( Allocator& allocator, const char *text );

struct DepthRange
{
	float minDepth;
	float maxDepth;
};

struct SamplerBinding
{
	image_t *image;
	qhandle_t videoMapHandle;
	uint8_t slot;
};

struct UniformBlockBinding
{
	GLuint ubo;
	int offset;
	uniformBlock_t block;
};

enum DrawCommandType
{
	DRAW_COMMAND_MULTI_INDEXED,
	DRAW_COMMAND_INDEXED,
	DRAW_COMMAND_ARRAYS
};

struct DrawCommand
{
	DrawCommandType type;
	GLenum primitiveType;
	int numInstances;

	union DrawParams
	{
		struct MultiDrawIndexed
		{
			int numDraws;
			GLsizei *numIndices;
			glIndex_t **firstIndices;
		} multiIndexed;

		struct DrawIndexed
		{
			GLenum indexType;
			GLsizei numIndices;
			glIndex_t firstIndex;
			glIndex_t baseVertex;
		} indexed;

		struct DrawArrays
		{
			glIndex_t firstVertex;
			GLsizei numVertices;
		} arrays;
	} params;
};

struct RenderState
{
	DepthRange depthRange;
	uint32_t stateBits;
	uint32_t cullType; // this is stupid

	bool transformFeedback;
};

struct DrawItem
{
	RenderState renderState;

	IBO_t *ibo;
	shaderProgram_t *program;

	uint32_t numAttributes;
	vertexAttribute_t *attributes;

	uint32_t numSamplerBindings;
	SamplerBinding *samplerBindings;

	uint32_t numUniformBlockBindings;
	UniformBlockBinding *uniformBlockBindings;

	bufferBinding_t transformFeedbackBuffer;

	UniformData *uniformData;

	DrawCommand draw;
};

void DrawItemSetSamplerBindings(
	DrawItem& drawItem,
	const SamplerBinding *bindings,
	uint32_t count,
	Allocator& allocator);
void DrawItemSetUniformBlockBindings(
	DrawItem& drawItem,
	const UniformBlockBinding *bindings,
	uint32_t count,
	Allocator& allocator);
void DrawItemSetVertexAttributes(
	DrawItem& drawItem,
	const vertexAttribute_t *attributes,
	uint32_t count,
	Allocator& allocator);

template<int N>
void DrawItemSetUniformBlockBindings(
	DrawItem& drawItem,
	const UniformBlockBinding(&bindings)[N],
	Allocator& allocator)
{
	DrawItemSetUniformBlockBindings(drawItem, &bindings[0], N, allocator);
}

class UniformDataWriter
{
public:
	UniformDataWriter();

	void Start( shaderProgram_t *sp );

	UniformDataWriter& SetUniformInt( uniform_t uniform, int value );
	UniformDataWriter& SetUniformFloat( uniform_t uniform, float value );
	UniformDataWriter& SetUniformFloat( uniform_t uniform, float *values, size_t count );

	UniformDataWriter& SetUniformVec2( uniform_t uniform, float x, float y );
	UniformDataWriter& SetUniformVec2( uniform_t uniform, const float *values, size_t count = 1 );

	UniformDataWriter& SetUniformVec3( uniform_t uniform, float x, float y, float z );
	UniformDataWriter& SetUniformVec3( uniform_t uniform, const float *values, size_t count = 1 );

	UniformDataWriter& SetUniformVec4( uniform_t uniform, float x, float y, float z, float w );
	UniformDataWriter& SetUniformVec4( uniform_t uniform, const float *values, size_t count = 1 );

	UniformDataWriter& SetUniformMatrix4x3( uniform_t uniform, const float *matrix, size_t count = 1 );
	UniformDataWriter& SetUniformMatrix4x4( uniform_t uniform, const float *matrix, size_t count = 1 );

	UniformData *Finish( Allocator& destHeap );

private:
	bool failed;
	shaderProgram_t *shaderProgram;
	char scratchBuffer[2048];
	Allocator scratch;
};

class SamplerBindingsWriter
{
public:
	SamplerBindingsWriter();

	SamplerBindingsWriter( const SamplerBindingsWriter& ) = delete;
	SamplerBindingsWriter& operator=( const SamplerBindingsWriter& ) = delete;

	SamplerBindingsWriter& AddStaticImage( image_t *image, int unit );

	SamplerBindingsWriter& AddAnimatedImage( textureBundle_t *bundle, int unit );

	SamplerBinding *Finish( Allocator& destHeap, uint32_t* numBindings );

private:
	SamplerBinding scratch[32];
	bool failed;
	int count;
};

void RB_FillDrawCommand(
	DrawCommand& drawCmd,
	GLenum primitiveType,
	int numInstances,
	const shaderCommands_t *input
);

uint32_t RB_CreateSortKey( const DrawItem& item, int stage, int layer );
void RB_AddDrawItem( Pass *pass, uint32_t sortKey, const DrawItem& drawItem );
DepthRange RB_GetDepthRange( const trRefEntity_t *re, const shader_t *shader );

#endif //TR_LOCAL_H
