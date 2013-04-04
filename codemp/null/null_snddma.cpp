
// snddma_null.c
// all other sound mixing is portable

#include "../client/client.h"

qboolean gbInsideLoadSound = qfalse; // important to default to this!!!

qboolean SNDDMA_Init(void)
{
	return qfalse;
}

int	SNDDMA_GetDMAPos(void)
{
	return 0;
}

void SNDDMA_Shutdown(void)
{
}

void SNDDMA_BeginPainting (void)
{
}

void SNDDMA_Submit(void)
{
}

sfxHandle_t S_RegisterSound( const char *name ) {
	return 0;
}

void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
}

void S_ClearSoundBuffer( void ) {
}

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean something)
{
	return qfalse;
}

int SND_FreeOldestSound(void)
{
	return 0;
}