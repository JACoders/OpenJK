/*
===========================================================================
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

#pragma once
#ifndef ICARUSINTERFACE_DEFINED
#define ICARUSINTERFACE_DEFINED

#include "../qcommon/q_shared.h"

// IcarusInterface.h: ICARUS Interface header file.
// -Date: ~October, 2002
// -Created by: Mike Crowns and Aurelio Reis.
// -Description: The new interface between a Game Engine and the Icarus Scripting Language.
// An Interface is an Abstract Base Class with pure virtual members that MUST be implemented
// in order for the compile to succeed. Because of this, all needed functionality can be
// added without compromising other core systems.
// -Usage: To use the new Icarus Interface, two classes must be derived. The first is the
// actual Icarus Interface class which contains all relevent functionality to the scripting
// system. The second is the Game Interface which is very much more broad and thus implemented
// by the user. Icarus functions by calling the Game Interface to do certain tasks for it. This
// is why the Game Interface is required to have certain functions implemented.


// The basic Icarus Interface ABC.
class IIcarusInterface
{
public:
	enum { ICARUS_INVALID = 0 };
	virtual ~IIcarusInterface();

	// Get a static singleton instance (of a specific flavor).
	static IIcarusInterface* GetIcarus(int flavor = 0,bool constructIfNecessary=true);		// must be implemented along with concrete class
	static void DestroyIcarus();		// Destroy the static singleton instance.

	virtual int GetFlavor() = 0;

	virtual int Save() = 0;				// Save all Icarus states.
	virtual int Load() = 0;				// Load all Icarus states.

	virtual int Run(int icarusID, char* buffer, long length) = 0;	// Execute a script.
	virtual void DeleteIcarusID(int &icarusID) = 0;				// Delete an Icarus ID from the list (making the ID Invalid on the other end through reference).
	virtual int GetIcarusID(int gameID) = 0;					// Get an Icarus ID.
	virtual int Update( int icarusID ) = 0;						// Update all internal Icarus structures.
	virtual int IsRunning( int icarusID ) = 0;					// Whether a Icarus is running or not.
	virtual void Completed( int icarusID, int taskID ) = 0;		// Tells Icarus a task is completed.
	virtual void Precache( char* buffer, long length ) = 0;		// Precache a Script in memory.
};

// Description: The Game Interface is used by the Icarus Interface to access specific
// data or initiate certain things within the engine being used. It is made to be
// as generic as possible to allow any engine to derive it's own interface for use.
// Created: 10/08/02 by Aurelio Reis.
class IGameInterface
{
protected:
	// Pure Virtual Destructor.
	virtual ~IGameInterface();

public:
	//For system-wide prints
	enum e_DebugPrintLevel { WL_ERROR = 1, WL_WARNING, WL_VERBOSE, WL_DEBUG };

	// How many flavors are needed.
	static int s_IcarusFlavorsNeeded;

	// Get a static singleton instance (of a specific flavor).
	static IGameInterface *GetGame( int flavor = 0 );

	// Destroy the static singleton instance (NOTE: Destroy the Game Interface BEFORE the Icarus Interface).
	static void Destroy();

	// General
	// Load a script File into the destination buffer. If the script has already been loaded
	// NOTE: This is what was called before:
	/*
	// Description	: Reads in a file and attaches the script directory properly
	extern int ICARUS_GetScript( const char *name, char **buf );	//g_icarus.cpp
	static int Q3_ReadScript( const char *name, void **buf )
	{
		return ICARUS_GetScript( va( "%s/%s", Q3_SCRIPT_DIR, name ), (char**)buf );	//get a (hopefully) cached file
	}
	*/
	virtual int		GetFlavor() = 0;

	virtual int		LoadFile( const char *name, void **buf ) = 0;
	virtual void	CenterPrint( const char *format, ... ) = 0;
	virtual void	DebugPrint( e_DebugPrintLevel, const char *, ... ) = 0;
	virtual unsigned int GetTime( void ) = 0;							//Gets the current time
	virtual int 	PlayIcarusSound( int taskID, int gameID, const char *name, const char *channel ) = 0;
	virtual void	Lerp2Pos( int taskID, int gameID, float origin[3], float angles[3], float duration ) = 0;
	virtual void	Lerp2Angles( int taskID, int gameID, float angles[3], float duration ) = 0;
	virtual int		GetTag( int gameID, const char *name, int lookup, float info[3] ) = 0;
	virtual void	Set( int taskID, int gameID, const char *type_name, const char *data ) = 0;
	virtual void	Use( int gameID, const char *name ) = 0;
	virtual void	Activate( int gameID, const char *name ) = 0;
	virtual void	Deactivate( int gameID, const char *name ) = 0;
	virtual void	Kill( int gameID, const char *name ) = 0;
	virtual void	Remove( int gameID, const char *name )  = 0;
	virtual float	Random( float min, float max ) = 0;
	virtual void	Play( int taskID, int gameID, const char *type, const char *name ) = 0;

	// Camera functions
	virtual void	CameraPan( float angles[3], float dir[3], float duration ) = 0;
	virtual void	CameraMove( float origin[3], float duration ) = 0;
	virtual void	CameraZoom( float fov, float duration ) = 0;
	virtual void	CameraRoll( float angle, float duration ) = 0;
	virtual void	CameraFollow( const char *name, float speed, float initLerp ) = 0;
	virtual void	CameraTrack( const char *name, float speed, float initLerp ) = 0;
	virtual void	CameraDistance( float dist, float initLerp ) = 0;
	virtual void	CameraFade( float sr, float sg, float sb, float sa, float dr, float dg, float db, float da, float duration ) = 0;
	virtual void	CameraPath( const char *name ) = 0;
	virtual void	CameraEnable( void ) = 0;
	virtual void	CameraDisable( void ) = 0;
	virtual void	CameraShake( float intensity, int duration ) = 0;

	virtual int		GetFloat( int gameID, const char *name, float *value ) = 0;
	// Should be float return type?
	virtual int		GetVector( int gameID, const char *name, float value[3] ) = 0;
	virtual int		GetString( int gameID, const char *name, char **value ) = 0;

	virtual int		Evaluate( int p1Type, const char *p1, int p2Type, const char *p2, int operatorType ) = 0;

	virtual void	DeclareVariable( int type, const char *name ) = 0;
	virtual void	FreeVariable( const char *name ) = 0;

	// Save / Load functions

	virtual int		WriteSaveData( unsigned int chid, void *data, int length ) = 0;
	virtual int		ReadSaveData( unsigned int chid, void *address, int length, void **addressptr = NULL )  = 0;
	virtual int		LinkGame( int gameID, int icarusID ) = 0;
	
	// Access functions

	virtual int		CreateIcarus( int gameID) = 0;
	virtual int		GetByName( const char *name ) = 0;		//Polls the engine for the sequencer of the entity matching the name passed
	virtual int		IsFrozen(int gameID) = 0;					// (g_entities[m_ownerID].svFlags&SVF_ICARUS_FREEZE)																	// return -1 indicates invalid
	virtual void	Free(void* data) = 0;
	virtual void*	Malloc( int size ) = 0;
	virtual float	MaxFloat(void) = 0;

	// Script precache functions.
	virtual void	PrecacheRoff(const char* name) = 0;		// G_LoadRoff
	virtual void	PrecacheScript(const char* name) = 0;	// must strip extension COM_StripExtension()
	virtual void	PrecacheSound(const char* name) = 0;	// G_SoundIndex
	virtual void	PrecacheFromSet(const char* setname, const char* filename) = 0;
};

#endif
