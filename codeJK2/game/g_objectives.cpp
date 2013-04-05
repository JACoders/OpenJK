// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"


//g_objectives.cpp
//reads in ext_data\objectives.dat to objectives[]

#include "g_local.h"
#include "g_items.h"

#define	G_OBJECTIVES_CPP

#include "objectives.h"

qboolean	missionInfo_Updated;


/*
============
OBJ_SetPendingObjectives
============
*/
void OBJ_SetPendingObjectives(gentity_t *ent)
{
	int i;

	for (i=0;i<MAX_OBJECTIVES;++i)
	{
		if ((ent->client->sess.mission_objectives[i].status == OBJECTIVE_STAT_PENDING) && 
			(ent->client->sess.mission_objectives[i].display))
		{
			ent->client->sess.mission_objectives[i].status = OBJECTIVE_STAT_FAILED;
		}
	}
}

/*
============
OBJ_SaveMissionObjectives
============
*/
void OBJ_SaveMissionObjectives( gclient_t *client )
{
	gi.AppendToSaveGame('OBJT', client->sess.mission_objectives, sizeof(client->sess.mission_objectives));
}


/*
============
OBJ_SaveObjectiveData
============
*/
void OBJ_SaveObjectiveData(void)
{
	gclient_t *client;

	client = &level.clients[0];

	OBJ_SaveMissionObjectives( client );
}

/*
============
OBJ_LoadMissionObjectives
============
*/
void OBJ_LoadMissionObjectives( gclient_t *client )
{
	gi.ReadFromSaveGame('OBJT', (void *) &client->sess.mission_objectives, sizeof(client->sess.mission_objectives), NULL);
}


/*
============
OBJ_LoadObjectiveData
============
*/
void OBJ_LoadObjectiveData(void)
{
	gclient_t *client;

	client = &level.clients[0];

	OBJ_LoadMissionObjectives( client );
}
