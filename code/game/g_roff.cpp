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

#include "g_local.h"
#include "g_roff.h"
#include "Q3_Interface.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"
// The list of precached ROFFs
roff_list_t	roffs[MAX_ROFFS];
int			num_roffs = 0;

qboolean g_bCollidableRoffs = qfalse;

extern void	Q3_TaskIDComplete( gentity_t *ent, taskID_t taskType );

static void G_RoffNotetrackCallback( gentity_t *cent, const char *notetrack)
{
	int i = 0, r = 0, r2 = 0, objectID = 0, anglesGathered = 0, posoffsetGathered = 0;
	char type[256];
	char argument[512];
	char addlArg[512];
	char errMsg[256];
	char t[64];
	char teststr[256];
	int addlArgs = 0;
	vec3_t parsedAngles, parsedOffset, useAngles, useOrigin, forward, right, up;

	if (!cent || !notetrack)
	{
		return;
	}

	//notetrack = "effect effects/explosion1.efx 0+0+64 0-0-1";

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if (notetrack[i] != ' ')
	{ //didn't pass in a valid notetrack type, or forgot the argument for it
		return;
	}

	i++;

	while (notetrack[i] && notetrack[i] != ' ')
	{
		if (notetrack[i] != '\n' && notetrack[i] != '\r')
		{ //don't read line ends for an argument
			argument[r] = notetrack[i];
			r++;
		}
		i++;
	}
	argument[r] = '\0';

	if (!r)
	{
		return;
	}

	if (notetrack[i] == ' ')
	{ //additional arguments...
		addlArgs = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addlArg[r] = notetrack[i];
			r++;
			i++;
		}
		addlArg[r] = '\0';
	}

	if (strcmp(type, "effect") == 0)
	{
		if (!addlArgs)
		{
			VectorClear(parsedOffset);
			goto defaultoffsetposition;
		}

		i = 0;

		while (posoffsetGathered < 3)
		{
			r = 0;
			while (addlArg[i] && addlArg[i] != '+' && addlArg[i] != ' ')
			{
				t[r] = addlArg[i];
				r++;
				i++;
			}
			t[r] = '\0';
			i++;
			if (!r)
			{ //failure..
				VectorClear(parsedOffset);
				i = 0;
				goto defaultoffsetposition;
			}
			parsedOffset[posoffsetGathered] = atof(t);
			posoffsetGathered++;
		}

		if (posoffsetGathered < 3)
		{
			sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
			goto functionend;
		}

		i--;

		if (addlArg[i] != ' ')
		{
			addlArgs = 0;
		}

defaultoffsetposition:

		r = 0;
		if (argument[r] == '/')
		{
			r++;
		}
		while (argument[r] && argument[r] != '/')
		{
			teststr[r2] = argument[r];
			r2++;
			r++;
		}
		teststr[r2] = '\0';

		if (r2 && strstr(teststr, "effects"))
		{ //get rid of the leading "effects" since it's auto-inserted
			r++;
			r2 = 0;

			while (argument[r])
			{
				teststr[r2] = argument[r];
				r2++;
				r++;
			}
			teststr[r2] = '\0';

			strcpy(argument, teststr);
		}

		objectID = G_EffectIndex(argument);
		r = 0;

		if (objectID)
		{
			if (addlArgs)
			{ //if there is an additional argument for an effect it is expected to be XANGLE-YANGLE-ZANGLE
				i++;
				while (anglesGathered < 3)
				{
					r = 0;
					while (addlArg[i] && addlArg[i] != '-')
					{
						t[r] = addlArg[i];
						r++;
						i++;
					}
					t[r] = '\0';
					i++;

					if (!r)
					{ //failed to get a new part of the vector
						anglesGathered = 0;
						break;
					}

					parsedAngles[anglesGathered] = atof(t);
					anglesGathered++;
				}

				if (anglesGathered)
				{
					VectorCopy(parsedAngles, useAngles);
				}
				else
				{ //failed to parse angles from the extra argument provided..
					VectorCopy(cent->s.apos.trBase, useAngles);
				}
			}
			else
			{ //if no constant angles, play in direction entity is facing
				VectorCopy(cent->s.apos.trBase, useAngles);
			}

			AngleVectors(useAngles, forward, right, up);

			VectorCopy(cent->s.pos.trBase, useOrigin);

			//forward
			useOrigin[0] += forward[0]*parsedOffset[0];
			useOrigin[1] += forward[1]*parsedOffset[0];
			useOrigin[2] += forward[2]*parsedOffset[0];

			//right
			useOrigin[0] += right[0]*parsedOffset[1];
			useOrigin[1] += right[1]*parsedOffset[1];
			useOrigin[2] += right[2]*parsedOffset[1];

			//up
			useOrigin[0] += up[0]*parsedOffset[2];
			useOrigin[1] += up[1]*parsedOffset[2];
			useOrigin[2] += up[2]*parsedOffset[2];

			G_PlayEffect(objectID, useOrigin, useAngles);
		}
	}
	else if (strcmp(type, "sound") == 0)
	{
		objectID = G_SoundIndex(argument);
		cgi_S_StartSound(cent->s.pos.trBase, cent->s.number, CHAN_BODY, objectID);
	}
	//else if ...
	else
	{
		if (type[0])
		{
			Com_Printf("Warning: \"%s\" is an invalid ROFF notetrack function\n", type);
		}
		else
		{
			Com_Printf("Warning: Notetrack is missing function and/or arguments\n");
		}
	}

	return;

functionend:
	Com_Printf("Type-specific notetrack error: %s\n", errMsg);
	return;
}

static qboolean G_ValidRoff( roff_hdr2_t *header )
{
	if ( !strncmp( header->mHeader, "ROFF", 4 ))
	{
		if ( LittleLong(header->mVersion) == ROFF_VERSION2 && LittleLong(header->mCount) > 0 )
		{
			return qtrue;
		}
		else if ( LittleLong(header->mVersion) == ROFF_VERSION && LittleFloat(((roff_hdr_t*)header)->mCount) > 0.0f )
		{ // version 1 defines the count as a float, so we best do the count check as a float or we'll get bogus results
			return qtrue;
		}
	}

	return qfalse;
}

static void G_FreeRoff(int index)
{
	if(roffs[index].mNumNoteTracks) {
		delete [] roffs[index].mNoteTrackIndexes[0];
		delete [] roffs[index].mNoteTrackIndexes;
	}
}

static qboolean G_InitRoff( char *file, unsigned char *data )
{
	roff_hdr_t *header = (roff_hdr_t *)data;
	int	count;
	int i;

	roffs[num_roffs].fileName = G_NewString( file );

	if ( LittleLong(header->mVersion) == ROFF_VERSION )
	{
		count = (int)LittleFloat(header->mCount);

		// We are Old School(tm)
		roffs[num_roffs].type = 1;

		roffs[num_roffs].data = (void *) G_Alloc( count * sizeof( move_rotate_t ) );
		move_rotate_t *mem	= (move_rotate_t *)roffs[num_roffs].data;

		roffs[num_roffs].mFrameTime = 100; // old school ones have a hard-coded frame time
		roffs[num_roffs].mLerp = 10;
		roffs[num_roffs].mNumNoteTracks = 0;
		roffs[num_roffs].mNoteTrackIndexes = NULL;

		if ( mem )
		{
			// The allocation worked, so stash this stuff off so we can reference the data later if needed
			roffs[num_roffs].frames		= count;

			// Step past the header to get to the goods
			move_rotate_t *roff_data = ( move_rotate_t *)&header[1];

			// Copy all of the goods into our ROFF cache
			for ( i = 0; i < count; i++, roff_data++, mem++ )
			{
				// Copy just the delta position and orientation which can be applied to anything at a later point
#ifdef Q3_BIG_ENDIAN
				mem->origin_delta[0] = LittleFloat(roff_data->origin_delta[0]);
				mem->origin_delta[1] = LittleFloat(roff_data->origin_delta[1]);
				mem->origin_delta[2] = LittleFloat(roff_data->origin_delta[2]);
				mem->rotate_delta[0] = LittleFloat(roff_data->rotate_delta[0]);
				mem->rotate_delta[1] = LittleFloat(roff_data->rotate_delta[1]);
				mem->rotate_delta[2] = LittleFloat(roff_data->rotate_delta[2]);
#else
				VectorCopy( roff_data->origin_delta, mem->origin_delta );
				VectorCopy( roff_data->rotate_delta, mem->rotate_delta );
#endif
			}
			return qtrue;
		}
	}
	else if ( LittleLong(header->mVersion) == ROFF_VERSION2 )
	{
		// Version 2.0, heck yeah!
		roff_hdr2_t *hdr = (roff_hdr2_t *)data;
		count = LittleLong(hdr->mCount);

		roffs[num_roffs].frames	= count;
		roffs[num_roffs].data	= (void *) G_Alloc( count * sizeof( move_rotate2_t ));		
		move_rotate2_t *mem		= (move_rotate2_t *)roffs[num_roffs].data;

		if ( mem )
		{
			roffs[num_roffs].mFrameTime			= LittleLong(hdr->mFrameRate);
			roffs[num_roffs].mLerp				= 1000 / LittleLong(hdr->mFrameRate);
			roffs[num_roffs].mNumNoteTracks		= LittleLong(hdr->mNumNotes);

			if (roffs[num_roffs].mFrameTime < 50)
			{
				Com_Printf(S_COLOR_RED"Error: \"%s\" has an invalid ROFF framerate (%d < 50)\n", file, roffs[num_roffs].mFrameTime);
			}
			assert( roffs[num_roffs].mFrameTime >= 50 );//HAS to be at least 50 to be reliable

			 // Step past the header to get to the goods
			move_rotate2_t *roff_data = ( move_rotate2_t *)&hdr[1];

			roffs[num_roffs].type = 2; //rww - any reason this wasn't being set already?

			// Copy all of the goods into our ROFF cache
			for ( i = 0; i < count; i++ )
			{
#ifdef Q3_BIG_ENDIAN
				mem[i].origin_delta[0] = LittleFloat(roff_data[i].origin_delta[0]);
				mem[i].origin_delta[1] = LittleFloat(roff_data[i].origin_delta[1]);
				mem[i].origin_delta[2] = LittleFloat(roff_data[i].origin_delta[2]);
				mem[i].rotate_delta[0] = LittleFloat(roff_data[i].rotate_delta[0]);
				mem[i].rotate_delta[1] = LittleFloat(roff_data[i].rotate_delta[1]);
				mem[i].rotate_delta[2] = LittleFloat(roff_data[i].rotate_delta[2]);
#else
				VectorCopy( roff_data[i].origin_delta, mem[i].origin_delta );
				VectorCopy( roff_data[i].rotate_delta, mem[i].rotate_delta );
#endif

				mem[i].mStartNote = LittleLong(roff_data[i].mStartNote);
				mem[i].mNumNotes = LittleLong(roff_data[i].mNumNotes);
			}

			if ( LittleLong(hdr->mNumNotes) )
			{
				int		size;
				char	*ptr, *start;

				ptr = start = (char *)&roff_data[i];
				size = 0;

				for( i = 0; i < LittleLong(hdr->mNumNotes); i++ )
				{
					size += strlen(ptr) + 1;
					ptr += strlen(ptr) + 1;
				}

				// ? Get rid of dynamic memory ?
				roffs[num_roffs].mNoteTrackIndexes = new char *[LittleLong(hdr->mNumNotes)];
				ptr = roffs[num_roffs].mNoteTrackIndexes[0] = new char[size];
				memcpy(roffs[num_roffs].mNoteTrackIndexes[0], start, size);

				for( i = 1; i < LittleLong(hdr->mNumNotes); i++ )
				{
					ptr += strlen(ptr) + 1;
					roffs[num_roffs].mNoteTrackIndexes[i] = ptr;
				}
			}
			return qtrue;
		}
	}

	return false;
}

//-------------------------------------------------------
// G_LoadRoff
//
// Does the fun work of loading and caching a roff file
//	If the file is already cached, it just returns an
//	ID to the cached file.
//-------------------------------------------------------

int G_LoadRoff( const char *fileName )
{
	char	file[MAX_QPATH];
	byte	*data;
	int		len, i, roff_id = 0;

	// Before even bothering with all of this, make sure we have a place to store it.
	if ( num_roffs >= MAX_ROFFS )
	{
		Com_Printf( S_COLOR_RED"MAX_ROFFS count exceeded.  Skipping load of .ROF '%s'\n", fileName );
		return roff_id;
	}

	// The actual path
	sprintf( file, "%s/%s.rof", Q3_SCRIPT_DIR, fileName );

	// See if I'm already precached
	for ( i = 0; i < num_roffs; i++ )
	{
		if ( Q_stricmp( file, roffs[i].fileName ) == 0 )
		{
			// Good, just return me...avoid zero index
			return i + 1;
		}
	}

#ifdef _DEBUG
//	Com_Printf( S_COLOR_GREEN"Caching ROF: '%s'\n", file );
#endif

	// Read the file in one fell swoop
	len = gi.FS_ReadFile( file, (void**) &data);

	if ( len <= 0 )
	{
		Com_Printf( S_COLOR_RED"Could not open .ROF file '%s'\n", fileName );
		return roff_id;
	}

	// Now let's check the header info...
	roff_hdr2_t *header = (roff_hdr2_t *)data;

	// ..and make sure it's reasonably valid
	if ( !G_ValidRoff( header ))
	{
		Com_Printf( S_COLOR_RED"Invalid roff format '%s'\n", fileName );
	}
	else
	{
		G_InitRoff( file, data );

		// Done loading this roff, so save off an id to it..increment first to avoid zero index
		roff_id = ++num_roffs;
	}

	gi.FS_FreeFile( data );

	return roff_id;
}


void G_FreeRoffs(void)
{
	while(num_roffs) {
		G_FreeRoff(num_roffs - 1);
		num_roffs--;
	}
}


//-------------------------------------------------------
// G_Roff
//
// Handles applying the roff data to the specified ent
//-------------------------------------------------------

void G_Roff( gentity_t *ent )
{
	if ( !ent->next_roff_time )
	{
		return;
	}
	
	if ( ent->next_roff_time > level.time )
	{// either I don't think or it's just not time to have me think yet
		return;
	}

	const int roff_id = G_LoadRoff( ent->roff );

	if ( !roff_id )
	{	// Couldn't cache this rof
		return;
	}

	// The ID is one higher than the array index
	const roff_list_t *  roff	= &roffs[ roff_id - 1 ];
	vec3_t	org, ang;

	if ( roff->type == 2 )
	{
		move_rotate2_t	*data	= &((move_rotate2_t *)roff->data)[ ent->roff_ctr ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );
		if (data->mStartNote != -1 || data->mNumNotes)
		{
			G_RoffNotetrackCallback(ent, roffs[roff_id - 1].mNoteTrackIndexes[data->mStartNote]);
		}
	}
	else
	{
		move_rotate_t	*data	= &((move_rotate_t *)roff->data)[ ent->roff_ctr ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );
	}

#ifdef _DEBUG
	if ( g_developer->integer )
	{
		Com_Printf( S_COLOR_GREEN"ROFF dat: num: %d o:<%.2f %.2f %.2f> a:<%.2f %.2f %.2f>\n", 
					ent->roff_ctr,
					org[0], org[1], org[2],
					ang[0], ang[1], ang[2] );
	}
#endif

	if ( ent->client )
	{
		// Set up the angle interpolation
		//-------------------------------------
		VectorAdd( ent->s.apos.trBase, ang, ent->s.apos.trBase );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_INTERPOLATE;

		// Store what the next apos->trBase should be
		VectorCopy( ent->s.apos.trBase, ent->client->ps.viewangles );
		VectorCopy( ent->s.apos.trBase, ent->currentAngles );
		VectorCopy( ent->s.apos.trBase, ent->s.angles );
		if ( ent->NPC )
		{
			//ent->NPC->desiredPitch = ent->s.apos.trBase[PITCH];
			ent->NPC->desiredYaw = ent->s.apos.trBase[YAW];
		}

		// Set up the origin interpolation
		//-------------------------------------
		VectorAdd( ent->s.pos.trBase, org, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trType = TR_INTERPOLATE;

		// Store what the next pos->trBase should be
		VectorCopy( ent->s.pos.trBase, ent->client->ps.origin );
		VectorCopy( ent->s.pos.trBase, ent->currentOrigin );
		//VectorCopy( ent->s.pos.trBase, ent->s.origin );
	}
	else
	{
		// Set up the angle interpolation
		//-------------------------------------
		VectorScale( ang, roff->mLerp, ent->s.apos.trDelta );
		VectorCopy( ent->pos2, ent->s.apos.trBase );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_LINEAR;

		// Store what the next apos->trBase should be
		VectorAdd( ent->pos2, ang, ent->pos2 );

		// Set up the origin interpolation
		//-------------------------------------
		VectorScale( org, roff->mLerp, ent->s.pos.trDelta );
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trType = TR_LINEAR;

		// Store what the next apos->trBase should be
		VectorAdd( ent->pos1, org, ent->pos1 );

		//make it true linear... FIXME: sticks around after ROFF is done, but do we really care?
		ent->alt_fire = qtrue;

		if ( ent->e_ThinkFunc == thinkF_TieFighterThink || ent->e_ThinkFunc == thinkF_TieBomberThink ||
			( !ent->e_ThinkFunc
			&& ent->s.eType != ET_MISSILE
			&& ent->s.eType != ET_ITEM
			&& ent->s.eType != ET_MOVER ) )
		{//will never set currentAngles & currentOrigin itself ( why do we limit which one's get set?, just set all the time? )
			EvaluateTrajectory( &ent->s.apos, level.time, ent->currentAngles );
			EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
		}
	}

	// Link just in case.
	gi.linkentity( ent );

	// See if the ROFF playback is done
	//-------------------------------------
	if ( ++ent->roff_ctr >= roff->frames )
	{
		// We are done, so let me think no more, then tell the task that we're done.
		ent->next_roff_time = 0;

		// Stop any rotation or movement.
		VectorClear( ent->s.pos.trDelta );
		VectorClear( ent->s.apos.trDelta );

		Q3_TaskIDComplete( ent, TID_MOVE_NAV );

		return;
	}

	ent->next_roff_time = level.time + roff->mFrameTime;
}


//-------------------------------------------------------
// G_SaveCachedRoffs
//
// Really fun savegame stuff
//-------------------------------------------------------

void G_SaveCachedRoffs()
{
	int i, len;

	// Write out the number of cached ROFFs
	gi.AppendToSaveGame( INT_ID('R','O','F','F'), (void *)&num_roffs, sizeof(num_roffs) );

	// Now dump out the cached ROFF file names in order so they can be loaded on the other end
	for ( i = 0; i < num_roffs; i++ )
	{
		// Dump out the string length to make things a bit easier on the other end...heh heh.
		len = strlen( roffs[i].fileName ) + 1;
		gi.AppendToSaveGame( INT_ID('S','L','E','N'), (void *)&len, sizeof(len) );
		gi.AppendToSaveGame( INT_ID('R','S','T','R'), (void *)roffs[i].fileName, len );
	}
}


//-------------------------------------------------------
// G_LoadCachedRoffs
//
// Really fun loadgame stuff
//-------------------------------------------------------

void G_LoadCachedRoffs()
{
	int		i, count, len;
	char	buffer[MAX_QPATH];

	// Get the count of goodies we need to revive
	gi.ReadFromSaveGame( INT_ID('R','O','F','F'), (void *)&count, sizeof(count), NULL );

	// Now bring 'em back to life
	for ( i = 0; i < count; i++ )
	{
		gi.ReadFromSaveGame( INT_ID('S','L','E','N'), (void *)&len, sizeof(len), NULL );
		gi.ReadFromSaveGame( INT_ID('R','S','T','R'), (void *)(buffer), len, NULL );
		G_LoadRoff( buffer );
	}
}
