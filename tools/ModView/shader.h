// Filename:-	shader.h
//
// stuff specific to shader files only



#ifndef SHADER_H
#define SHADER_H

#include "R_Common.h"


void ScanAndLoadShaderFiles( void );
void KillAllShaderFiles(void);

const char *R_FindShader( const char *psLocalMaterialName);

#if 0

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define		GLS_SRCBLEND_BITS					0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define		GLS_DSTBLEND_BITS					0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define		GLS_ATEST_BITS						0x70000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE

#endif

typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;					// gl texture binding

	struct image_s*	next;
} image_t;


struct shaderCommands_s;



typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension

	int			index;					// this shader == tr.shaders[index]

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	struct	shader_s	*next;
} shader_t;



typedef byte color4ub_t[4];


typedef unsigned int glIndex_t;

typedef struct shaderCommands_s 
{
	glIndex_t	indexes[ACTUAL_SHADER_MAX_INDEXES];
	vec4_t		xyz[ACTUAL_SHADER_MAX_VERTEXES];
	vec4_t		normal[ACTUAL_SHADER_MAX_VERTEXES];
	vec2_t		texCoords[ACTUAL_SHADER_MAX_VERTEXES][2];
	int			WeightsUsed[ACTUAL_SHADER_MAX_VERTEXES];	// new for MODVIEW, shows vert weighting-count
	int			WeightsOmitted[ACTUAL_SHADER_MAX_VERTEXES];	// new for MODVIEW
/*	color4ub_t	vertexColors[ACTUAL_SHADER_MAX_VERTEXES];
	int			vertexDlightBits[ACTUAL_SHADER_MAX_VERTEXES];

	stageVars_t	svars;

	color4ub_t	constantColor255[ACTUAL_SHADER_MAX_VERTEXES];
*/
//////////////	shader_t	*shader;
	GLuint		gluiTextureBind;
/*
	int			fogNum;

	int			dlightBits;	// or together of all vertexDlightBits
*/
	int			numIndexes;
	int			numVertexes;
/*
	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;
*/

	// some MODVIEW crap just for ModView surface highlighting (and now surface bolting)...
	//
	ModelHandle_t hModel;
	int			iSurfaceNum;	
	bool		bSurfaceIsG2Tag;
	refEntity_t *pRefEnt;	// uberhack alert!

} shaderCommands_t;

extern	shaderCommands_t	tess;
/*shader_t *R_FindShader( const char *name );
shader_t *R_GetShaderByHandle( qhandle_t hShader );
*/

#endif	// #ifndef SHADER_H

///////////////// eof //////////////
