// Filename:-	R_Surface.cpp
//
// a container module for paste-code to do with surface rendering
//
#include "stdafx.h"
#include "includes.h"
#include "R_Common.h"
#include "text.h"
//
#include "r_surface.h"


int giRenderedBoneWeights;
int giOmittedBoneWeights;


void GetWeightColour(int iNumWeights, byte &r, byte &g, byte &b)
{
	switch (iNumWeights)
	{
		case 0:
			
			//assert(0);	// this shouldn't happen, but...
			//
			r=255,g=255,b=255;	// white
			break;

		case 1:

			r=0,g=255,b=0;		// bright green
			break;

		case 2:

			r=0,g=128,b=0;		// dark green
			break;

		case 3:

			r=255,g=255,b=0;	// bright yellow
//			r=128,g=128,b=0;	// dark yellow
			break;

		case 4:

			r=0,g=255,b=255;	// bright cyan
			break;

		default:

			// anything > 4 (shouldn't happen, because carcass will limit it)...
			//
			r=255,g=0,b=0;
			break;
	}
}



void RB_StageIteratorGeneric( void );

void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0) {
		return;
	}

	if (input->indexes[ACTUAL_SHADER_MAX_INDEXES-1] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - ACTUAL_SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[ACTUAL_SHADER_MAX_VERTEXES-1][0] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - ACTUAL_SHADER_MAX_VERTEXES hit");
	}
/*MODVIEWREM
	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris (input);
	}
	if ( r_shownormals->integer ) {
		DrawNormals (input);
	}

*/

	RB_StageIteratorGeneric();

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

//MODVIEWREM	GLimp_LogComment( "----------\n" );
}


/*
==============
We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum, GLuint gluiTextureBind ) {
	tess.numIndexes = 0;
	tess.numVertexes = 0;
//MODVIEWREM	tess.shader = shader;
//MODVIEWREM	tess.fogNum = fogNum;
//MODVIEWREM	tess.dlightBits = 0;		// will be OR'd in by surface functions
//MODVIEWREM	tess.xstages = shader->stages;
//MODVIEWREM	tess.numPasses = shader->numUnfoggedPasses;
//MODVIEWREM	tess.currentStageIteratorFunc = shader->optimalStageIteratorFunc;
	tess.gluiTextureBind = gluiTextureBind;
	tess.bSurfaceIsG2Tag = false;
}



void RB_CheckOverflow( int verts, int indexes ) {
	if (tess.numVertexes + verts < ACTUAL_SHADER_MAX_VERTEXES
		&& tess.numIndexes + indexes < ACTUAL_SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= ACTUAL_SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, ACTUAL_SHADER_MAX_VERTEXES );
	}
	if ( indexes >= ACTUAL_SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, ACTUAL_SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(NULL/*MODVIEWREM tess.shader*/, 0/*MODVIEWREM tess.fogNum*/, tess.gluiTextureBind );
}



///////////////////////////////////////////////////
//
// 2 hack functions for ModView to emulate cgame ent-adding...
//
void R_ModView_BeginEntityAdd()
{
	tr.refdef.num_entities = 0;
	tr.refdef.numDrawSurfs = 0;
}

void R_ModView_AddEntity(ModelHandle_t hModel,			int iFrame_Primary, int iOldFrame_Primary, 
							int iBoneNum_SecondaryStart,int iFrame_Secondary, int iOldFrame_Secondary, 
							int iSurfaceNum_RootOverride,
							float fLerp,
					 		surfaceInfo_t *slist,			// pointer to list of surfaces turned off
							boneInfo_t	*blist,				// pointer to list of bones to be overriden
							mdxaBone_t	*pXFormedG2Bones,		// feedback array for after model has rendered
							bool		*pXFormedG2BonesValid,	// and a validity check because of deactivated bones
							mdxaBone_t	*pXFormedG2TagSurfs,		// feedback array for after model has rendered
							bool		*pXFormedG2TagSurfsValid,	// and a validity check because of deactivated surfs
							//
							int *piRenderedTris,
							int *piRenderedVerts,
							int *piRenderedSurfs,
							int *piXformedG2Bones,
//							int	*piRenderedBoneWeightsThisSurface,
							int *piRenderedBoneWeights,
							int *piOmittedBoneWeights
						 )
{
	trRefEntity_t trHackJob;

	// general params...
	//
	trHackJob.e.hModel				=	hModel;
	trHackJob.e.iFrame_Primary		=	iFrame_Primary;
	trHackJob.e.iOldFrame_Primary	=	iOldFrame_Primary;
	trHackJob.e.iFrame_Secondary	=	iFrame_Secondary;
	trHackJob.e.iOldFrame_Secondary	=	iOldFrame_Secondary;
	trHackJob.e.iBoneNum_SecondaryStart = iBoneNum_SecondaryStart;
	trHackJob.e.iSurfaceNum_RootOverride= iSurfaceNum_RootOverride;

	trHackJob.e.backlerp			=	fLerp;		// 0.0 = current, 1.0 = old


	// Ghoul2 params... (even though model not nec. g2 format)
	//
	//
	trHackJob.e.slist		=	slist;
	trHackJob.e.blist		=	blist;

	// some other crap to make life simpler...
	//
	trHackJob.e.renderfx				= RF_CAP_FRAMES;
	trHackJob.e.piRenderedTris			= piRenderedTris;
	trHackJob.e.piRenderedVerts			= piRenderedVerts;
	trHackJob.e.piRenderedSurfs			= piRenderedSurfs;
	trHackJob.e.piXformedG2Bones		= piXformedG2Bones;
//	trHackJob.e.piRenderedBoneWeightsThisSurface = piRenderedBoneWeightsThisSurface;
	trHackJob.e.piRenderedBoneWeights	= piRenderedBoneWeights;
	trHackJob.e.piOmittedBoneWeights	= piOmittedBoneWeights;

	trHackJob.e.pXFormedG2Bones			= pXFormedG2Bones;
	trHackJob.e.pXFormedG2BonesValid	= pXFormedG2BonesValid;
	trHackJob.e.pXFormedG2TagSurfs		= pXFormedG2TagSurfs;
	trHackJob.e.pXFormedG2TagSurfsValid	= pXFormedG2TagSurfsValid;

	// now add it to the list to be processed...
	//
	tr.refdef.entities[ tr.refdef.num_entities++ ] = trHackJob;
}
//
///////////////////////////////////////////////////


void R_AddEntitySurfaces (void)
{
	trRefEntity_t	*ent;
//MODVIEWREM	shader_t		*shader;

	for ( tr.currentEntityNum = 0; 
	      tr.currentEntityNum < tr.refdef.num_entities; 
		  tr.currentEntityNum++ )
	{
		ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

//		ent->needDlights = qfalse;

		// we must set up parts of tr.or for model culling
//MODVIEWREM		R_RotateForEntity( ent, &tr.viewParms, &tr.or );

		tr.currentModel = R_GetModelByHandle( ent->e.hModel );
		if (!tr.currentModel)
		{
			assert(0);
//MODVIEWREM			R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
		}
		else
		{
			switch ( tr.currentModel->type )
			{
			case MOD_MESH:
				R_AddMD3Surfaces( ent );
				break;
			case MOD_MD4:
				R_AddAnimSurfaces( ent );				
				break;
			case MOD_MDXM:
				R_AddGhoulSurfaces( ent);
				break;
			case MOD_BRUSH:
				assert(0);
//				R_AddBrushModelSurfaces( ent );
				break;
			case MOD_BAD:		// null model axis
				assert(0);
/*MODVIEWREM
				if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
				{
					break;
				}
				shader = R_GetShaderByHandle( ent->e.customShader );
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
*/
				break;
			default:
				ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
				break;
			}
		}
	}
}


// entry point from ModView draw function to setup all surfaces ready for actual render call later
void RE_GenerateDrawSurfs( void ) 
{
/*
	R_AddWorldSurfaces ();

	R_AddPolygonSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection ();
*/
	R_AddEntitySurfaces ();
}



void RB_NULL( surfaceInfo_t *surf ) 
{
	assert(0);
	ri.Error( ERR_DROP, "RB_NULL() reached" );
}

int giSurfaceVertsDrawn;
int giSurfaceTrisDrawn;
int giRenderedBoneWeightDrawn;
void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceBad,			// SF_BAD, 
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP, 
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceTriangles,	// SF_TRIANGLES,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfacePolychain,	// SF_POLY,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceMesh,			// SF_MD3,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceAnim,			// SF_MD4,
	(void(*)(void*))RB_SurfaceGhoul,		// SF_MDX,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
(void(*)(void*))RB_NULL,//	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
(void(*)(void*))RB_NULL //	(void(*)(void*))RB_SurfaceDisplayList	// SF_DISPLAY_LIST
};


// findme label: finaldrawpos
//
static int		c_vertexes;		// for seeing how long our average strips are
static int		c_begins;
static void R_DrawStripElements( int numIndexes, const glIndex_t *indexes, void ( APIENTRY *element )(GLint) ) 
{
	int i;
	int last[3] = { -1, -1, -1 };
	qboolean even;

	glBegin( GL_TRIANGLE_STRIP );
	c_begins++;

	if ( numIndexes <= 0 ) {
		return;
	}

	// prime the strip
	element( indexes[0] );
	element( indexes[1] );
	element( indexes[2] );
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for ( i = 3; i < numIndexes; i += 3 )
	{
		// odd numbered triangle in potential strip
		if ( !even )
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( indexes[i+0] == last[2] ) && ( indexes[i+1] == last[1] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;
				assert( indexes[i+2] < tess.numVertexes );
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( last[2] == indexes[i+1] ) && ( last[0] == indexes[i+0] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				glEnd();

				glBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i+0];
		last[1] = indexes[i+1];
		last[2] = indexes[i+2];
	}

	glEnd();
}



static void R_DrawElements( int numIndexes, const glIndex_t *indexes ) {
	int		primitives;

	primitives = 1;//MODVIEWREM: r_primitives->integer;
/*MODVIEWREM
	// default is to use triangles if compiled vertex arrays are present
	if ( primitives == 0 ) {
		if ( qglLockArraysEXT ) {
			primitives = 2;
		} else {
			primitives = 1;
		}
	}


	if ( primitives == 2 ) {
		qglDrawElements( GL_TRIANGLES, 
						numIndexes,
						GL_INDEX_TYPE,
						indexes );
		return;
	}
*/
	if ( primitives == 1 ) {
		R_DrawStripElements( numIndexes,  indexes, glArrayElement );
		return;
	}

/*	MODVIEWREM
	if ( primitives == 3 ) {
		R_DrawStripElements( numIndexes,  indexes, R_ArrayElementDiscrete );
		return;
	}
*/
	// anything else will cause no drawing
}


static inline void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	cross[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	cross[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
}


static vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if (length)
	{
		ilength = 1/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}
		
	return length;

}

/*
// Fuck this maths shit, it doesn't work
#define real_nclip(x0,y0,x1,y1,x2,y2)   (  (y1-y0)*(x2-x1) - (x1-x0)*(y2-y1) )

static bool MouseOverTri(float x0, float x1, float x2, float y0, float y1, float y2, float mX, float mY)
{
	// Check that winding of all 3 lines of x/y(n) mX,mY is in same direction
	// Don't know winding direction, so use first winding check to pick further winding direction checks
	//
	float a = real_nclip(x0, y0, y1, y1, mX,mY);
	if (a == 0) return true; // p0 exactly on edge
	if (a > 0)
	{
		// all further winding checks should be greater of equal to zero for the point to lie inside the polygon
		//
		if (real_nclip(y1,y1,y2,y2,mX,mY)<0) return false;
		if (real_nclip(y2,y2,y0,y0,mX,mY)<0) return false;
	}
	else
	{
		// all further winding checks should be less than zero for the point to lie inside the polygon
		//
		if (real_nclip(y1,y1,y2,y2,mX,mY)>0) return false;
		if (real_nclip(y2,y2,y0,y0,mX,mY)>0) return false;		
	}

	return true;
}
*/


/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
/*	MODVIEWREM
	int stage;

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if ( !pStage )
		{
			break;
		}

		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce )
		{
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}

		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != 0 )
		{
			DrawMultitextured( input, stage );
		}
		else
		{

			if ( !setArraysOnce )
			{
				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
			}

			//
			// set state
			//
			if ( pStage->bundle[0].vertexLightmap && ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) && r_lightmap->integer )
			{
				GL_Bind( tr.whiteImage );
			}
			else 
				R_BindAnimatedImage( &pStage->bundle[0] );

			GL_State( pStage->stateBits );

			//
			// draw
			//
			R_DrawElements( input->numIndexes, input->indexes );
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
		{
			break;
		}
	}
*/		
	glBindTexture( GL_TEXTURE_2D, input->gluiTextureBind );


		
	
	{// note additional loop I put here for overriding polys to be wireframe - Ste.

		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);	// preserves GL_CULL_FACE, GL_CULL_FACE_MODE, GL_POLYGON_MODE
		{
			if (!input->bSurfaceIsG2Tag			// don't draw G2 surface tags
				|| AppVars.bShowTagSurfaces		// ... unless you really want to
				)
			{
				bool bSurfaceIsUnshadowable =	AppVars.bShowUnshadowableSurfaces && (input->numVertexes > (SHADER_MAX_VERTEXES/2));
				bool bSurfaceIsHighlighted =	AppVars.bSurfaceHighlight &&
												input->hModel == AppVars.hModelToHighLight &&
												(
													(AppVars.iSurfaceNumToHighlight == iITEMHIGHLIGHT_ALL)
													||
													(AppVars.iSurfaceNumToHighlight == iITEMHIGHLIGHT_ALL_TAGSURFACES && input->bSurfaceIsG2Tag)
													||
													(AppVars.iSurfaceNumToHighlight == input->iSurfaceNum)
												);				
				bool bSpecialCaseHighlightSoNoYellowNumberClash = (AppVars.bVertIndexes && AppVars.bVertWeighting && AppVars.iSurfaceNumToHighlight < 0);

				if (bSurfaceIsHighlighted && !bSpecialCaseHighlightSoNoYellowNumberClash)
					glLineWidth(2);
				else
					glLineWidth(1);
				bool b2PassForWire = AppVars.bWireFrame || (AppVars.bWireFrame && bSurfaceIsHighlighted);

				if (b2PassForWire)
				{
					if (AppVars.bShowPolysAsDoubleSided && !AppVars.bForceWhite)
					{
						glEnable(GL_CULL_FACE);	
					}
				}
		
		//		for (int iPass=0; iPass<(AppVars.bWireFrame?2:1); iPass++)
				for (int iPass=0; iPass<(b2PassForWire?2:1); iPass++)
				{
					if (b2PassForWire)
					{
						if (!iPass)
						{
							glCullFace(GL_BACK);

							if (bSurfaceIsHighlighted && !bSpecialCaseHighlightSoNoYellowNumberClash)
								glColor3f(0.5,0.5,0.0);	// dim yellow
							else
								glColor3f(0.5,0.5,0.5);	// dim white
						}
						else
						{
							glCullFace(GL_FRONT);
							if (bSurfaceIsHighlighted && !bSpecialCaseHighlightSoNoYellowNumberClash)
								glColor3f( 1,1,0);		// yellow
							else
								glColor3f( 1,1,1);		// white
						}
					}
					
					R_DrawElements( input->numIndexes, input->indexes );	// the standard surface draw code
				}

				if (b2PassForWire)
				{						
					if (AppVars.bShowPolysAsDoubleSided && !AppVars.bForceWhite)
					{
						glDisable(GL_CULL_FACE);
					}
				}


				glLineWidth(1);

				// draw surface-highlights?...  (2 types, so do 2 passes to keep code down)
				//
				if (!AppVars.bWireFrame && bSurfaceIsHighlighted && !bSpecialCaseHighlightSoNoYellowNumberClash)
				{
					// do these 3 in case we're not already in wireframe...
					//
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_BLEND);
					glDisable(GL_LIGHTING);					

					glLineWidth(2);
					glColor3f(1,1,0);	// yellow
					
					if (AppVars.iSurfaceNumToHighlight > 0)
					{					
						SurfaceOnOff_t eOnOff = Model_GLMSurface_GetStatus( input->hModel, input->iSurfaceNum );
						if (eOnOff != SURF_ON)
						{
							// then we must be ON only because of highlighting an OFF surface in the treeview,
							//	so show it dimmer (particualrly if they've just turned it off and wonder why they
							//	can still see it...
							//
							glColor3f(0.5,0.5,0);	// dim yellow
						}
					}

					for (int iVert = 0; iVert<input->numIndexes; iVert+=3)
					{
						glBegin(GL_LINE_LOOP);
						{
							glVertex3fv( input->xyz[input->indexes[iVert+0]] );
							glVertex3fv( input->xyz[input->indexes[iVert+1]] );
							glVertex3fv( input->xyz[input->indexes[iVert+2]] );
						}
						glEnd();
					}
					
					glLineWidth(1);
				}

				// draw unshadowable surfaces...
				//
				if (bSurfaceIsUnshadowable)
				{
					// do these 3 in case we're not already in wireframe...
					//
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_BLEND);
					glDisable(GL_LIGHTING);					

					glLineStipple( 8, 0xAAAA);
					glEnable(GL_LINE_STIPPLE);
					glColor3f(1,0,0);	// red

					if (bSurfaceIsHighlighted)
					{
						glLineWidth(4);	// ... or it won't stand out much over the existing yellow highlights
					}

					for (int iVert = 0; iVert<input->numIndexes; iVert+=3)
					{
						glBegin(GL_LINE_LOOP);
						{
							glVertex3fv( input->xyz[input->indexes[iVert+0]] );
							glVertex3fv( input->xyz[input->indexes[iVert+1]] );
							glVertex3fv( input->xyz[input->indexes[iVert+2]] );
						}
						glEnd();
					}
					
					glDisable(GL_LINE_STIPPLE);

					if (bSurfaceIsHighlighted)
					{
						glLineWidth(1);
					}
				}

				if (AppVars.bCrackHighlight && bSurfaceIsHighlighted)
				{
					extern ModelContainer_t* gpContainerBeingRendered;
					if (gpContainerBeingRendered)	// arrrghhh!!!!
					{
						int iCappedLOD = Model_EnsureGenerated_VertEdgeInfo(gpContainerBeingRendered, AppVars.iLOD);

						SurfaceEdgeVertBools_t &SurfaceEdgeVertBools = gpContainerBeingRendered->SurfaceEdgeInfoPerLOD[iCappedLOD];
						SurfaceEdgeVertBools_t::iterator it = SurfaceEdgeVertBools.find(input->iSurfaceNum);
						if (it != SurfaceEdgeVertBools.end())
						{
							VertIsEdge_t &vrVertIsEdge = (*it).second;

							// highlight the edge verts...
							//
							for (int iIndex=0; iIndex<input->numIndexes; iIndex++)
							{
								int iVert = input->indexes[iIndex];
								if (vrVertIsEdge[iVert])
								{										
									Text_Display("*",input->xyz[iVert],0,255,0);					
								}
							}
						}
					}
				}

/*
				if (1)
				{
					extern int g_iScreenWidth;
					extern int g_iScreenHeight;
					extern int g_iViewAreaMouseX;
					extern int g_iViewAreaMouseY;

					// Header: Declared in Glu.h.
					// Library: Use Glu32.lib.

					GLdouble	modelMatrix[16];
					GLdouble	projMatrix[16];
					GLint		viewPort[4];
					int			iOpenGLMouseX = g_iViewAreaMouseX;
					int			iOpenGLMouseY = (g_iScreenHeight - g_iViewAreaMouseY)-1;

					glGetDoublev	( GL_MODELVIEW_MATRIX,  modelMatrix);
					glGetDoublev	( GL_PROJECTION_MATRIX, projMatrix);
					glGetIntegerv	( GL_VIEWPORT,			viewPort);

					for (int iVert = 0; iVert<input->numIndexes; iVert+=3)
					{						
						GLdouble dX[3],dY[3],dZ[3];

						int iSuccess = 0;
						for (int i=0; i<3; i++)
						{
							iSuccess += gluProject(	input->xyz[input->indexes[iVert+i]][0],	// GLdouble objx,
													input->xyz[input->indexes[iVert+i]][1],	// GLdouble objy,
													input->xyz[input->indexes[iVert+i]][2],	// GLdouble objz,
													modelMatrix,							// const GLdouble modelMatrix[16],
													projMatrix,								// const GLdouble projMatrix[16],
													viewPort,								// const GLint viewport[4],
													&dX[i],&dY[i],&dZ[i]
													);
						}

						if (iSuccess == i)
						{
							// got the 3 vert coords as screen coords, now see if the mouse is within this poly
							//
							if (MouseOverTri(dX[0],dX[1],dX[2],dY[0],dY[1],dY[2], iOpenGLMouseX, iOpenGLMouseY))
							{
								AppVars.iSurfaceNumToHighlight = input->iSurfaceNum;
								OutputDebugString(va("Over surface %d\n",input->iSurfaceNum));
								break;
							}
						}
					}
				}
*/
				// draw normals?...
				//
				if (AppVars.bVertexNormals)
				{
					// do these 3 in case we're doing normals but not wireframe...
					//
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_BLEND);
					glDisable(GL_LIGHTING);

					for (int iNormal = 0; iNormal<input->numVertexes/*numIndexes*/; iNormal++)
					{
						glColor3f(1,0.5,1);	// purple
						glBegin(GL_LINES);
						{
							glVertex3fv(	input->xyz[iNormal] );
							glVertex3f (	input->xyz[iNormal][0] + input->normal[iNormal][0],
											input->xyz[iNormal][1] + input->normal[iNormal][1],
											input->xyz[iNormal][2] + input->normal[iNormal][2]
										);
						}				
						glEnd();				
					}
				}

				// show vertex indexes?...
				//
				if (AppVars.bVertIndexes && bSurfaceIsHighlighted && 
						(
						(AppVars.iSurfaceNumToHighlight != iITEMHIGHLIGHT_ALL || AppVars.bVertWeighting)	// or it drops the framerate through the floor!
						&&
						AppVars.iSurfaceNumToHighlight != iITEMHIGHLIGHT_ALL_TAGSURFACES
						)
					)
				{						
					for (int iVert = 0; iVert<input->numIndexes; iVert++)
					{
						byte r=255,g=0,b=0;	// red

						int iNumWeights = 0;

						if (AppVars.bVertWeighting)
						{
							iNumWeights = input->WeightsUsed[input->indexes[iVert]];						

//							if (gpContainerBeingRendered)
//								gpContainerBeingRendered->iRenderedBoneWeightsThisSurface += iNumWeights;

							GetWeightColour(iNumWeights,r,g,b);

							AppVars.bAtleast1VertWeightDisplayed = true;
						}

						if (AppVars.iSurfaceNumToHighlight != iITEMHIGHLIGHT_ALL
							|| iNumWeights>=3
							)
						{
							Text_Display(va(" %d",input->indexes[iVert]),input->xyz[input->indexes[iVert]],r,g,b);					
						}
					}
				}

				// show triangle indexes?...
				//
				if (AppVars.bTriIndexes && bSurfaceIsHighlighted && 
						(
						(AppVars.iSurfaceNumToHighlight != iITEMHIGHLIGHT_ALL)	// or it drops the framerate through the floor!
						&&
						AppVars.iSurfaceNumToHighlight != iITEMHIGHLIGHT_ALL_TAGSURFACES
						)
					)
				{
					for (int iTri = 0; iTri<input->numIndexes; iTri+=3)	// iTri is effectively like stepval 3 for vert parsing
					{
						byte r=0,g=255,b=255;	// magenta

						vec3_t v3TriCentre;

						v3TriCentre[0] =	(
											input->xyz[input->indexes[iTri+0]][0] +
											input->xyz[input->indexes[iTri+1]][0] +
											input->xyz[input->indexes[iTri+2]][0]
											)/3;

						v3TriCentre[1] =	(
											input->xyz[input->indexes[iTri+0]][1] +
											input->xyz[input->indexes[iTri+1]][1] +
											input->xyz[input->indexes[iTri+2]][1]
											)/3;

						v3TriCentre[2] =	(
											input->xyz[input->indexes[iTri+0]][2] +
											input->xyz[input->indexes[iTri+1]][2] +
											input->xyz[input->indexes[iTri+2]][2]
											)/3;

						Text_Display(va("T:%d",iTri/3), v3TriCentre ,r,g,b);					
					}
				}	

				// show vertexes with omitted bone-weights (threshholding)?...
				//
				if (AppVars.bBoneWeightThreshholdingActive && AppVars.bWireFrame)
				{
//					glDisable(GL_TEXTURE_2D);
//					glDisable(GL_BLEND);
//					glDisable(GL_LIGHTING);

//					glLineWidth(9);
					{
//						glColor3f(0,1,0);	// green

//						glBegin(GL_POINTS);
						{
							for (int iVert=0; iVert<input->numIndexes; iVert++)
							{
								if (input->WeightsOmitted[input->indexes[iVert]])
								{										
									Text_Display("*",input->xyz[input->indexes[iVert]],0,255,0);					
								}
							}
						}
//						glEnd();
					}
//					glLineWidth(1);
				}
			}

			// if this is a G2 tag surface, then work out a matrix from it and store for later use...
			//
			if (input->bSurfaceIsG2Tag)
			{
				// not a clever place to do this, but WTF...
				//
				// Anyway, this is some of Jake's mysterious code to turn a one-triangle tag-surface into a matrix...
				//
				vec3_t			axes[3], sides[3];
				float			pTri[3][3], d;

				memcpy(pTri[0],input->xyz[0],sizeof(vec3_t));
				memcpy(pTri[1],input->xyz[1],sizeof(vec3_t));
				memcpy(pTri[2],input->xyz[2],sizeof(vec3_t));

 				// clear out used arrays
 				memset( axes, 0, sizeof( axes ) );
 				memset( sides, 0, sizeof( sides ) );

 				// work out actual sides of the tag triangle
 				for ( int j = 0; j < 3; j++ )
 				{
 					sides[j][0] = pTri[(j+1)%3][0] - pTri[j][0];
 					sides[j][1] = pTri[(j+1)%3][1] - pTri[j][1];
 					sides[j][2] = pTri[(j+1)%3][2] - pTri[j][2];
 				}

 				// do math trig to work out what the matrix will be from this triangle's translated position
 				VectorNormalize2( sides[iG2_TRISIDE_LONGEST], axes[0] );
 				VectorNormalize2( sides[iG2_TRISIDE_SHORTEST], axes[1] );

 				// project shortest side so that it is exactly 90 degrees to the longer side
 				d = DotProduct( axes[0], axes[1] );
 				VectorMA( axes[0], -d, axes[1], axes[0] );
 				VectorNormalize2( axes[0], axes[0] );

 				CrossProduct( sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2] );
 				VectorNormalize2( axes[2], axes[2] );

				//float Jmatrix[3][4];
				mdxaBone_t Jmatrix;

				#define MDX_TAG_ORIGIN 2

 				// set up location in world space of the origin point in out going matrix
 				Jmatrix.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 				Jmatrix.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 				Jmatrix.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

 				// copy axis to matrix - do some magic to orient minus Y to positive X and so on so bolt on stuff is oriented correctly
				Jmatrix.matrix[0][0] = axes[1][0];
				Jmatrix.matrix[0][1] = axes[0][0];
				Jmatrix.matrix[0][2] = -axes[2][0];

				Jmatrix.matrix[1][0] = axes[1][1];
				Jmatrix.matrix[1][1] = axes[0][1];
				Jmatrix.matrix[1][2] = -axes[2][1];

				Jmatrix.matrix[2][0] = axes[1][2];
				Jmatrix.matrix[2][1] = axes[0][2];
				Jmatrix.matrix[2][2] = -axes[2][2];				

				input->pRefEnt->pXFormedG2TagSurfs		[input->iSurfaceNum] = Jmatrix;
				input->pRefEnt->pXFormedG2TagSurfsValid	[input->iSurfaceNum] = true;

	//			OutputDebugString(va("Tag surf %d is valid\n",input->iSurfaceNum));
			}
		}
		glPopAttrib();
		glColor3f( 1,1,1);		
	}
}




void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input;

	input = &tess;
/* MODVIEWREM
	RB_DeformTessGeometry();

	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}
*/

/*	MODVIEWREM
	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 || input->shader->multitextureEnv )
	{
		setArraysOnce = qfalse;
		qglDisableClientState (GL_COLOR_ARRAY);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
	else
*/
	{
//		setArraysOnce = qtrue;

//MODVIEWREM		glEnableClientState( GL_COLOR_ARRAY);
//		glColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		if (!AppVars.bWireFrame)
		{
			glEnableClientState( GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer( 2, GL_FLOAT, 16, input->texCoords	);//tess.svars.texcoords[0] );
		}
	}

	//
	// lock XYZ
	//
	glVertexPointer (3, GL_FLOAT, 16, input->xyz);	// padded for SIMD
/*MODVIEWREM
	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}
*/

/*	MODVUEWREM
	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if ( !setArraysOnce )
	{
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glEnableClientState( GL_COLOR_ARRAY );
	}
*/
	//
	// call shader function
	//
	glEnableClientState( GL_VERTEX_ARRAY );
	RB_IterateStagesGeneric( input );

/*	MODVIEWREM
	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
		&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
*/
}


/*
=================
Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
/*
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms,
					   orientationr_t *or ) {
	float	glMatrix[16];
	vec3_t	delta;
	float	axisLength;

	if ( ent->e.reType != RT_MODEL ) {
		*or = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, or->origin );

	VectorCopy( ent->e.axis[0], or->axis[0] );
	VectorCopy( ent->e.axis[1], or->axis[1] );
	VectorCopy( ent->e.axis[2], or->axis[2] );

	glMatrix[0] = or->axis[0][0];
	glMatrix[4] = or->axis[1][0];
	glMatrix[8] = or->axis[2][0];
	glMatrix[12] = or->origin[0];

	glMatrix[1] = or->axis[0][1];
	glMatrix[5] = or->axis[1][1];
	glMatrix[9] = or->axis[2][1];
	glMatrix[13] = or->origin[1];

	glMatrix[2] = or->axis[0][2];
	glMatrix[6] = or->axis[1][2];
	glMatrix[10] = or->axis[2][2];
	glMatrix[14] = or->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->or.origin, or->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0 / axisLength;
		}
	} else {
		axisLength = 1.0;
	}

	or->viewOrigin[0] = DotProduct( delta, or->axis[0] ) * axisLength;
	or->viewOrigin[1] = DotProduct( delta, or->axis[1] ) * axisLength;
	or->viewOrigin[2] = DotProduct( delta, or->axis[2] ) * axisLength;
}
*/




void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) 
{
	shader_t		/**shader,*/ *oldShader;
	int				/*fogNum,*/ oldFogNum;
	int				entityNum, oldEntityNum;
	int				/*dlighted,*/ oldDlighted;
	qboolean		depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	int				oldSort;
//	float			originalTime;

/*MODVIEWREM
	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();
*/
	// draw everything
	oldEntityNum = -1;
//MODVIEWREM	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = -1;
	depthRange = qfalse;

//MODVIEWREM	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) 
	{
/*MODVIEWREM
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
*/
		GLuint gluiTextureBind = 0;
		R_DecomposeSort( drawSurf->sort, &entityNum, &gluiTextureBind);//MODVIEWREM	, &shader, &fogNum, &dlighted );
/*
		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted 
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = qfalse;

			if ( entityNum != ENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

				// set up the transformation matrix
*/
				////////////////////////////////R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );
/*
				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}
*/
			//////////////////////////////////////glLoadMatrixf( backEnd.or.modelMatrix );
/*
			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange ) {
				if ( depthRange ) {
					qglDepthRange (0, 0.3);
				} else {
					qglDepthRange (0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}
*/

				RB_BeginSurface( 0,0, gluiTextureBind);//shader, fogNum );				

				tess.hModel = tr.refdef.entities[entityNum].e.hModel;
				tess.pRefEnt=&tr.refdef.entities[entityNum].e;

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );


				RB_EndSurface();

				// stats...
				//
				if (!tess.bSurfaceIsG2Tag || AppVars.bShowTagSurfaces)
				{	
					*tr.refdef.entities[entityNum].e.piRenderedTris += giSurfaceTrisDrawn;
					*tr.refdef.entities[entityNum].e.piRenderedVerts+= giSurfaceVertsDrawn;
					*tr.refdef.entities[entityNum].e.piRenderedSurfs+= 1;		// NOT ++!
					*tr.refdef.entities[entityNum].e.piRenderedBoneWeights += giRenderedBoneWeights;
					*tr.refdef.entities[entityNum].e.piOmittedBoneWeights  += giOmittedBoneWeights;
				}
	}
/*MODVIEWREM
	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange (0, 1);
	}
*/
}



// entry point from ModView draw function to setup all surfaces ready for actual render call later...
//
void RE_RenderDrawSurfs( void )
{
	RB_RenderDrawSurfList( tr.refdef.drawSurfs, tr.refdef.numDrawSurfs );
}


////////////// eof //////////

