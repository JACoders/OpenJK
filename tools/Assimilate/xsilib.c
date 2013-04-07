#include "xsilib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


#define MAX_ASE_MATERIALS			32
#define MAX_ASE_OBJECTS				64 + 32
#define MAX_ASE_ANIMATIONS			32

#define MAX_ASE_BONE_FRAMES (1280)
#define MAX_BONES (150)
#define MAX_BONES_VERT (8)


#define VERBOSE( x ) { if ( ase.verbose ) { printf x ; } }

typedef struct
{
	int bone;
	float wt;
} aseWeight_t;

typedef struct
{
	float x, y, z;
	float xBase, yBase, zBase;
	float nx, ny, nz;
	float s, t;
	aseWeight_t Weights[MAX_BONES_VERT];
} aseVertex_t;

typedef struct
{
	float s, t;
} aseTVertex_t;

typedef int aseFace_t[3];

typedef struct
{
	int numFaces;
	int numVertexes;
	int numTVertexes;

	int timeValue;

	aseVertex_t		*vertexes;
	aseTVertex_t	*tvertexes;
	aseFace_t		*faces, *tfaces;

	int currentFace, currentVertex;

	int BoneRemap[MAX_BONES];
	int numBones;
	int curBone,curBoneVertex;
} aseMesh_t;

typedef struct
{
	char name[128];
} aseMaterial_t;

typedef struct
{
	char name[128];
	float scale;
} aseBone_t;

/*
** contains the animate sequence of a single surface
** using a single material
*/
typedef struct
{
	char name[128];

	int materialRef;

	aseMesh_t	anim;

} aseGeomObject_t;

typedef struct
{
	int				numMaterials;
	aseMaterial_t	materials[MAX_ASE_MATERIALS];
	aseGeomObject_t objects[MAX_ASE_OBJECTS];

	char	*buffer;
	char	*curpos;
	int		 len;

	int			currentObject;
	qboolean	verbose;
	qboolean	grabAnims;

	int			numBones;
	int			numBoneFrames;
	aseBone_t	bones[MAX_BONES];

} ase_t;


static char s_token[1024];
static ase_t ase;
static ase_t aseGrab;

static void ASE_Process( int type );
static void ASE_FreeGeomObject( int ndx );

/*
** ASE_Load
*/
void ASE_Load( const char *filename, qboolean verbose, qboolean grabAnims, int type )
{
	FILE *fp = fopen( filename, "rb" );

	if ( !fp )
		Error( "File not found '%s'", filename );

	memset( &ase, 0, sizeof( ase ) );

	ase.verbose = verbose;
	ase.grabAnims = grabAnims;
	ase.len = Q_filelength( fp );

	ase.curpos = ase.buffer = malloc( ase.len );
	assert (ase.buffer != NULL);

	printf( "Processing '%s'\n", filename );

	if ( fread( ase.buffer, ase.len, 1, fp ) != 1 )
	{
		fclose( fp );
		Error( "fread() != 1 for '%s'", filename );
	}

	fclose( fp );

	ASE_Process(type);
	free (ase.buffer);
}

/*
** ASE_Free
*/
void ASE_Free( void )
{
	int i;

	for ( i = 0; i < ase.currentObject; i++ )
	{
		ASE_FreeGeomObject( i );
	}
}

/*
** ASE_GetNumSurfaces
*/
int ASE_GetNumSurfaces( void )
{
	return ase.currentObject;
}

/*
** ASE_GetSurfaceName
*/
const char *ASE_GetSurfaceName( int which )
{
	aseGeomObject_t *pObject = &ase.objects[which];

	if ( !pObject->anim.numBones )
		return "";

	return pObject->name;
}

/*
** ASE_GetSurfaceAnimation
**
** Returns an animation (sequence of polysets)
*/
polyset_t *ASE_GetSurfaceAnimation( int which, int *pNumFrames, int skipFrameStart, int skipFrameEnd, int maxFrames )
{
	aseGeomObject_t *pObject = &ase.objects[which];
	aseMesh_t *pMesh = &pObject->anim;
	polyset_t *psets;
	int numFramesInAnimation;
	int numFramesToKeep;
	int f;
	int t;

	numFramesInAnimation = 1;

	numFramesToKeep = numFramesInAnimation;

	*pNumFrames = numFramesToKeep;

	psets = calloc( sizeof( polyset_t ) * numFramesToKeep, 1 );

	f=0;


	strcpy( psets[f].name, pObject->name );
	strcpy( psets[f].materialname, ase.materials[pObject->materialRef].name );

	psets[f].triangles = calloc( sizeof( triangle_t ) * pObject->anim.numFaces, 1 );
	psets[f].numtriangles = pObject->anim.numFaces;

	for ( t = 0; t < pObject->anim.numFaces; t++ )
	{
		int k;

		for ( k = 0; k < 3; k++ )
		{
			psets[f].triangles[t].verts[k][0] = pMesh->vertexes[pMesh->faces[t][k]].x;
			psets[f].triangles[t].verts[k][1] = pMesh->vertexes[pMesh->faces[t][k]].y;
			psets[f].triangles[t].verts[k][2] = pMesh->vertexes[pMesh->faces[t][k]].z;

			if ( pMesh->tvertexes && pMesh->tfaces )
			{
				psets[f].triangles[t].texcoords[k][0] = pMesh->tvertexes[pMesh->tfaces[t][k]].s;
				psets[f].triangles[t].texcoords[k][1] = pMesh->tvertexes[pMesh->tfaces[t][k]].t;
			}

		}
	}
	return psets;
}

static void ASE_FreeGeomObject( int ndx )
{
	aseGeomObject_t *pObject;
	pObject = &ase.objects[ndx];

	if ( pObject->anim.vertexes )
	{
		free( pObject->anim.vertexes );
	}
	if ( pObject->anim.tvertexes )
	{
		free( pObject->anim.tvertexes );
	}
	if ( pObject->anim.faces )
	{
		free( pObject->anim.faces );
	}
	if ( pObject->anim.tfaces )
	{
		free( pObject->anim.tfaces );
	}

	memset( pObject, 0, sizeof( *pObject ) );
}

static aseMesh_t *ASE_GetCurrentMesh( void )
{
	aseGeomObject_t *pObject;

	if ( ase.currentObject >= MAX_ASE_OBJECTS )
	{
		Error( "Too many GEOMOBJECTs" );
		return 0; // never called
	}

	pObject = &ase.objects[ase.currentObject];

	return &pObject->anim;
}

static int CharIsTokenDelimiter( int ch )
{
	if ( ch <= 32 )
		return 1;
	return 0;
}

static int ASE_GetToken( qboolean restOfLine )
{
	int i = 0;

	if ( ase.buffer == 0 )
		return 0;

	if ( ( ase.curpos - ase.buffer ) == ase.len )
		return 0;

	// skip over crap
	while ( ( ( ase.curpos - ase.buffer ) < ase.len ) &&
		    ( *ase.curpos <= 32 ) )
	{
		ase.curpos++;
	}

	while ( ( ase.curpos - ase.buffer ) < ase.len )
	{
		s_token[i] = *ase.curpos;

		ase.curpos++;
		i++;

		if ( ( CharIsTokenDelimiter( s_token[i-1] ) && !restOfLine ) ||
			 ( ( s_token[i-1] == '\n' ) || ( s_token[i-1] == '\r' ) ) )
		{
			s_token[i-1] = 0;
			break;
		}
	}

	s_token[i] = 0;

	return 1;
}

static void ASE_ParseBracedBlock( void (*parser)( const char *token ) )
{
	int indent = 0;

	while ( ASE_GetToken( qfalse ) )
	{
		if ( !strcmp( s_token, "{" ) )
		{
			indent++;
		}
		else if ( !strcmp( s_token, "}" ) )
		{
			--indent;
			if ( indent == 0 )
				break;
			else if ( indent < 0 )
				Error( "Unexpected '}'" );
		}
		else
		{
			if ( parser )
				parser( s_token );
		}
	}
}

static void ASE_SkipEnclosingBraces( void )
{
	int indent = 0;

	while ( ASE_GetToken( qfalse ) )
	{
		if ( !strcmp( s_token, "{" ) )
		{
			indent++;
		}
		else if ( !strcmp( s_token, "}" ) )
		{
			indent--;
			if ( indent == 0 )
				break;
			else if ( indent < 0 )
				Error( "Unexpected '}'" );
		}
	}
}

static void ASE_SkipRestOfLine( void )
{
	ASE_GetToken( qtrue );
}

static void ASE_KeyMAP_DIFFUSE( const char *token )
{
	char buffer[1024];
	int i = 0;

	if ( !strcmp( token, "*BITMAP" ) )
	{
		ASE_GetToken( qfalse );

		strcpy( buffer, s_token + 1 );
		if ( strchr( buffer, '"' ) )
			*strchr( buffer, '"' ) = 0;

		while ( buffer[i] )
		{
			if ( buffer[i] == '\\' )
				buffer[i] = '/';
			i++;
		}

		_strlwr(buffer);
		if ( strstr( buffer, gamedir + 2 ) )
		{
			strcpy( ase.materials[ase.numMaterials].name, strstr( buffer, gamedir + 2 ) + strlen( gamedir ) - 2 );
		}
		else
		{
			sprintf( ase.materials[ase.numMaterials].name, "(not converted: '%s')", buffer );
			printf( "WARNING: illegal material name '%s'\n", buffer );
		}
	}
	else
	{
	}
}

static void ASE_KeyMATERIAL( const char *token )
{
	if ( !strcmp( token, "*MAP_DIFFUSE" ) )
	{
		ASE_ParseBracedBlock( ASE_KeyMAP_DIFFUSE );
	}
	else
	{
	}
}

static void ASE_KeyMATERIAL_LIST( const char *token )
{
	if ( !strcmp( token, "*MATERIAL_COUNT" ) )
	{
		ASE_GetToken( qfalse );
		VERBOSE( ( "..num materials: %s\n", s_token ) );
		if ( atoi( s_token ) > MAX_ASE_MATERIALS )
		{
			Error( "Too many materials!" );
		}
		ase.numMaterials = 0;
	}
	else if ( !strcmp( token, "*MATERIAL" ) )
	{
		VERBOSE( ( "..material %d ", ase.numMaterials ) );
		ASE_ParseBracedBlock( ASE_KeyMATERIAL );
		ase.numMaterials++;
	}
}

static void ASE_KeyMESH_VERTEX_LIST( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_VERTEX" ) )
	{
		ASE_GetToken( qfalse );		// skip number

#if 0 //this is handled with matrices later
		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].y = atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].x = -atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].z = atof( s_token );
#else
		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].x = atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].y = atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->currentVertex].z = atof( s_token );
#endif

		pMesh->currentVertex++;

		if ( pMesh->currentVertex > pMesh->numVertexes )
		{
			Error( "pMesh->currentVertex >= pMesh->numVertexes" );
		}
	}
	else
	{
		Error( "Unknown token '%s' while parsing MESH_VERTEX_LIST", token );
	}
}

static void ASE_KeyMESH_FACE_LIST( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_FACE" ) )
	{
		ASE_GetToken( qfalse );	// skip face number

		ASE_GetToken( qfalse );	// skip label
		ASE_GetToken( qfalse );	// first vertex
		pMesh->faces[pMesh->currentFace][0] = atoi( s_token );

		ASE_GetToken( qfalse );	// skip label
		ASE_GetToken( qfalse );	// second vertex
		pMesh->faces[pMesh->currentFace][2] = atoi( s_token );

		ASE_GetToken( qfalse );	// skip label
		ASE_GetToken( qfalse );	// third vertex
		pMesh->faces[pMesh->currentFace][1] = atoi( s_token );

		ASE_GetToken( qtrue );

/*
		if ( ( p = strstr( s_token, "*MESH_MTLID" ) ) != 0 )
		{
			p += strlen( "*MESH_MTLID" ) + 1;
			mtlID = atoi( p );
		}
		else
		{
			Error( "No *MESH_MTLID found for face!" );
		}
*/

		pMesh->currentFace++;
	}
	else
	{
		Error( "Unknown token '%s' while parsing MESH_FACE_LIST", token );
	}
}

static void ASE_KeyTFACE_LIST( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_TFACE" ) )
	{
		int a, b, c;

		ASE_GetToken( qfalse );

		ASE_GetToken( qfalse );
		a = atoi( s_token );
		ASE_GetToken( qfalse );
		c = atoi( s_token );
		ASE_GetToken( qfalse );
		b = atoi( s_token );

		pMesh->tfaces[pMesh->currentFace][0] = a;
		pMesh->tfaces[pMesh->currentFace][1] = b;
		pMesh->tfaces[pMesh->currentFace][2] = c;

		pMesh->currentFace++;
	}
	else
	{
		Error( "Unknown token '%s' in MESH_TFACE", token );
	}
}

static void ASE_KeyMESH_TVERTLIST( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_TVERT" ) )
	{
		char u[80], v[80], w[80];

		ASE_GetToken( qfalse );

		ASE_GetToken( qfalse );
		strcpy( u, s_token );

		ASE_GetToken( qfalse );
		strcpy( v, s_token );

		ASE_GetToken( qfalse );
		strcpy( w, s_token );

		pMesh->tvertexes[pMesh->currentVertex].s = atof( u );
		pMesh->tvertexes[pMesh->currentVertex].t = 1.0f - atof( v );

		pMesh->currentVertex++;

		if ( pMesh->currentVertex > pMesh->numTVertexes )
		{
			Error( "pMesh->currentVertex > pMesh->numTVertexes" );
		}
	}
	else
	{
		Error( "Unknown token '%s' while parsing MESH_TVERTLIST" );
	}
}

static void ASE_KeyMESH( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*TIMEVALUE" ) )
	{
		ASE_GetToken( qfalse );

		pMesh->timeValue = atoi( s_token );
		VERBOSE( ( ".....timevalue: %d\n", pMesh->timeValue ) );
	}
	else if ( !strcmp( token, "*MESH_NUMVERTEX" ) )
	{
		ASE_GetToken( qfalse );

		pMesh->numVertexes = atoi( s_token );
		VERBOSE( ( ".....TIMEVALUE: %d\n", pMesh->timeValue ) );
		VERBOSE( ( ".....num vertexes: %d\n", pMesh->numVertexes ) );
	}
	else if ( !strcmp( token, "*MESH_NUMFACES" ) )
	{
		ASE_GetToken( qfalse );

		pMesh->numFaces = atoi( s_token );
		VERBOSE( ( ".....num faces: %d\n", pMesh->numFaces ) );
	}
	else if ( !strcmp( token, "*MESH_NUMTVFACES" ) )
	{
		ASE_GetToken( qfalse );

		if ( atoi( s_token ) != pMesh->numFaces )
		{
			Error( "MESH_NUMTVFACES != MESH_NUMFACES" );
		}
	}
	else if ( !strcmp( token, "*MESH_NUMTVERTEX" ) )
	{
		ASE_GetToken( qfalse );

		pMesh->numTVertexes = atoi( s_token );
		VERBOSE( ( ".....num tvertexes: %d\n", pMesh->numTVertexes ) );
	}
	else if ( !strcmp( token, "*MESH_VERTEX_LIST" ) )
	{
		pMesh->vertexes = calloc( sizeof( aseVertex_t ) * pMesh->numVertexes, 1 );
		pMesh->currentVertex = 0;
		VERBOSE( ( ".....parsing MESH_VERTEX_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_VERTEX_LIST );
	}
	else if ( !strcmp( token, "*MESH_TVERTLIST" ) )
	{
		pMesh->currentVertex = 0;
		pMesh->tvertexes = calloc( sizeof( aseTVertex_t ) * pMesh->numTVertexes, 1 );
		VERBOSE( ( ".....parsing MESH_TVERTLIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_TVERTLIST );
	}
	else if ( !strcmp( token, "*MESH_FACE_LIST" ) )
	{
		pMesh->faces = calloc( sizeof( aseFace_t ) * pMesh->numFaces, 1 );
		pMesh->currentFace = 0;
		VERBOSE( ( ".....parsing MESH_FACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_FACE_LIST );
	}
	else if ( !strcmp( token, "*MESH_TFACELIST" ) )
	{
		pMesh->tfaces = calloc( sizeof( aseFace_t ) * pMesh->numFaces, 1 );
		pMesh->currentFace = 0;
		VERBOSE( ( ".....parsing MESH_TFACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyTFACE_LIST );
	}
	else if ( !strcmp( token, "*MESH_NORMALS" ) )
	{
		ASE_ParseBracedBlock( 0 );
	}
}

static void ASE_KeyMESH_BONE_LIST( const char *token )
{
	int i;
	char buffer[1024];
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_BONE_NAME" ) )
	{
		ASE_GetToken( qfalse );

		ASE_GetToken( qfalse );

		if (*s_token=='"')
			strcpy( buffer, s_token + 1 );
		else
			strcpy( buffer, s_token );
		if ( strchr( buffer, '"' ) )
			*strchr( buffer, '"' ) = 0;
		for (i=0;i<ase.numBones;i++)
		{
			if (!strcmpi(buffer,ase.bones[i].name))
				break;
		}
		if (i>=ase.numBones)
		{
			strcpy(ase.bones[i].name,buffer);
			ase.numBones++;
		}
		pMesh->BoneRemap[pMesh->curBone]=i;
		pMesh->curBone++;
	}
	else
	{
		Error( "Unknown token '%s' in MESH_BONE_LIST", token );
	}
}

static void ASE_KeyMESH_BONE_VERTEX_LIST( const char *token )
{
	int i,b;
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_BONE_VERTEX" ) )
	{
		ASE_GetToken( qfalse );		// skip number

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->curBoneVertex].xBase = atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->curBoneVertex].yBase = atof( s_token );

		ASE_GetToken( qfalse );
		pMesh->vertexes[pMesh->curBoneVertex].zBase = atof( s_token );

		for (i=0;i<8;i++)
		{
			ASE_GetToken( qfalse );
			b=atoi( s_token );
			if (b>=0)
			{
				assert(b<pMesh->curBone);
				b=pMesh->BoneRemap[b];
			}
			pMesh->vertexes[pMesh->curBoneVertex].Weights[i].bone = b;
			assert(pMesh->vertexes[pMesh->curBoneVertex].Weights[i].bone<ase.numBones);
			ASE_GetToken( qfalse );
			pMesh->vertexes[pMesh->curBoneVertex].Weights[i].wt = atof( s_token );
		}

		pMesh->curBoneVertex++;

		if ( pMesh->curBoneVertex > pMesh->numVertexes )
		{
			Error( "pMesh->curBoneVertex >= pMesh->numVertexes" );
		}
	}
	else
	{
		Error( "Unknown token '%s' in MESH_BONE_VERTEX_LIST", token );
	}
}

static void ASE_KeyMESH_WEIGHTS( const char *token )
{
	aseMesh_t *pMesh = ASE_GetCurrentMesh();

	if ( !strcmp( token, "*MESH_NUMBONE" ) )
	{
		ASE_GetToken( qfalse );

		pMesh->numBones = atoi( s_token );
	}
	else if ( !strcmp( token, "*MESH_NUMVERTEX" ) )
	{
		ASE_GetToken( qfalse );

		if (pMesh->numVertexes != atoi( s_token ))
			Error( "MESH_WEIGHTS NumVerts(%s) doesn't match mesh %s (%d)).", s_token, ase.objects[ase.currentObject].name, pMesh->numVertexes );
	}
	else if ( !strcmp( token, "*MESH_BONE_LIST" ) )
	{
		pMesh->curBone=0;
		ASE_ParseBracedBlock( ASE_KeyMESH_BONE_LIST );
	}
	else if ( !strcmp( token, "*MESH_BONE_VERTEX_LIST" ) )
	{
		pMesh->curBoneVertex=0;
		ASE_ParseBracedBlock( ASE_KeyMESH_BONE_VERTEX_LIST );
	}
}


static void ASE_KeyGEOMOBJECT( const char *token )
{
	if ( !strcmp( token, "*NODE_NAME" ) )
	{
		char *name = ase.objects[ase.currentObject].name;

		ASE_GetToken( qtrue );
		VERBOSE( ( " %s\n", s_token ) );
		strcpy( ase.objects[ase.currentObject].name, s_token + 1 );
		if ( strchr( ase.objects[ase.currentObject].name, '"' ) )
			*strchr( ase.objects[ase.currentObject].name, '"' ) = 0;

		if ( strstr( name, "tag" ) == name )
		{
			while ( strchr( name, '_' ) != strrchr( name, '_' ) )
			{
				*strrchr( name, '_' ) = 0;
			}
			while ( strrchr( name, ' ' ) )
			{
				*strrchr( name, ' ' ) = 0;
			}
		}
	}
	else if ( !strcmp( token, "*NODE_PARENT" ) )
	{
		ASE_SkipRestOfLine();
	}
	// ignore unused data blocks
	else if ( !strcmp( token, "*NODE_TM" ) ||
		      !strcmp( token, "*TM_ANIMATION" ) )
	{
		ASE_ParseBracedBlock( 0 );
	}
	// ignore regular meshes that aren't part of animation
	else if ( !strcmp( token, "*MESH" ))
	{
		ASE_ParseBracedBlock( ASE_KeyMESH );
	}
	else if ( !strcmp( token, "*MESH_WEIGHTS" ))
	{
		ASE_ParseBracedBlock( ASE_KeyMESH_WEIGHTS );
	}
	// according to spec these are obsolete
	else if ( !strcmp( token, "*MATERIAL_REF" ) )
	{
		ASE_GetToken( qfalse );

		ase.objects[ase.currentObject].materialRef = atoi( s_token );
	}
	// loads a sequence of animation frames
	else if ( !strcmp( token, "*MESH_ANIMATION" ) )
	{
		ASE_SkipEnclosingBraces();
	}
	// skip unused info
	else if ( !strcmp( token, "*PROP_MOTIONBLUR" ) ||
		      !strcmp( token, "*PROP_CASTSHADOW" ) ||
			  !strcmp( token, "*PROP_RECVSHADOW" ) )
	{
		ASE_SkipRestOfLine();
	}
}

static void CollapseObjects( void )
{
}

/*
** ASE_Process
*/
static void ASE_Process( int type )
{
	while ( ASE_GetToken( qfalse ) )
	{
		if ( !strcmp( s_token, "*3DSMAX_ASCIIEXPORT" ) ||
			 !strcmp( s_token, "*COMMENT" ) )
		{
			ASE_SkipRestOfLine();
		}
		else if ( !strcmp( s_token, "*SCENE" ) )
			ASE_SkipEnclosingBraces();
		else if ( !strcmp( s_token, "*MATERIAL_LIST" ) )
		{
			VERBOSE( ("MATERIAL_LIST\n") );

			ASE_ParseBracedBlock( ASE_KeyMATERIAL_LIST );
		}
		else if ( !strcmp( s_token, "*GEOMOBJECT" ) )
		{
			VERBOSE( ("GEOMOBJECT" ) );

			ASE_ParseBracedBlock( ASE_KeyGEOMOBJECT );

			if (!ase.objects[ase.currentObject].anim.numFaces)	//we didn't get any faces of animation!
			{
				printf( "WARNING: ASE_Process no triangles grabbed for %s!\n", ase.objects[ase.currentObject].name);
			}
			_strlwr(ase.objects[ase.currentObject].name);
			if ( strstr( ase.objects[ase.currentObject].name, "Bip" ) ||
				 strstr( ase.objects[ase.currentObject].name, "ignore_" ) )
			{
				VERBOSE( ( "(discarding BIP/ignore object)\n" ) );
				ASE_FreeGeomObject( ase.currentObject );
			}
			else if ( (type /*== TYPE_PLAYER*/) && ( strstr( ase.objects[ase.currentObject].name, "h_" ) != ase.objects[ase.currentObject].name ) &&
				      ( strstr( ase.objects[ase.currentObject].name, "l_" ) != ase.objects[ase.currentObject].name ) &&
					  ( strstr( ase.objects[ase.currentObject].name, "u_" ) != ase.objects[ase.currentObject].name ) &&
					  ( strstr( ase.objects[ase.currentObject].name, "tag" ) != ase.objects[ase.currentObject].name ) &&
					  ase.grabAnims )
			{
				VERBOSE( ( "(ignoring improperly labeled object '%s')\n", ase.objects[ase.currentObject].name ) );
				ASE_FreeGeomObject( ase.currentObject );
			}
			else
			{
				if ( ++ase.currentObject == MAX_ASE_OBJECTS )
				{
					Error( "Too many GEOMOBJECTs" );
				}
			}
		}
		else if ( s_token[0] )
		{
			printf( "Unknown token '%s'\n", s_token );
		}
	}

	if ( !ase.currentObject )
		Error( "No animation data!" );

//	CollapseObjects();
}
