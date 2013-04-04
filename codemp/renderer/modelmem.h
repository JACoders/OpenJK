//
// ModelMem.h
//

// OK, here's the deal with this class...
// At game initialization time, this class sets aside a certain amount of memory
// strictly for use by player models.  6 of these slots are of the size of the 
// largest jedi player model, and the other 6 are of the size of the largest
// non-jedi player model (since there are only 6 jedi_xx models).  Whenever a 
// player model is allocated/deallocated, it uses only the memory that is managed
// by this class.  This is done to reduce memory fragmentation that would 
// normally occour when many players join and leave a multiplayer game.
// Only the first 8 slots are allocated at first (only 8 players in a normal
// multiplayer game), with the other 4 being allocated only if a connection
// to a dedicated server is detected.


//#define MAX_MODEL_JEDI_SIZE 786740 //Amount used for UI slot.
#define MAX_MODEL_SLOTS 11

#include "../client/cl_data.h"
#include "../renderer/qgl_console.h"

extern void RE_RemoveModelFromHash(const char *name);

extern int uiClientNum;

typedef struct modelSlot_s
{
	void	*memory;
	int		allocatedSize;
	bool	inUse;
	int 	modelID;
//	int		refCount;
	char	name[64];
}modelSlot_t;

class ModelMemoryManager
{
	modelSlot_s		modelSlot[MAX_MODEL_SLOTS];
	int				numUsedSlots;
	bool			NPCMode;

private:

	void FreeModelSlot(int index)
	{
		assert(index >=0 && index < MAX_MODEL_SLOTS);

		if(modelSlot[index].memory) {
			HeapFree(GetProcessHeap(), 0, modelSlot[index].memory);
			numUsedSlots--;
		}
		memset(&modelSlot[index], 0, sizeof(modelSlot[index]));

		assert(numUsedSlots >= 0);
	}


	void AllocateModelSlot(int index, int size, int ID, const char *name)
	{
		assert(index >=0 && index < MAX_MODEL_SLOTS);

		if(modelSlot[index].memory) {
			FreeModelSlot(index);
		}

		modelSlot[index].memory = HeapAlloc(GetProcessHeap(), 0, size);

		if(!modelSlot[index].memory) {
			assert(0);
			//Something used all our heap memory.  That's bad.  Make the
			//screen green.
			Com_PrintfAlways("Model manager out of memory trying to allocate %d bytes for %s\n", size, name);
			for (;;)
			{
				qglBeginFrame();
				qglClearColor(0, 1, 0, 1);
				qglClear(GL_COLOR_BUFFER_BIT);
				qglEndFrame();
			}
		}
		modelSlot[index].allocatedSize = size;
		modelSlot[index].inUse = true;
		modelSlot[index].modelID = ID;
//		modelSlot[index].refCount = 1;
		strcpy(modelSlot[index].name, name);
		numUsedSlots++;

		assert(numUsedSlots <= MAX_MODEL_SLOTS);
	}


public:
	char			uiName[64];
	char			uiSkin[64];

	ModelMemoryManager(void)
	{
		memset(modelSlot, 0, sizeof(modelSlot[0]) * MAX_MODEL_SLOTS);
		uiName[0] = 0;
		uiSkin[0] = 0;
	}

	void AllocateModelSlots()
	{
		numUsedSlots = 0;
	}

	void SetNPCMode(bool npcMode)
	{
		NPCMode = npcMode;
	}

	bool IsNPCMode()
	{
		return NPCMode;
	}

	void* GetModelMemory(int size, int ID, const char *name)
	{
		// Find the first non-used slot with enough memory
		// Work backwards to try and use smaller slots first
		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if((strcmp(name, modelSlot[i].name) == 0) && modelSlot[i].inUse == true)
			{
				// The only time this will happen is if there is a holdover model
				// left from the UI - so don't increase the refcount
				return modelSlot[i].memory;
			}
		}

		// No slot found, so scan thru the client info to see if a slot can be killed
		bool bFound;
		for(i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			// The server NEVER throws out kyle
			if(com_sv_running->integer && !strcmp(modelSlot[i].name, "models/players/kyle/model.glm"))
				continue;

			bFound = false;
			for(int j = 0; j < cgs.maxclients; j++)
			{
				if(strlen(cgs.clientinfo[j].modelName)) 
				{
                    if(!strcmp(modelSlot[i].name, va("models/players/%s/model.glm", cgs.clientinfo[j].modelName)))
					{
						bFound = true;
						break;
					}
				}
			}

			if(strlen(uiName) && !strcmp(modelSlot[i].name, uiName))
				bFound = true;

			if(!bFound && modelSlot[i].inUse)
			{
				// This model slot is not listed in the clientinfo, kill it
				RE_RemoveModelFromHash(modelSlot[i].name);

				FreeModelSlot(i);
			}
		}   

		for(i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if(modelSlot[i].inUse == false)
			{
				AllocateModelSlot(i, size, ID, name);
				return modelSlot[i].memory;
			}
		}
		// Something horrible happened if we got here.  All model slots are
		// in use by active clients and we're trying to allocate another
		// one.  Find out why and prevent that.
		assert(0);

		// Make some debug spew with the hopes of finding the problem if it
		// gets to QA.
		Com_PrintfAlways("Hi, I'm about to crash.  Here's why.  I was told \
				to allocate memory for a new model: %s.  But all my model \
				slots are already in use.  Here's what's using them.  Good \
				luck!\n\n", name);
		for(i=0; i<MAX_MODEL_SLOTS; i++) {
			if(modelSlot[i].inUse) {
				Com_PrintfAlways("%s\n", modelSlot[i].name);
			}
		}
		Com_PrintfAlways("\nI'm done spewing now.  Any future messages didn't \
				come from the model manager.  Verbose messages are fun!\n");
		return NULL;
	}
/*
	void ModelAddRef(const char *name)
	{
		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if(strcmp(modelSlot[i].name, name) == 0)
				modelSlot[i].refCount++;
		}
	}
*/
/*
	void FreeModelMemory(int ID)
	{
		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if(modelSlot[i].modelID == ID && modelSlot[i].inUse == true)
			{
				modelSlot[i].refCount--;

				if( modelSlot[i].refCount < 0 )
					Com_Error( ERR_DROP, "FreeModelMemory by ID: refCount is negative" );

				if(modelSlot[i].refCount < 1)
				{
					RE_RemoveModelFromHash(modelSlot[i].name);
					FreeModelSlot(i);
				}
			}
		}
	}
*/

	void FreeModelMemory(const char *name)
	{
		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if((strcmp(name, modelSlot[i].name) == 0) && modelSlot[i].inUse == true)
			{
				int timesFound = 0;
				for( int j = 0; j < cgs.maxclients; ++j )
					if(!strcmp(modelSlot[i].name, va("models/players/%s/model.glm", cgs.clientinfo[j].modelName)))
						timesFound++;

				if( !strcmp(modelSlot[i].name, uiName) )
					timesFound++;

				if( com_sv_running->integer && !strcmp(modelSlot[i].name, "models/players/kyle/model.glm") )
					timesFound++;

				if( !timesFound )
					Com_Error( ERR_DROP, "FreeModelMemory by name: refCount is negative" );

				if( timesFound == 1 )
				{
					RE_RemoveModelFromHash(name);
					FreeModelSlot(i);
				}
			}
		}
	}

	// Returns a bool to indicate whether or not an erase was done on the map<>
	bool ClearModelMemory(int ID)
	{
		bool bRemovedFromHash = false;

		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			if(modelSlot[i].modelID == ID && modelSlot[i].inUse == true)
			{
				RE_RemoveModelFromHash(modelSlot[i].name);

				FreeModelSlot(i);
				bRemovedFromHash = true;
			}
		}

		return bRemovedFromHash;
	}

	void SetUIName( const char *name )
	{
		if( name )
			strcpy( uiName, name );
		else
			uiName[0] = 0;
	}

	void SetUISkin( const char *name )
	{
		if( name )
			strcpy( uiSkin, name );
		else
			uiSkin[0] = 0;
	}

	const char *GetUISkin( void )
	{
		return uiSkin;
	}

	void ClearAll()
	{
		for(int i = 0; i < MAX_MODEL_SLOTS; i++)
		{
			FreeModelSlot(i);
		}

		numUsedSlots = 0;
		uiName[0] = 0;
		uiSkin[0] = 0;
	}
};

extern ModelMemoryManager ModelMem;

