
#include "../client/client.h"
#include "mac_local.h"
#include "InputSprocket.h"

qboolean			inputActive;
qboolean			inputSuspended;

#define	MAX_DEVICES		100
ISpDeviceReference	devices[MAX_DEVICES];
ISpElementListReference	elementList;

#define	MAX_ELEMENTS	512
#define	MAX_MOUSE_DEVICES	2
UInt32				numDevices;
UInt32				numElements[MAX_MOUSE_DEVICES];
ISpElementReference	elements[MAX_MOUSE_DEVICES][MAX_ELEMENTS];

cvar_t				*in_nomouse;

void Input_Init(void);
void Input_GetState( void );

/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput( void ) {
	NumVersion		ver;
	ISpElementInfo	info;
	int				i, j;
	OSStatus		err;
	
	// no input with dedicated servers
	if ( com_dedicated->integer ) {
		return;
	}
	
	Com_Printf( "------- Input Initialization -------\n" );
	in_nomouse = Cvar_Get( "in_nomouse", "0", 0 );
	if ( in_nomouse->integer != 0 ) {
		Com_Printf( "in_nomouse is set, skipping.\n" );
		Com_Printf( "------------------------------------\n" );
		return;
	}
	
	ver = ISpGetVersion();
	Com_Printf( "InputSprocket version: 0x%x\n", ver );
		
	err = ISpStartup();
	if ( err ) {
		Com_Printf( "ISpStartup failed: %i\n", err );
		Com_Printf( "------------------------------------\n" );
		return;
	}

	// disable everything
	ISpDevices_Extract( MAX_DEVICES, &numDevices, devices );
	Com_Printf("%i total devices\n", numDevices);
	if (numDevices > MAX_DEVICES) {
		numDevices = MAX_DEVICES;
	}
	err = ISpDevices_Deactivate(
			numDevices,
			devices);
	
	// enable mouse
	err = ISpDevices_ExtractByClass(
			kISpDeviceClass_Mouse,
			MAX_DEVICES,
			&numDevices,
			devices);
	Com_Printf("%i mouse devices\n", numDevices);
	if (numDevices > MAX_MOUSE_DEVICES) {
		numDevices = MAX_MOUSE_DEVICES;
	}
	
	err = ISpDevices_Activate( numDevices, devices);
	for ( i = 0 ; i < numDevices ; i++ ) {
		ISpDevice_GetElementList( devices[i], &elementList );
	
//	ISpGetGlobalElementList( &elementList );
	
		// go through all the elements and asign them Quake key codes
		ISpElementList_Extract( elementList, MAX_ELEMENTS, &numElements[i], elements[i] );
		Com_Printf("%i elements in list\n", numElements[i] );
		
		for ( j = 0 ; j	< numElements[i] ; j++ ) {
			ISpElement_GetInfo( elements[i][j], &info );
			PStringToCString( (char *)info.theString );
			Com_Printf( "%i : %s\n", i, info.theString );
		}
	}
	
	inputActive = true;

	HideCursor();

	Com_Printf( "------------------------------------\n" );
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput( void ) {
	if ( !inputActive ) {
		return;
	}
	ShowCursor();
	ISpShutdown();
	inputActive = qfalse;
}

void Sys_SuspendInput( void ) {
	if ( inputSuspended ) {
		return;
	}
	inputSuspended = true;
	ShowCursor();
	ISpSuspend();
}

void Sys_ResumeInput( void ) {
	if ( !inputSuspended ) {
		return;
	}
	inputSuspended = false;
	HideCursor();
	ISpResume();
}

/*
=================
Sys_Input
=================
*/
void Sys_Input( void ) {
	ISpElementEvent		event;
	Boolean				wasEvent;
	UInt32				state, state2;
	int					xmove, ymove;
	int					button;
	static int xtotal, ytotal;
	int					device;
	
	if ( !inputActive ) {
		return;
	}

	// during debugging it is sometimes usefull to be able to kill mouse support
	if ( in_nomouse->integer ) {
		Com_Printf( "Shutting down input.\n");
		Sys_ShutdownInput();
		return;
	}
	
	// always suspend for dedicated 
	if ( com_dedicated->integer ) {
		Sys_SuspendInput();
		return;
	}
	
	// temporarily deactivate if not in the game and
	if ( cls.keyCatchers || cls.state != CA_ACTIVE ) {
		if ( !glConfig.isFullscreen ) {
			Sys_SuspendInput();
			return;
		}
	}

	Sys_ResumeInput();

	// send all button events
	for ( device = 0 ; device < numDevices ; device++ ) {
		// mouse buttons
		
		for ( button = 2 ; button < numElements[device] ; button++ ) {
			while ( 1 ) {
				ISpElement_GetNextEvent( elements[device][button], sizeof( event ), &event, &wasEvent );
				if ( !wasEvent ) {
					break;
				}
				if ( event.data ) {
					Sys_QueEvent( 0, SE_KEY, K_MOUSE1 + button - 2, 1, 0, NULL );
				} else {
					Sys_QueEvent( 0, SE_KEY, K_MOUSE1 + button - 2, 0, 0, NULL );
				}
			}
		}
		
		// mouse movement
		
#define	MAC_MOUSE_SCALE		163		// why this constant?
		// send mouse event
		ISpElement_GetSimpleState( elements[device][0], &state );
		xmove = (int)state / MAC_MOUSE_SCALE;
		
		ISpElement_GetSimpleState( elements[device][1], &state2 );
		ymove = (int)state2 / -MAC_MOUSE_SCALE;
		
		if ( xmove || ymove ) {
			xtotal += xmove;
			ytotal += ymove;
	//Com_Printf("%i %i = %i %i\n", state, state2, xtotal, ytotal );
			Sys_QueEvent( 0, SE_MOUSE, xmove, ymove, 0, NULL );
		}
	}
	
}
