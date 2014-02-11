#include "q_shared.h"
#include "qcommon.h"

typedef struct persisentData_t
{
	const void *data;
	size_t size;

	char name[MAX_QPATH];
} persisentData_t;

#define MAX_PERSISENT_DATA_STORES (16)
static persisentData_t persistentData[MAX_PERSISENT_DATA_STORES];

static persisentData_t *FindEmptyStore ( persisentData_t *stores, size_t count )
{
	for ( size_t i = 0; i < count; i++ )
	{
		if ( stores[i].data == NULL )
		{
			return &stores[i];
		}
	}

	return NULL;
}

static persisentData_t *FindStoreWithName ( persisentData_t *stores, size_t count, const char *name )
{
	for ( size_t i = 0; i < count; i++ )
	{
		if ( Q_stricmp (stores[i].name, name) == 0 )
		{
			return &stores[i];
		}
	}

	return NULL;
}

bool PD_Store ( const char *name, const void *data, size_t size )
{
	persisentData_t *store = FindEmptyStore (persistentData, MAX_PERSISENT_DATA_STORES);
	if ( store == NULL )
	{
		Com_Printf (S_COLOR_YELLOW "WARNING: No persistent data store found.\n");
		return false;
	}

	store->data = data;
	store->size = size;
	Q_strncpyz (store->name, name, sizeof (store->name));

	return true;
}

const void *PD_Load ( const char *name, size_t *size )
{
	persisentData_t *store = FindStoreWithName (persistentData, MAX_PERSISENT_DATA_STORES, name);
	if ( store == NULL )
	{
		return NULL;
	}

	const void *data = store->data;
	if ( size != NULL )
	{
		*size = store->size;
	}

	store->data = NULL;
	store->size = 0;

	return data;
}
