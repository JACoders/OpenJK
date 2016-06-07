/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// tr_arb.c -- this file deals with the arb shaders for dynamic glow

#include "tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex and Pixel Shader definitions.	- AReis
/***********************************************************************************************************/
// This vertex shader basically passes through most values and calculates no lighting. The only
// unusual thing it does is add the inputed texel offsets to all four texture units (this allows
// nearest neighbor pixel peeking).
const unsigned char g_strGlowVShaderARB[] =
{
	"!!ARBvp1.0\
	\
	# Input.\n\
	ATTRIB	iPos		= vertex.position;\
	ATTRIB	iColor		= vertex.color;\
	ATTRIB	iTex0		= vertex.texcoord[0];\
	ATTRIB	iTex1		= vertex.texcoord[1];\
	ATTRIB	iTex2		= vertex.texcoord[2];\
	ATTRIB	iTex3		= vertex.texcoord[3];\
	\
	# Output.\n\
	OUTPUT	oPos		= result.position;\
	OUTPUT	oColor		= result.color;\
	OUTPUT	oTex0		= result.texcoord[0];\
	OUTPUT	oTex1		= result.texcoord[1];\
	OUTPUT	oTex2		= result.texcoord[2];\
	OUTPUT	oTex3		= result.texcoord[3];\
	\
	# Constants.\n\
	PARAM	ModelViewProj[4]= { state.matrix.mvp };\
	PARAM	TexelOffset0	= program.env[0];\
	PARAM	TexelOffset1	= program.env[1];\
	PARAM	TexelOffset2	= program.env[2];\
	PARAM	TexelOffset3	= program.env[3];\
	\
	# Main.\n\
	DP4		oPos.x, ModelViewProj[0], iPos;\
	DP4		oPos.y, ModelViewProj[1], iPos;\
	DP4		oPos.z, ModelViewProj[2], iPos;\
	DP4		oPos.w, ModelViewProj[3], iPos;\
	MOV		oColor, iColor;\
	# Notice the optimization of using one texture coord instead of all four.\n\
	ADD		oTex0, iTex0, TexelOffset0;\
	ADD		oTex1, iTex0, TexelOffset1;\
	ADD		oTex2, iTex0, TexelOffset2;\
	ADD		oTex3, iTex0, TexelOffset3;\
	\
	END"
};

// This Pixel Shader loads four texture units and adds them all together (with a modifier
// multiplied to each in the process). The final output is r0 = t0 + t1 + t2 + t3.
const unsigned char g_strGlowPShaderARB[] =
{
	"!!ARBfp1.0\
	\
	# Input.\n\
	ATTRIB	iColor	= fragment.color.primary;\
	\
	# Output.\n\
	OUTPUT	oColor	= result.color;\
	\
	# Constants.\n\
	PARAM	Weight	= program.env[0];\
	TEMP	t0;\
	TEMP	t1;\
	TEMP	t2;\
	TEMP	t3;\
	TEMP	r0;\
	\
	# Main.\n\
	TEX		t0, fragment.texcoord[0], texture[0], RECT;\
	TEX		t1, fragment.texcoord[1], texture[1], RECT;\
	TEX		t2, fragment.texcoord[2], texture[2], RECT;\
	TEX		t3, fragment.texcoord[3], texture[3], RECT;\
	\
    MUL		r0, t0, Weight;\
	MAD		r0, t1, Weight, r0;\
	MAD		r0, t2, Weight, r0;\
	MAD		r0, t3, Weight, r0;\
	\
	MOV		oColor, r0;\
	\
	END"
};

static const char *gammaCorrectVtxShader =
"!!ARBvp1.0\
MOV result.position, vertex.position;\
MOV result.texcoord[0], vertex.texcoord[0];\
END";

static const char *gammaCorrectPxShader =
"!!ARBfp1.0\
TEMP r0;\
TEX r0, fragment.texcoord[0], texture[0], RECT;\
TEX result.color, r0, texture[1], 3D;\
END";
/***********************************************************************************************************/

#define GL_PROGRAM_ERROR_STRING_ARB						0x8874
#define GL_PROGRAM_ERROR_POSITION_ARB					0x864B

void ARB_InitGPUShaders(void) {
	if ( !qglGenProgramsARB )
	{
		return;
	}

	// Allocate and Load the global 'Glow' Vertex Program. - AReis
	qglGenProgramsARB( 1, &tr.glowVShader );
	qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, tr.glowVShader );
	qglProgramStringARB( GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, ( GLsizei ) strlen( ( char * ) g_strGlowVShaderARB ), g_strGlowVShaderARB );

//	const GLubyte *strErr = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
	int iErrPos = 0;
	qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &iErrPos );
	assert( iErrPos == -1 );

	// NOTE: I make an assumption here. If you have (current) nvidia hardware, you obviously support register combiners instead of fragment
	// programs, so use those. The problem with this is that nv30 WILL support fragment shaders, breaking this logic. The good thing is that
	// if you always ask for regcoms before fragment shaders, you'll always just use regcoms (problem solved... for now). - AReis

	// Load Pixel Shaders (either regcoms or fragprogs).
	if ( qglCombinerParameteriNV )
	{
		// The purpose of this regcom is to blend all the pixels together from the 4 texture units, but with their
		// texture coordinates offset by 1 (or more) texels, effectively letting us blend adjoining pixels. The weight is
		// used to either strengthen or weaken the pixel intensity. The more it diffuses (the higher the radius of the glow),
		// the higher the intensity should be for a noticable effect.
		// Regcom result is: ( tex1 * fBlurWeight ) + ( tex2 * fBlurWeight ) + ( tex2 * fBlurWeight ) + ( tex2 * fBlurWeight )

		// VV guys, this is the pixel shader you would use instead :-)
		/*
		// c0 is the blur weight.
		ps 1.1
		tex		t0
		tex		t1
		tex		t2
		tex		t3

		mul		r0, c0, t0;
		madd	r0, c0, t1, r0;
		madd	r0, c0, t2, r0;
		madd	r0, c0, t3, r0;
		*/
		tr.glowPShader = qglGenLists( 1 );
		qglNewList( tr.glowPShader, GL_COMPILE );
			qglCombinerParameteriNV( GL_NUM_GENERAL_COMBINERS_NV, 2 );

			// spare0 = fBlend * tex0 + fBlend * tex1.
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerOutputNV( GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

			// spare1 = fBlend * tex2 + fBlend * tex3.
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE2_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE3_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerInputNV( GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV, GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglCombinerOutputNV( GL_COMBINER1_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE1_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

			// ( A * B ) + ( ( 1 - A ) * C ) + D = ( spare0 * 1 ) + ( ( 1 - spare0 ) * 0 ) + spare1 == spare0 + spare1.
			qglFinalCombinerInputNV( GL_VARIABLE_A_NV, GL_SPARE0_NV,    GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_B_NV, GL_ZERO,			GL_UNSIGNED_INVERT_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_C_NV, GL_ZERO,			GL_UNSIGNED_IDENTITY_NV, GL_RGB );
			qglFinalCombinerInputNV( GL_VARIABLE_D_NV, GL_SPARE1_NV,	GL_UNSIGNED_IDENTITY_NV, GL_RGB );
		qglEndList();
	}
	else
	{
		qglGenProgramsARB( 1, &tr.glowPShader );
		qglBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, tr.glowPShader );
		qglProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, ( GLsizei ) strlen( ( char * ) g_strGlowPShaderARB ), g_strGlowPShaderARB );

//		const GLubyte *strErr = qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
		int iErrPos = 0;
		qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &iErrPos );
		assert( iErrPos == -1 );
	}

	qglGenProgramsARB(1, &tr.gammaCorrectVtxShader);
	qglBindProgramARB(GL_VERTEX_PROGRAM_ARB, tr.gammaCorrectVtxShader);
	qglProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(gammaCorrectVtxShader), gammaCorrectVtxShader);

	int errorChar;
	qglGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorChar);
	if ( errorChar != -1 )
	{
		Com_Printf(S_COLOR_RED "ERROR: Failed to compile gamma correction vertex shader. Error at character %d\n", errorChar);
		glConfigExt.doGammaCorrectionWithShaders = qfalse;
	}
	else
	{
		qglGenProgramsARB(1, &tr.gammaCorrectPxShader);
		qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, tr.gammaCorrectPxShader);
		qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(gammaCorrectPxShader), gammaCorrectPxShader);

		qglGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorChar);
		if ( errorChar != -1 )
		{
			Com_Printf(S_COLOR_RED "Failed to compile gamma correction pixel shader. Error at character %d\n", errorChar);
			glConfigExt.doGammaCorrectionWithShaders = qfalse;
		}
	}
}
