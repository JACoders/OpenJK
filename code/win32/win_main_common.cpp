// win_main.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "win_local.h"
#include "resource.h"

#ifndef _GAMECUBE
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#endif



//#define SPANK_MONKEYS	//----(SA)	commented out for running net developer release builds
int	sys_monkeySpank;


/*
==================
Sys_MonkeyShouldBeSpanked
==================
*/
int Sys_MonkeyShouldBeSpanked( void ) {
	return sys_monkeySpank;
}




/*
==================
Sys_FunctionCmp
==================
*/
int Sys_FunctionCmp(void *f1, void *f2) {

	int i, j, l;
	byte func_end[32] = {0xC3, 0x90, 0x90, 0x00};
	byte *ptr, *ptr2;
	byte *f1_ptr, *f2_ptr;

	ptr = (byte *) f1;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f1 %p1 jmp %d\n", (int *) f1, *(int*)(ptr+1));
		f1_ptr = (byte*)(((byte*)f1) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f1_ptr = ptr;
	}
	//Com_Printf("f1 ptr %p\n", f1_ptr);

	ptr = (byte *) f2;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f2 %p jmp %d\n", (int *) f2, *(int*)(ptr+1));
		f2_ptr = (byte*)(((byte*)f2) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f2_ptr = ptr;
	}
	//Com_Printf("f2 ptr %p\n", f2_ptr);

#ifdef _DEBUG
	sprintf((char *)func_end, "%c%c%c%c%c%c%c", 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3);
#endif
	for (i = 0; i < 1024; i++) {
		for (j = 0; func_end[j]; j++) {
			if (f1_ptr[i+j] != func_end[j])
				break;
		}
		if (!func_end[j]) {
			break;
		}
	}
#ifdef _DEBUG
	l = i + 7;
#else
	l = i + 2;
#endif
	//Com_Printf("function length = %d\n", l);

	for (i = 0; i < l; i++) {
		// check for a potential function call
		if (*((byte *) &f1_ptr[i]) == 0xE8) {
			// get the function pointers in case this really is a function call
			ptr = (byte *) (((byte *) &f1_ptr[i]) + (*(int *) &f1_ptr[i+1])) + 5;
			ptr2 = (byte *) (((byte *) &f2_ptr[i]) + (*(int *) &f2_ptr[i+1])) + 5;
			// if it was a function call and both f1 and f2 call the same function
			if (ptr == ptr2) {
				i += 4;
				continue;
			}
		}
		if (f1_ptr[i] != f2_ptr[i])
			return qfalse;
	}
	return qtrue;
}

/*
==================
Sys_FunctionCheckSum
==================
*/
int Sys_FunctionCheckSum(void *f1) {

	int i, j, l;
	unsigned shermcrap;
	byte func_end[32] = {0xC3, 0x90, 0x90, 0x00};
	byte *ptr;
	byte *f1_ptr;

	ptr = (byte *) f1;
	if (*(byte *)ptr == 0xE9) {
		//Com_Printf("f1 %p1 jmp %d\n", (int *) f1, *(int*)(ptr+1));
		f1_ptr = (byte*)(((byte*)f1) + (*(int *)(ptr+1)) + 5);
	}
	else {
		f1_ptr = ptr;
	}
	//Com_Printf("f1 ptr %p\n", f1_ptr);

#ifdef _DEBUG
	sprintf((char *)func_end, "%c%c%c%c%c%c%c", 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3);
#endif
	for (i = 0; i < 1024; i++) {
		for (j = 0; func_end[j]; j++) {
			if (f1_ptr[i+j] != func_end[j])
				break;
		}
		if (!func_end[j]) {
			break;
		}
	}
#ifdef _DEBUG
	l = i + 7;
#else
	l = i + 2;
#endif
	//Com_Printf("function length = %d\n", l);
	shermcrap = Com_BlockChecksum( f1_ptr, l );
	return (int)shermcrap;
}


//NOTE TTimo: heavily NON PORTABLE, PLZ DON'T USE
//  https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=447
#if 0
//----(SA) added
/*
==============
Sys_ShellExecute

-	Windows only

	Performs an operation on a specified file. 

	See info on ShellExecute() for details
  
==============
*/
int Sys_ShellExecute(char *op, char *file, qboolean doexit, char *params, char *dir ) {
	unsigned int retval;
	char *se_op;
	
	// set default operation to "open"
	if(op)	se_op = op;
	else	se_op = "open";


	// probably need to protect this some in the future so people have
	// less chance of system invasion with this powerful interface
	// (okay, not so invasive, but could be annoying/rude)


	retval = (UINT)ShellExecute(NULL, se_op, file, params, dir, SW_NORMAL);	// only option forced by game is 'sw_normal'
	
	if( retval <= 32) {	// ERROR
		Com_DPrintf("Sys_ShellExecuteERROR: %d\n", retval);
		return retval;
	}

	if ( doexit ) {
		// (SA) this works better for exiting cleanly...
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
	}

	return 999;	// success
}
//----(SA) end
#endif

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}





/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( void ) {
	return "";
}

/*
==============
Sys_DefaultBasePath
==============
*/
char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}




/*
========================================================================

EVENT LOOP

========================================================================
*/


sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}


//================================================================


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f( void ) {
//	NET_Restart();
}



//=======================================================================


void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}


void *Sys_InitializeCriticalSection() {
	return (void*)-1;
}

void Sys_EnterCriticalSection(void *ptr) {
}

void Sys_LeaveCriticalSection(void *ptr) {
}

