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

#include "cg_local.h"

int strafeHelperActiveColorModificationCount = -1;//japro
int enginePatchModificationCount = -1; //japro

//
// Cvar callbacks
//

static void CG_SVRunningChange( void ) {
	cgs.localServer = sv_running.integer;
}

static void CG_ForceModelChange( void ) {
	int i;

	for ( i=0; i<MAX_CLIENTS; i++ ) {
		const char *clientInfo = CG_ConfigString( CS_PLAYERS+i );

		if ( !VALIDSTRING( clientInfo ) )
			continue;

		CG_NewClientInfo( i, qtrue );
	}
}

static void CG_TeamOverlayChange( void ) {
	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( cg_drawTeamOverlay.integer > 0 && cgs.gametype >= GT_SINGLE_PLAYER)
		trap->Cvar_Set( "teamoverlay", "1" );
	else
		trap->Cvar_Set( "teamoverlay", "0" );
}

void CG_Set2DRatio(void) {
	if (cl_ratioFix.integer) // shared with UI module
		cgs.widthRatioCoef = (float)(SCREEN_WIDTH * cgs.glconfig.vidHeight) / (float)(SCREEN_HEIGHT * cgs.glconfig.vidWidth);
	else
		cgs.widthRatioCoef = 1.0f;
}

extern void CG_LoadHud_f(void);
static void CG_UpdateHUD(void) {
	if (cg.snap && cg_hudFiles.integer != 1 && cg_hudFiles.integer != 2)
		CG_LoadHud_f();
	else
		return;
}

//Strafehelper colors
static void CG_StrafeHelperActiveColorChange(void) {
	if (sscanf(cg_strafeHelperActiveColor.string, "%f %f %f %f", &cg.strafeHelperActiveColor[0], &cg.strafeHelperActiveColor[1], &cg.strafeHelperActiveColor[2], &cg.strafeHelperActiveColor[3]) != 4) {
		cg.strafeHelperActiveColor[0] = 0;
		cg.strafeHelperActiveColor[1] = 255;
		cg.strafeHelperActiveColor[2] = 0;
		cg.strafeHelperActiveColor[3] = 200;
	}

	if (cg.strafeHelperActiveColor[0] < 0)
		cg.strafeHelperActiveColor[0] = 0;
	else if (cg.strafeHelperActiveColor[0] > 255)
		cg.strafeHelperActiveColor[0] = 255;

	if (cg.strafeHelperActiveColor[1] < 0)
		cg.strafeHelperActiveColor[1] = 0;
	else if (cg.strafeHelperActiveColor[1] > 255)
		cg.strafeHelperActiveColor[1] = 255;

	if (cg.strafeHelperActiveColor[2] < 0)
		cg.strafeHelperActiveColor[2] = 0;
	else if (cg.strafeHelperActiveColor[2] > 255)
		cg.strafeHelperActiveColor[2] = 255;

	if (cg.strafeHelperActiveColor[3] < 25)
		cg.strafeHelperActiveColor[3] = 25;
	else if (cg.strafeHelperActiveColor[3] > 255)
		cg.strafeHelperActiveColor[3] = 255;

	trap->Cvar_Set("ui_sha_r", va("%f", cg.strafeHelperActiveColor[0]));
	trap->Cvar_Set("ui_sha_g", va("%f", cg.strafeHelperActiveColor[1]));
	trap->Cvar_Set("ui_sha_b", va("%f", cg.strafeHelperActiveColor[2]));
	trap->Cvar_Set("ui_sha_a", va("%f", cg.strafeHelperActiveColor[3]));

	cg.strafeHelperActiveColor[0] /= 255.0f;
	cg.strafeHelperActiveColor[1] /= 255.0f;
	cg.strafeHelperActiveColor[2] /= 255.0f;
	cg.strafeHelperActiveColor[3] /= 255.0f;

	//Com_Printf("New color is %f, %f, %f, %f\n", cg.strafeHelperActiveColor[0], cg.strafeHelperActiveColor[1], cg.strafeHelperActiveColor[2], cg.strafeHelperActiveColor[3]);
}

//#ifndef __linux__
#ifdef WIN32
#include "windows.h"
#define PATCH(addr, value, type) { type patch = value; MemoryPatch((void *)addr, (void *)&patch, sizeof(type)); }
void MemoryPatch(void *address, void *patch, size_t size)
{
	DWORD protect;

	VirtualProtect(address, size, PAGE_READWRITE, &protect);
	memcpy_s(address, size, patch, size);
	VirtualProtect(address, size, protect, &protect);
}

static void CG_MemoryPatchChange(void) {
	char buf[128] = { 0 };
	trap->Cvar_VariableStringBuffer("version", buf, sizeof(buf));

	if (Q_stricmp(buf, "JAmp: v1.0.1.0 win-x86 Oct 24 2003")) { //Its not the original exe i guess, so cancel the patch
																//Com_Printf("Engine patching canceled because you are not using the base .exe\n");
		return;
	}

	if (cg_engineModifications.integer > 0) { //Patch
		Com_Printf("Engine patches applied\n");
		PATCH(0x41CA05, 0x9090, unsigned short); // record patch		--Fixed in OPENJK
		PATCH(0x41CA1D, 0xEB, byte); // record patch					--Fixed in OPENJK
		PATCH(0x41A404, 0xEB, byte); // maxpackets patch				--Fixed in OPENJK
		PATCH(0x41A412, 0xEB, byte); // maxpackets patch				--Fixed in OPENJK
		PATCH(0x41D9B8, 0xB8, byte); // noCD patch						--Loda fixme
		PATCH(0x45DE61, 0xB8, byte); // noCD patch						--fixme
		PATCH(0x52C329, 0xB8, byte); // noCD patch						--fixme
		PATCH(0x414B78, 0xEB, byte); // timenudge patch					--Fixed in OPENJK+
		PATCH(0x414B84, 0xEB, byte); // timenudge patch					--Fixed in OPENJK+
		PATCH(0x48904b, 0x9090, unsigned short); // r_we patch			--Fixed in OPENJK
		PATCH(0x41DACB, 0x03, byte); // black screen connect failure patch
	}
	else //Unpatch
	{
		Com_Printf("Engine patches removed\n");
		PATCH(0x41CA05, 0x7418, unsigned short); // record unpatch*******	
		PATCH(0x41CA1D, 0x7A, byte); // record unpatch******
		PATCH(0x41A404, 0x7D, byte); // maxpackets unpatch
		PATCH(0x41A412, 0x7E, byte); // maxpackets unpatch
		PATCH(0x41D9B8, 0xE8, byte); // noCD unpatch
		PATCH(0x45DE61, 0xE8, byte); // noCD unpatch
		PATCH(0x52C329, 0xE8, byte); // noCD unpatch
		PATCH(0x414B78, 0x7D, byte); // timenudge unpatch
		PATCH(0x414B84, 0x7E, byte); // timenudge unpatch
		PATCH(0x48904b, 0x7440, unsigned short); // r_we unpatch*******
		PATCH(0x41DACB, 0x01, byte); // black screen connect failure unpatch
	}
}
#endif

//
// Cvar table
//

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	uint32_t	cvarFlags;
} cvarTable_t;

#define XCVAR_DECL
	#include "cg_xcvar.h"
#undef XCVAR_DECL

static const cvarTable_t cvarTable[] = {
	#define XCVAR_LIST
		#include "cg_xcvar.h"
	#undef XCVAR_LIST
};
static const size_t cvarTableSize = ARRAY_LEN( cvarTable );

void CG_RegisterCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void CG_UpdateCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}
