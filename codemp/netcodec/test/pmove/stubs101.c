// Functions the original 1.01 bg code links to and the pmove test never calls (Ghoul2, files, effects, damage, ICARUS,
// vehicles): unprototyped stubs, so that their many signatures need not be repeated. Built with pmove_det.c -DJKA101.
#define STUB(name) int name() { return 0; }
STUB(Client_CheckImpactBBrush) STUB(FighterIsLanded) STUB(G_AddEvent) STUB(G_CanBeEnemy) STUB(G_CheapWeaponFire)
STUB(G_Damage) STUB(G_DamageFromKiller) STUB(G_EffectIndex) STUB(G_FlyVehicleSurfaceDestruction) STUB(G_ModelIndex)
STUB(G_NewString) STUB(G_PlayEffect) STUB(G_PlayEffectID) STUB(G_SetAnimalVehicleFunctions) STUB(G_SetFighterVehicleFunctions)
STUB(G_SetSharedVehicleFunctions) STUB(G_SetSpeederVehicleFunctions) STUB(G_SetWalkerVehicleFunctions) STUB(G_SoundIndex)
STUB(NPC_SetAnim) STUB(Q3_SetParm) STUB(TryGrapple) STUB(WP_GetVehicleCamPos)
STUB(strap_G2API_AnimateG2Models) STUB(strap_G2API_GetBoltMatrix) STUB(strap_G2API_GetBoltMatrix_NoRecNoRot)
STUB(strap_G2API_GetBoneAnim) STUB(strap_G2API_IKMove) STUB(strap_G2API_SetBoneAngles) STUB(strap_G2API_SetBoneAnim)
STUB(strap_G2API_SetBoneIKState) STUB(trap_FS_FCloseFile) STUB(trap_FS_FOpenFile) STUB(trap_FS_GetFileList) STUB(trap_FS_Read)
STUB(trap_G2API_AddBolt) STUB(trap_G2API_CleanGhoul2Models) STUB(trap_G2API_GetBoltMatrix) STUB(trap_G2API_GetSurfaceRenderStatus)
STUB(trap_G2API_InitGhoul2Model) STUB(trap_R_RegisterSkin) STUB(trap_Trace)
