/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// sv_bot.c
#include "server.h"
#include "botlib/botlib.h"
#include "qcommon/cm_public.h"
#include "server/sv_gameapi.h"

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

static bot_debugpoly_t *debugpolygons;
int bot_maxdebugpolys;

extern botlib_export_t	*botlib_export;
int	bot_enable;

static int gWPNum = 0;
static wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

static int NotWithinRange(int base, int extent)
{
	if (extent > base && base+5 >= extent)
	{
		return 0;
	}

	if (extent < base && base-5 <= extent)
	{
		return 0;
	}

	return 1;
}

int SV_OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore)
{
	trace_t tr;


	SV_Trace(&tr, org1, mins, maxs, org2, ignore, MASK_SOLID, 0, 0, 10);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

void *BotVMShift( intptr_t ptr );

void SV_BotWaypointReception(int wpnum, wpobject_t **wps)
{
	int i = 0;

	gWPNum = wpnum;

	while (i < gWPNum)
	{
		gWPArray[i] = wps[i];
		i++;
	}
}

/*
==================
SV_BotCalculatePaths
==================
*/
void SV_BotCalculatePaths( int /*rmg*/ )
{
	int i;
	int c;
	int forceJumpable;
	int maxNeighborDist = MAX_NEIGHBOR_LINK_DISTANCE;
	float nLDist;
	vec3_t a;
	vec3_t mins, maxs;

	if (!gWPNum)
	{
		return;
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -15; //-1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 15; //1

	//now clear out all the neighbor data before we recalculate
	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->neighbornum)
		{
			while (gWPArray[i]->neighbornum >= 0)
			{
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = 0;
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
				gWPArray[i]->neighbornum--;
			}
			gWPArray[i]->neighbornum = 0;
		}

		i++;
	}

	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			c = 0;

			while (c < gWPNum)
			{
				if (gWPArray[c] && gWPArray[c]->inuse && i != c &&
					NotWithinRange(i, c))
				{
					VectorSubtract(gWPArray[i]->origin, gWPArray[c]->origin, a);

					nLDist = VectorLength(a);
					forceJumpable = qfalse;//CanForceJumpTo(i, c, nLDist);

					if ((nLDist < maxNeighborDist || forceJumpable) &&
						((int)gWPArray[i]->origin[2] == (int)gWPArray[c]->origin[2] || forceJumpable) &&
						(SV_OrgVisibleBox(gWPArray[i]->origin, mins, maxs, gWPArray[c]->origin, ENTITYNUM_NONE) || forceJumpable))
					{
						gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = c;
						if (forceJumpable && ((int)gWPArray[i]->origin[2] != (int)gWPArray[c]->origin[2] || nLDist < maxNeighborDist))
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 999;//forceJumpable; //FJSR
						}
						else
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
						}
						gWPArray[i]->neighbornum++;
					}

					if (gWPArray[i]->neighbornum >= MAX_NEIGHBOR_SIZE)
					{
						break;
					}
				}
				c++;
			}
		}
		i++;
	}
}

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(void) {
	int			i;
	client_t	*cl;

	// find a client slot
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if ( cl->state == CS_FREE ) {
			break;
		}
	}

	if ( i == sv_maxclients->integer ) {
		return -1;
	}

	cl->gentity = SV_GentityNum( i );
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	// cannot start recording auto demos here since bot's name is not set yet
	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient( int clientNum ) {
	client_t	*cl;

	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum );
	}
	cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if ( cl->gentity ) {
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}

	if ( cl->demo.demorecording ) {
		SV_StopRecordDemo( cl );
	}
}

/*
==================
BotDrawDebugPolygons
==================
*/
void BotDrawDebugPolygons(void (*drawPoly)(int color, int numPoints, float *points), int value) {
	static cvar_t *bot_debug, *bot_groundonly, *bot_reachability, *bot_highlightarea;
	bot_debugpoly_t *poly;
	int i, parm0;

	if (!debugpolygons)
		return;
	//bot debugging
	if (!bot_debug) bot_debug = Cvar_Get("bot_debug", "0", 0);
	//
	if (bot_enable && bot_debug->integer) {
		//show reachabilities
		if (!bot_reachability) bot_reachability = Cvar_Get("bot_reachability", "0", 0);
		//show ground faces only
		if (!bot_groundonly) bot_groundonly = Cvar_Get("bot_groundonly", "1", 0);
		//get the hightlight area
		if (!bot_highlightarea) bot_highlightarea = Cvar_Get("bot_highlightarea", "0", 0);
		//
		parm0 = 0;
		if (svs.clients[0].lastUsercmd.buttons & BUTTON_ATTACK) parm0 |= 1;
		if (bot_reachability->integer) parm0 |= 2;
		if (bot_groundonly->integer) parm0 |= 4;
		botlib_export->BotLibVarSet("bot_highlightarea", bot_highlightarea->string);
		botlib_export->Test(parm0, NULL, svs.clients[0].gentity->r.currentOrigin,
			svs.clients[0].gentity->r.currentAngles);
	} //end if
	//draw all debug polys
	for (i = 0; i < bot_maxdebugpolys; i++) {
		poly = &debugpolygons[i];
		if (!poly->inuse) continue;
		drawPoly(poly->color, poly->numPoints, (float *) poly->points);
		//Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints);
	}
}

/*
==================
BotImport_Print
==================
*/
void QDECL BotImport_Print(int type, char *fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			Com_Printf("%s", str);
			break;
		}
		case PRT_WARNING: {
			Com_Printf(S_COLOR_YELLOW "Warning: %s", str);
			break;
		}
		case PRT_ERROR: {
			Com_Printf(S_COLOR_RED "Error: %s", str);
			break;
		}
		case PRT_FATAL: {
			Com_Printf(S_COLOR_RED "Fatal: %s", str);
			break;
		}
		case PRT_EXIT: {
			Com_Error(ERR_DROP, S_COLOR_RED "Exit: %s", str);
			break;
		}
		default: {
			Com_Printf("unknown print type\n");
			break;
		}
	}
}

/*
==================
BotImport_Trace
==================
*/
void BotImport_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) {
	trace_t trace;

	SV_Trace(&trace, start, mins, maxs, end, passent, contentmask, qfalse, 0, 10);
	//copy the trace information
	bsptrace->allsolid = (qboolean)trace.allsolid;
	bsptrace->startsolid = (qboolean)trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotImport_EntityTrace
==================
*/
void BotImport_EntityTrace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask) {
	trace_t trace;

	SV_ClipToEntity(&trace, start, mins, maxs, end, entnum, contentmask, qfalse);
	//copy the trace information
	bsptrace->allsolid = (qboolean)trace.allsolid;
	bsptrace->startsolid = (qboolean)trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}


/*
==================
BotImport_PointContents
==================
*/
int BotImport_PointContents(vec3_t point) {
	return SV_PointContents(point, -1);
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS(vec3_t p1, vec3_t p2) {
	return SV_inPVS (p1, p2);
}

/*
==================
BotImport_BSPEntityData
==================
*/
char *BotImport_BSPEntityData(void) {
	return CM_EntityString();
}

/*
==================
BotImport_BSPModelMinsMaxsOrigin
==================
*/
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin) {
	clipHandle_t h;
	vec3_t mins, maxs;
	float max;
	int	i;

	h = CM_InlineModel(modelnum);
	CM_ModelBounds(h, mins, maxs);
	//if the model is rotated
	if ((angles[0] || angles[1] || angles[2])) {
		// expand for rotation

		max = RadiusFromBounds(mins, maxs);
		for (i = 0; i < 3; i++) {
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if (outmins) VectorCopy(mins, outmins);
	if (outmaxs) VectorCopy(maxs, outmaxs);
	if (origin) VectorClear(origin);
}

/*
==================
BotImport_GetMemoryGame
==================
*/
void *Bot_GetMemoryGame(int size) {
	void *ptr;

	ptr = Z_Malloc( size, TAG_BOTGAME, qtrue );

	return ptr;
}

/*
==================
BotImport_FreeMemoryGame
==================
*/
void Bot_FreeMemoryGame(void *ptr) {
	Z_Free(ptr);
}

/*
==================
BotImport_GetMemory
==================
*/
void *BotImport_GetMemory(int size) {
	void *ptr;

	ptr = Z_Malloc( size, TAG_BOTLIB, qtrue );
	return ptr;
}

/*
==================
BotImport_FreeMemory
==================
*/
void BotImport_FreeMemory(void *ptr) {
	Z_Free(ptr);
}

/*
=================
BotImport_HunkAlloc
=================
*/
void *BotImport_HunkAlloc( int size ) {
	if( Hunk_CheckMark() ) {
		Com_Error( ERR_DROP, "SV_Bot_HunkAlloc: Alloc with marks already set\n" );
	}
	return Hunk_Alloc( size, h_high );
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points) {
	bot_debugpoly_t *poly;
	int i;

	if (!debugpolygons)
		return 0;

	for (i = 1; i < bot_maxdebugpolys; i++) 	{
		if (!debugpolygons[i].inuse)
			break;
	}
	if (i >= bot_maxdebugpolys)
		return 0;
	poly = &debugpolygons[i];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
	//
	return i;
}

/*
==================
BotImport_DebugPolygonShow
==================
*/
void BotImport_DebugPolygonShow(int id, int color, int numPoints, vec3_t *points) {
	bot_debugpoly_t *poly;

	if (!debugpolygons) return;
	poly = &debugpolygons[id];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
}

/*
==================
BotImport_DebugPolygonDelete
==================
*/
void BotImport_DebugPolygonDelete(int id)
{
	if (!debugpolygons) return;
	debugpolygons[id].inuse = qfalse;
}

/*
==================
BotImport_DebugLineCreate
==================
*/
int BotImport_DebugLineCreate(void) {
	vec3_t points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

/*
==================
BotImport_DebugLineDelete
==================
*/
void BotImport_DebugLineDelete(int line) {
	BotImport_DebugPolygonDelete(line);
}

/*
==================
BotImport_DebugLineShow
==================
*/
void BotImport_DebugLineShow(int line, vec3_t start, vec3_t end, int color) {
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	//points[1][2] -= 2;
	VectorCopy(end, points[2]);
	//points[2][2] -= 2;
	VectorCopy(end, points[3]);


	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99) VectorSet(cross, 1, 0, 0);
	else CrossProduct(dir, up, cross);

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	BotImport_DebugPolygonShow(line, color, 4, points);
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand( int client, char *command ) {
	SV_ExecuteClientCommand( &svs.clients[client], command, qtrue );
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame( int time ) {
	if (!bot_enable)
		return;
	//NOTE: maybe the game is already shutdown
	if (!svs.gameStarted)
		return;
	GVM_BotAIStartFrame( time );
}

/*
===============
SV_BotLibSetup
===============
*/
int SV_BotLibSetup( void ) {
	if (!bot_enable) {
		return 0;
	}

	if ( !botlib_export ) {
		Com_Printf( S_COLOR_RED "Error: SV_BotLibSetup without SV_BotInitBotLib\n" );
		return -1;
	}

	return botlib_export->BotLibSetup();
}

/*
===============
SV_ShutdownBotLib

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
int SV_BotLibShutdown( void ) {

	if ( !botlib_export ) {
		return -1;
	}

	return botlib_export->BotLibShutdown();
}

/*
==================
SV_BotInitCvars
==================
*/
void SV_BotInitCvars(void) {

	Cvar_Get("bot_enable", "1", 0);						//enable the bot
	Cvar_Get("bot_developer", "0", CVAR_CHEAT);			//bot developer mode
	Cvar_Get("bot_debug", "0", CVAR_CHEAT);				//enable bot debugging
	Cvar_Get("bot_maxdebugpolys", "2", 0);				//maximum number of debug polys
	Cvar_Get("bot_groundonly", "1", 0);					//only show ground faces of areas
	Cvar_Get("bot_reachability", "0", 0);				//show all reachabilities to other areas
	Cvar_Get("bot_visualizejumppads", "0", CVAR_CHEAT);	//show jumppads
	Cvar_Get("bot_forceclustering", "0", 0);			//force cluster calculations
	Cvar_Get("bot_forcereachability", "0", 0);			//force reachability calculations
	Cvar_Get("bot_forcewrite", "0", 0);					//force writing aas file
	Cvar_Get("bot_aasoptimize", "0", 0);				//no aas file optimisation
	Cvar_Get("bot_saveroutingcache", "0", 0);			//save routing cache
	Cvar_Get("bot_thinktime", "100", CVAR_CHEAT);		//msec the bots thinks
	Cvar_Get("bot_reloadcharacters", "0", 0);			//reload the bot characters each time
	Cvar_Get("bot_testichat", "0", 0);					//test ichats
	Cvar_Get("bot_testrchat", "0", 0);					//test rchats
	Cvar_Get("bot_testsolid", "0", CVAR_CHEAT);			//test for solid areas
	Cvar_Get("bot_testclusters", "0", CVAR_CHEAT);		//test the AAS clusters
	Cvar_Get("bot_fastchat", "0", 0);					//fast chatting bots
	Cvar_Get("bot_nochat", "0", 0);						//disable chats
	Cvar_Get("bot_pause", "0", CVAR_CHEAT);				//pause the bots thinking
	Cvar_Get("bot_report", "0", CVAR_CHEAT);			//get a full report in ctf
	Cvar_Get("bot_grapple", "0", 0);					//enable grapple
	Cvar_Get("bot_rocketjump", "1", 0);					//enable rocket jumping
	Cvar_Get("bot_challenge", "0", 0);					//challenging bot
	Cvar_Get("bot_minplayers", "0", 0);					//minimum players in a team or the game
	Cvar_Get("bot_interbreedchar", "", CVAR_CHEAT);		//bot character used for interbreeding
	Cvar_Get("bot_interbreedbots", "10", CVAR_CHEAT);	//number of bots used for interbreeding
	Cvar_Get("bot_interbreedcycle", "20", CVAR_CHEAT);	//bot interbreeding cycle
	Cvar_Get("bot_interbreedwrite", "", CVAR_CHEAT);	//write interbreeded bots to this file
}

extern botlib_export_t *GetBotLibAPI( int apiVersion, botlib_import_t *import );

// there's no such thing as this now, since the zone is unlimited, but I have to provide something
//	so it doesn't run out of control alloc-wise (since the bot code calls this in a while() loop to free
//	up bot mem until zone has > 1MB available again. So, simulate a reasonable limit...
//
static int bot_Z_AvailableMemory(void)
{
	const int iMaxBOTLIBMem = 8 * 1024 * 1024;	// adjust accordingly.
	return iMaxBOTLIBMem - Z_MemSize( TAG_BOTLIB );
}

/*
==================
SV_BotInitBotLib
==================
*/
void SV_BotInitBotLib(void) {
	botlib_import_t	botlib_import;

	if (debugpolygons) Z_Free(debugpolygons);
	bot_maxdebugpolys = Cvar_VariableIntegerValue("bot_maxdebugpolys");
	debugpolygons = (struct bot_debugpoly_s *)Z_Malloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys, TAG_BOTLIB, qtrue);

	botlib_import.Print = BotImport_Print;
	botlib_import.Trace = BotImport_Trace;
	botlib_import.EntityTrace = BotImport_EntityTrace;
	botlib_import.PointContents = BotImport_PointContents;
	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BSPEntityData = BotImport_BSPEntityData;
	botlib_import.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botlib_import.BotClientCommand = BotClientCommand;

	//memory management
	botlib_import.GetMemory = BotImport_GetMemory;
	botlib_import.FreeMemory = BotImport_FreeMemory;
	botlib_import.AvailableMemory = bot_Z_AvailableMemory;	//Z_AvailableMemory;
	botlib_import.HunkAlloc = BotImport_HunkAlloc;

	// file system access
	botlib_import.FS_FOpenFile = FS_FOpenFileByMode;
	botlib_import.FS_Read = FS_Read;
	botlib_import.FS_Write = FS_Write;
	botlib_import.FS_FCloseFile = FS_FCloseFile;
	botlib_import.FS_Seek = FS_Seek;

	//debug lines
	botlib_import.DebugLineCreate = BotImport_DebugLineCreate;
	botlib_import.DebugLineDelete = BotImport_DebugLineDelete;
	botlib_import.DebugLineShow = BotImport_DebugLineShow;

	//debug polygons
	botlib_import.DebugPolygonCreate = BotImport_DebugPolygonCreate;
	botlib_import.DebugPolygonDelete = BotImport_DebugPolygonDelete;

	botlib_export = (botlib_export_t *)GetBotLibAPI( BOTLIB_API_VERSION, &botlib_import );
	assert(botlib_export);
}


//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage( int client, char *buf, int size )
{
	client_t	*cl;
	int			index;

	cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if ( cl->reliableAcknowledge == cl->reliableSequence ) {
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 );

	if ( !cl->reliableCommands[index][0] ) {
		return qfalse;
	}

	Q_strncpyz( buf, cl->reliableCommands[index], size );
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS( int client, int entityNum ) {
	client_t			*cl;
	clientSnapshot_t	*frame;
	int					i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	for ( i = 0; i < frame->num_entities; i++ )	{
		if ( svs.snapshotEntities[(frame->first_entity + i) % svs.numSnapshotEntities].number == entityNum ) {
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity( int client, int sequence ) {
	client_t			*cl;
	clientSnapshot_t	*frame;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if (sequence < 0 || sequence >= frame->num_entities) {
		return -1;
	}
	return svs.snapshotEntities[(frame->first_entity + sequence) % svs.numSnapshotEntities].number;
}

