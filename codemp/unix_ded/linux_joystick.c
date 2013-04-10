/*
** linux_joystick.c
**
** This file contains ALL Linux specific stuff having to do with the
** Joystick input.  When a port is being made the following functions
** must be implemented by the port:
**
** Authors: mkv, bk
**
*/

#include <linux/joystick.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>  // bk001204


#include "../client/client.h"
#include "linux_local.h"

/* We translate axes movement into keypresses. */
int joy_keys[16] = {
     K_LEFTARROW, K_RIGHTARROW,
     K_UPARROW, K_DOWNARROW,
     K_JOY16, K_JOY17,
     K_JOY18, K_JOY19,
     K_JOY20, K_JOY21,
     K_JOY22, K_JOY23,

     K_JOY24, K_JOY25,
     K_JOY26, K_JOY27
};

/* Our file descriptor for the joystick device. */
static int             joy_fd = -1;


// bk001130 - from linux_glimp.c
extern cvar_t *  in_joystick;
extern cvar_t *  in_joystickDebug;
extern cvar_t *  joy_threshold;


/**********************************************/
/* Joystick routines.                         */
/**********************************************/
// bk001130 - from cvs1.17 (mkv), removed from linux_glimp.c
void IN_StartupJoystick( void )
{
  int i = 0;

  joy_fd = -1;

  if( !in_joystick->integer ) {
    Com_Printf( "Joystick is not active.\n" );
    return;
  }

  for( i = 0; i < 4; i++ ) {
    char filename[PATH_MAX];

    snprintf( filename, PATH_MAX, "/dev/js%d", i );

    joy_fd = open( filename, O_RDONLY | O_NONBLOCK );

    if( joy_fd != -1 ) {
      struct js_event event;
      char axes = 0;
      char buttons = 0;
      char name[128];
      int n = -1;

      Com_Printf( "Joystick %s found\n", filename );

      /* Get rid of initialization messages. */
      do {
	n = read( joy_fd, &event, sizeof( event ) );

	if( n == -1 ) {
	  break;
	}

      } while( ( event.type & JS_EVENT_INIT ) );

      /* Get joystick statistics. */
      ioctl( joy_fd, JSIOCGAXES, &axes );
      ioctl( joy_fd, JSIOCGBUTTONS, &buttons );

      if( ioctl( joy_fd, JSIOCGNAME( sizeof( name ) ), name ) < 0 ) {
	strncpy( name, "Unknown", sizeof( name ) );
      }

      Com_Printf( "Name:    %s\n", name );
      Com_Printf( "Axes:    %d\n", axes );
      Com_Printf( "Buttons: %d\n", buttons );

      /* Our work here is done. */
      return;
    }

  }

  /* No soup for you. */
  if( joy_fd == -1 ) {
    Com_Printf( "No joystick found.\n" );
    return;
  }

}

void IN_JoyMove( void )
{
  /* Store instantaneous joystick state. Hack to get around
   * event model used in Linux joystick driver.
	 */
  static int axes_state[16];
  /* Old bits for Quake-style input compares. */
  static unsigned int old_axes = 0;
  /* Our current goodies. */
  unsigned int axes = 0;
  int i = 0;

  if( joy_fd == -1 ) {
    return;
  }

  /* Empty the queue, dispatching button presses immediately
	 * and updating the instantaneous state for the axes.
	 */
  do {
    int n = -1;
    struct js_event event;

    n = read( joy_fd, &event, sizeof( event ) );

    if( n == -1 ) {
      /* No error, we're non-blocking. */
      break;
    }

    if( event.type & JS_EVENT_BUTTON ) {
      Sys_QueEvent( 0, SE_KEY, K_JOY1 + event.number, event.value, 0, NULL );
    } else if( event.type & JS_EVENT_AXIS ) {

      if( event.number >= 16 ) {
	continue;
      }

      axes_state[event.number] = event.value;
    } else {
      Com_Printf( "Unknown joystick event type\n" );
    }

  } while( 1 );


  /* Translate our instantaneous state to bits. */
  for( i = 0; i < 16; i++ ) {
    float f = ( (float) axes_state[i] ) / 32767.0f;

    if( f < -joy_threshold->value ) {
      axes |= ( 1 << ( i * 2 ) );
    } else if( f > joy_threshold->value ) {
      axes |= ( 1 << ( ( i * 2 ) + 1 ) );
    }

  }

  /* Time to update axes state based on old vs. new. */
  for( i = 0; i < 16; i++ ) {

    if( ( axes & ( 1 << i ) ) && !( old_axes & ( 1 << i ) ) ) {
      Sys_QueEvent( 0, SE_KEY, joy_keys[i], qtrue, 0, NULL );
    }

    if( !( axes & ( 1 << i ) ) && ( old_axes & ( 1 << i ) ) ) {
      Sys_QueEvent( 0, SE_KEY, joy_keys[i], qfalse, 0, NULL );
    }
  }

  /* Save for future generations. */
  old_axes = axes;
}


