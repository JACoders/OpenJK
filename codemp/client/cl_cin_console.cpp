
/*****************************************************************************
 * name:		cl_cin_stubs.cpp
 *
 * desc:		video and cinematic playback stubs to avoid link errors
 *
 *
 * cl_glconfig.hwtype trtypes 3dfx/ragepro need 256x256
 *
 *****************************************************************************/

#include "client.h"
//#include "../win32/win_local.h"
//#include "../win32/win_input.h"

//#ifdef _XBOX
//#include "../cgame/cg_local.h"
//#include "cl_data.h"
//#endif

void CIN_CloseAllVideos(void)
{
	return;
}

e_status CIN_StopCinematic(int handle)
{
	return FMV_EOF;
}

e_status CIN_RunCinematic(int handle)
{
	return FMV_EOF;
}

int CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits)
{
	return 0;
}

void CIN_SetExtents(int handle, int x, int y, int w, int h)
{
	return;
}

void CIN_DrawCinematic(int handle)
{
	return;
}

void SCR_DrawCinematic(void)
{
	return;
}

void SCR_RunCinematic(void)
{
	return;
}

void SCR_StopCinematic(void)
{
	return;
}

void CIN_UploadCinematic(int handle)
{
	return;
}

void CIN_Init(void)
{
	return;
}

void CL_PlayCinematic_f(void)
{
	return;
}
