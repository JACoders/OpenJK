
/*****************************************************************************
 * name:		be_aas_main.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_main.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#ifdef AASINTERN

extern aas_t aasworld;

//AAS error message
void QDECL AAS_Error(char *fmt, ...);
//set AAS initialized
void AAS_SetInitialized(void);
//setup AAS with the given number of entities and clients
int AAS_Setup(void);
//shutdown AAS
void AAS_Shutdown(void);
//start a new map
int AAS_LoadMap(const char *mapname);
//start a new time frame
int AAS_StartFrame(float time);
#endif //AASINTERN

//returns true if AAS is initialized
int AAS_Initialized(void);
//returns true if the AAS file is loaded
int AAS_Loaded(void);
//returns the model name from the given index
char *AAS_ModelFromIndex(int index);
//returns the index from the given model name
int AAS_IndexFromModel(char *modelname);
//returns the current time
float AAS_Time(void);
//
void AAS_ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
