
/*****************************************************************************
 * name:		be_aas_entity.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_entity.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#ifdef AASINTERN
//invalidates all entity infos
void AAS_InvalidateEntities(void);
//unlink not updated entities
void AAS_UnlinkInvalidEntities(void);
//resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks(void);
//updates an entity
int AAS_UpdateEntity(int ent, bot_entitystate_t *state);
//gives the entity data used for collision detection
void AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata);
#endif //AASINTERN

//returns the size of the entity bounding box in mins and maxs
void AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs);
//returns the BSP model number of the entity
int AAS_EntityModelNum(int entnum);
//returns the origin of an entity with the given model number
int AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin);
//returns the best reachable area the entity is situated in
int AAS_BestReachableEntityArea(int entnum);
//returns the info of the given entity
void AAS_EntityInfo(int entnum, aas_entityinfo_t *info);
//returns the next entity
int AAS_NextEntity(int entnum);
//returns the origin of the entity
void AAS_EntityOrigin(int entnum, vec3_t origin);
//returns the entity type
int AAS_EntityType(int entnum);
//returns the model index of the entity
int AAS_EntityModelindex(int entnum);

