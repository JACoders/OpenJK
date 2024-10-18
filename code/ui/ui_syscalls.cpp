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

#include "../server/exe_headers.h"
#include "ui_local.h"

void trap_R_ClearScene(void) {
    ui.R_ClearScene();
}

void trap_R_AddRefEntityToScene(const refEntity_t *re) {
    if (!re) return; 
    ui.R_AddRefEntityToScene(re);
}

void trap_R_RenderScene(const refdef_t *fd) {
    if (!fd) return; 
    ui.R_RenderScene(fd);
}

void trap_R_SetColor(const float *rgba) {
    if (!rgba) return; 
    ui.R_SetColor(rgba);
}

void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) {
    if (hShader < 0) return; 
    ui.R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
}

void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) {
    if (!mins || !maxs) return; 
    ui.R_ModelBounds(model, mins, maxs);
}

void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum) {
    if (sfx < 0) return; 
    S_StartLocalSound(sfx, channelNum);
}

void trap_S_StopSounds(void) {
    S_StopSounds();
}

sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed) {
    if (!sample || sample[0] == '\0') return -1; 
    return S_RegisterSound(sample);
}

void trap_Key_SetBinding(int keynum, const char *binding) {
    if (!binding) return; 
    Key_SetBinding(keynum, binding);
}

qboolean trap_Key_GetOverstrikeMode(void) {
    return Key_GetOverstrikeMode();
}

void trap_Key_SetOverstrikeMode(qboolean state) {
    Key_SetOverstrikeMode(state);
}

void trap_Key_ClearStates(void) {
    Key_ClearStates();
}

int trap_Key_GetCatcher(void) {
    return Key_GetCatcher();
}

void trap_Key_SetCatcher(int catcher) {
    Key_SetCatcher(catcher);
}

void trap_GetGlconfig(glconfig_t *glconfig) {
    if (!glconfig) return; 
    CL_GetGlconfig(glconfig);
}

int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile) {
    if (!arg0 || arg0[0] == '\0') return -1; 
    return CIN_PlayCinematic(arg0, xpos, ypos, width, height, bits, psAudioFile);
}

int trap_CIN_StopCinematic(int handle) {
    if (handle < 0) return -1; 
    return CIN_StopCinematic(handle);
}

