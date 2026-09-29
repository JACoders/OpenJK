/*
 * External split-screen input simulator for macOS.
 *
 * This intentionally lives outside the game. It posts keyboard and mouse events
 * through Quartz so OpenJK receives them through the same OS event path as
 * normal keyboard and mouse input.
 *
 * Controller/gamepad simulation uses IOHIDUserDevice, which is a real virtual
 * HID device outside OpenJK. macOS requires the virtual HID entitlement for
 * that backend to activate successfully.
 */

#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hidsystem/IOHIDUserDevice.h>
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GAMEPAD_MAGIC 0x4f4a4750u
#define GAMEPAD_VERSION 1
#define GAMEPAD_AXES 6
#define GAMEPAD_TAP_USEC 120000

typedef struct gamepad_packet_s {
	uint32_t magic;
	uint8_t version;
	uint8_t controller;
	uint16_t buttons;
	int16_t axes[GAMEPAD_AXES];
} gamepad_packet_t;

static gamepad_packet_t gamepads[4];
static gamepad_packet_t system_input;

typedef struct key_map_s {
	const char *name;
	CGKeyCode code;
} key_map_t;

static const key_map_t key_map[] = {
	{ "a", 0 }, { "s", 1 }, { "d", 2 }, { "f", 3 }, { "h", 4 }, { "g", 5 },
	{ "z", 6 }, { "x", 7 }, { "c", 8 }, { "v", 9 }, { "b", 11 }, { "q", 12 },
	{ "w", 13 }, { "e", 14 }, { "r", 15 }, { "y", 16 }, { "t", 17 },
	{ "1", 18 }, { "2", 19 }, { "3", 20 }, { "4", 21 }, { "6", 22 },
	{ "5", 23 }, { "=", 24 }, { "9", 25 }, { "7", 26 }, { "-", 27 },
	{ "8", 28 }, { "0", 29 }, { "]", 30 }, { "o", 31 }, { "u", 32 },
	{ "[", 33 }, { "i", 34 }, { "p", 35 }, { "l", 37 }, { "j", 38 },
	{ "'", 39 }, { "k", 40 }, { ";", 41 }, { "\\", 42 }, { ",", 43 },
	{ "/", 44 }, { "n", 45 }, { "m", 46 }, { ".", 47 }, { "`", 50 },
	{ "space", 49 }, { "tab", 48 }, { "enter", 36 }, { "return", 36 },
	{ "escape", 53 }, { "esc", 53 }, { "backspace", 51 }, { "delete", 51 },
	{ "left", 123 }, { "right", 124 }, { "down", 125 }, { "up", 126 },
	{ "shift", 56 }, { "ctrl", 59 }, { "control", 59 }, { "alt", 58 },
	{ "option", 58 }, { "cmd", 55 }, { "command", 55 },
};

static void usage( const char *argv0 )
{
	fprintf( stderr,
		"usage: %s <commands...>\n"
		"\n"
		"commands:\n"
		"  check-permission\n"
		"  request-permission\n"
		"  wait <ms>\n"
		"  key <name> <down|up|tap>\n"
		"  mouse <dx> <dy>\n"
		"  click <left|right>\n"
		"  gamepad <1-4> axis <0-5> <-32768..32767>\n"
		"  gamepad <1-4> button <0-15> <down|up|tap>\n"
		"  bridge-mouse <left> <down|up|tap>\n"
		"  bridge-mouse-move <dx> <dy>\n"
		"  gamepad-demo <ms>   # drive three SDL gamepads through the localhost bridge\n"
		"  hid-gamepad-demo <ms> # restricted IOHIDUserDevice backend\n"
		"\n"
		"example:\n"
		"  %s wait 1000 key w down wait 250 key w up mouse 30 0 click left\n",
		argv0, argv0 );
}

static int streq( const char *a, const char *b )
{
	return strcmp( a, b ) == 0;
}

static int lookup_key( const char *name, CGKeyCode *out )
{
	size_t i;
	char lower[64];
	size_t len = strlen( name );

	if ( len >= sizeof( lower ) ) {
		return 0;
	}
	for ( i = 0; i <= len; i++ ) {
		lower[i] = (char)tolower( (unsigned char)name[i] );
	}

	for ( i = 0; i < sizeof( key_map ) / sizeof( key_map[0] ); i++ ) {
		if ( streq( lower, key_map[i].name ) ) {
			*out = key_map[i].code;
			return 1;
		}
	}
	return 0;
}

static int trusted( int prompt )
{
	const void *keys[] = { kAXTrustedCheckOptionPrompt };
	const void *values[] = { prompt ? kCFBooleanTrue : kCFBooleanFalse };
	CFDictionaryRef options = CFDictionaryCreate( NULL, keys, values, 1, NULL, NULL );
	Boolean ok = AXIsProcessTrustedWithOptions( options );
	CFRelease( options );
	return ok ? 1 : 0;
}

static void cfset_int( CFMutableDictionaryRef dict, CFStringRef key, int value )
{
	CFNumberRef number = CFNumberCreate( NULL, kCFNumberIntType, &value );
	CFDictionarySetValue( dict, key, number );
	CFRelease( number );
}

static void post_key( CGKeyCode key, int down )
{
	CGEventRef event = CGEventCreateKeyboardEvent( NULL, key, down ? true : false );
	CGEventPost( kCGHIDEventTap, event );
	CFRelease( event );
}

static void post_mouse_move( int dx, int dy )
{
	CGEventRef current = CGEventCreate( NULL );
	CGPoint point = CGEventGetLocation( current );
	CFRelease( current );

	point.x += dx;
	point.y += dy;

	CGEventRef event = CGEventCreateMouseEvent( NULL, kCGEventMouseMoved, point, 0 );
	CGEventPost( kCGHIDEventTap, event );
	CFRelease( event );
}

static void post_click( const char *button )
{
	CGMouseButton cg_button = kCGMouseButtonLeft;
	CGEventType down_type = kCGEventLeftMouseDown;
	CGEventType up_type = kCGEventLeftMouseUp;
	CGEventRef current;
	CGPoint point;
	CGEventRef event;

	if ( streq( button, "right" ) ) {
		cg_button = kCGMouseButtonRight;
		down_type = kCGEventRightMouseDown;
		up_type = kCGEventRightMouseUp;
	} else if ( !streq( button, "left" ) ) {
		fprintf( stderr, "unknown mouse button: %s\n", button );
		exit( 2 );
	}

	current = CGEventCreate( NULL );
	point = CGEventGetLocation( current );
	CFRelease( current );

	event = CGEventCreateMouseEvent( NULL, down_type, point, cg_button );
	CGEventPost( kCGHIDEventTap, event );
	CFRelease( event );

	usleep( 20000 );

	event = CGEventCreateMouseEvent( NULL, up_type, point, cg_button );
	CGEventPost( kCGHIDEventTap, event );
	CFRelease( event );
}

static IOHIDUserDeviceRef create_virtual_gamepad( void )
{
	static const uint8_t descriptor[] = {
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x05,       /* Usage (Game Pad) */
		0xA1, 0x01,       /* Collection (Application) */
		0x85, 0x01,       /*   Report ID (1) */
		0x05, 0x09,       /*   Usage Page (Button) */
		0x19, 0x01,       /*   Usage Minimum (1) */
		0x29, 0x10,       /*   Usage Maximum (16) */
		0x15, 0x00,       /*   Logical Minimum (0) */
		0x25, 0x01,       /*   Logical Maximum (1) */
		0x75, 0x01,       /*   Report Size (1) */
		0x95, 0x10,       /*   Report Count (16) */
		0x81, 0x02,       /*   Input (Data, Variable, Absolute) */
		0x05, 0x01,       /*   Usage Page (Generic Desktop) */
		0x09, 0x30,       /*   Usage (X) */
		0x09, 0x31,       /*   Usage (Y) */
		0x09, 0x32,       /*   Usage (Z) */
		0x09, 0x35,       /*   Usage (Rz) */
		0x15, 0x81,       /*   Logical Minimum (-127) */
		0x25, 0x7F,       /*   Logical Maximum (127) */
		0x75, 0x08,       /*   Report Size (8) */
		0x95, 0x04,       /*   Report Count (4) */
		0x81, 0x02,       /*   Input (Data, Variable, Absolute) */
		0xC0              /* End Collection */
	};
	CFMutableDictionaryRef properties;
	CFDataRef report_descriptor;
	IOHIDUserDeviceRef device;
	dispatch_queue_t queue;

	properties = CFDictionaryCreateMutable( NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
	report_descriptor = CFDataCreate( NULL, descriptor, sizeof( descriptor ) );
	CFDictionarySetValue( properties, CFSTR( kIOHIDReportDescriptorKey ), report_descriptor );
	CFDictionarySetValue( properties, CFSTR( kIOHIDProductKey ), CFSTR( "OpenJK QA Virtual Gamepad" ) );
	cfset_int( properties, CFSTR( kIOHIDVendorIDKey ), 0x1209 );
	cfset_int( properties, CFSTR( kIOHIDProductIDKey ), 0x4A4B );
	cfset_int( properties, CFSTR( kIOHIDPrimaryUsagePageKey ), 0x01 );
	cfset_int( properties, CFSTR( kIOHIDPrimaryUsageKey ), 0x05 );
	cfset_int( properties, CFSTR( kIOHIDMaxInputReportSizeKey ), 7 );

	device = IOHIDUserDeviceCreateWithProperties( NULL, properties, 0 );
	CFRelease( report_descriptor );
	CFRelease( properties );
	if ( !device ) {
		return NULL;
	}

	queue = dispatch_queue_create( "openjk.qa.virtual-gamepad", DISPATCH_QUEUE_SERIAL );
	IOHIDUserDeviceSetDispatchQueue( device, queue );
	IOHIDUserDeviceActivate( device );
	dispatch_release( queue );
	return device;
}

static void send_gamepad_report( IOHIDUserDeviceRef device, uint16_t buttons, int8_t x, int8_t y, int8_t z, int8_t rz )
{
	uint8_t report[7];
	report[0] = 1;
	report[1] = (uint8_t)( buttons & 0xff );
	report[2] = (uint8_t)( ( buttons >> 8 ) & 0xff );
	report[3] = (uint8_t)x;
	report[4] = (uint8_t)y;
	report[5] = (uint8_t)z;
	report[6] = (uint8_t)rz;
	IOHIDUserDeviceHandleReportWithTimeStamp( device, mach_absolute_time(), report, sizeof( report ) );
}

static int hid_gamepad_demo( int duration_ms )
{
	IOHIDUserDeviceRef device = create_virtual_gamepad();

	if ( !device ) {
		fprintf( stderr,
			"Could not create IOHIDUserDevice virtual gamepad. macOS requires the "
			"com.apple.developer.hid.virtual.device entitlement for this backend.\n" );
		return 4;
	}

	printf( "Created virtual HID gamepad. Keep this process running before launching OpenJK for SDL enumeration.\n" );
	send_gamepad_report( device, 0, 0, 0, 0, 0 );
	usleep( 500000 );
	send_gamepad_report( device, 0, 0, 127, 0, 0 );
	usleep( 250000 );
	send_gamepad_report( device, 0, 0, 0, 0, 0 );
	send_gamepad_report( device, 1, 0, 0, 0, 0 );
	usleep( 100000 );
	send_gamepad_report( device, 0, 0, 0, 0, 0 );

	if ( duration_ms > 0 ) {
		usleep( (useconds_t)duration_ms * 1000 );
	}

	IOHIDUserDeviceCancel( device );
	CFRelease( device );
	return 0;
}

static int bridge_port( void )
{
	const char *text = getenv( "OPENJK_VIRTUAL_GAMEPAD_PORT" );
	int port = text && text[0] ? atoi( text ) : 29180;
	return port >= 1024 && port <= 65535 ? port : 29180;
}

static int send_bridge_gamepad( int controller )
{
	struct sockaddr_in address;
	gamepad_packet_t packet = gamepads[controller];
	int sock;
	int attempt;
	int i;
	ssize_t sent;

	packet.magic = htonl( GAMEPAD_MAGIC );
	packet.version = GAMEPAD_VERSION;
	packet.controller = (uint8_t)controller;
	packet.buttons = htons( packet.buttons );
	for ( i = 0; i < GAMEPAD_AXES; ++i ) {
		packet.axes[i] = (int16_t)htons( (uint16_t)packet.axes[i] );
	}

	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sock < 0 ) {
		perror( "gamepad bridge socket" );
		return 5;
	}
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	address.sin_port = htons( (uint16_t)bridge_port() );
	for ( attempt = 0; attempt < 3; ++attempt ) {
		sent = sendto( sock, &packet, sizeof( packet ), 0, (struct sockaddr *)&address, sizeof( address ) );
		if ( sent != sizeof( packet ) ) {
			close( sock );
			perror( "gamepad bridge send" );
			return 5;
		}
		usleep( 1000 );
	}
	close( sock );
	return 0;
}

static int send_bridge_system_input( void )
{
	struct sockaddr_in address;
	gamepad_packet_t packet = system_input;
	int sock;
	int attempt;
	int i;
	ssize_t sent;

	packet.magic = htonl( GAMEPAD_MAGIC );
	packet.version = GAMEPAD_VERSION;
	packet.controller = 255;
	packet.buttons = htons( packet.buttons );
	for ( i = 0; i < GAMEPAD_AXES; ++i ) {
		packet.axes[i] = (int16_t)htons( (uint16_t)packet.axes[i] );
	}
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sock < 0 ) {
		perror( "system input bridge socket" );
		return 5;
	}
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	address.sin_port = htons( (uint16_t)bridge_port() );
	for ( attempt = 0; attempt < 3; ++attempt ) {
		sent = sendto( sock, &packet, sizeof( packet ), 0, (struct sockaddr *)&address, sizeof( address ) );
		if ( sent != sizeof( packet ) ) {
			close( sock );
			return 5;
		}
		usleep( 1000 );
	}
	close( sock );
	return 0;
}

static int bridge_gamepad_demo( int duration_ms )
{
	int controller;

	for ( controller = 0; controller < 3; ++controller ) {
		gamepads[controller].axes[controller % 2] = (int16_t)( 12000 + controller * 4000 );
		gamepads[controller].buttons = (uint16_t)( 1u << controller );
		send_bridge_gamepad( controller );
	}
	usleep( (useconds_t)duration_ms * 1000 );
	for ( controller = 0; controller < 3; ++controller ) {
		memset( &gamepads[controller], 0, sizeof( gamepads[controller] ) );
		send_bridge_gamepad( controller );
	}
	printf( "Drove three external SDL gamepads through 127.0.0.1:%d.\n", bridge_port() );
	return 0;
}

int main( int argc, char **argv )
{
	int i = 1;

	if ( argc < 2 ) {
		usage( argv[0] );
		return 2;
	}

	while ( i < argc ) {
		const char *cmd = argv[i++];

		if ( streq( cmd, "check-permission" ) ) {
			if ( !trusted( 0 ) ) {
				fprintf( stderr, "Accessibility permission is not granted for this process.\n" );
				return 3;
			}
			printf( "Accessibility permission granted.\n" );
			continue;
		}

		if ( streq( cmd, "request-permission" ) ) {
			if ( !trusted( 1 ) ) {
				fprintf( stderr, "Accessibility permission was requested but is not granted yet.\n" );
				return 3;
			}
			printf( "Accessibility permission granted.\n" );
			continue;
		}

		if ( streq( cmd, "wait" ) ) {
			if ( i >= argc ) {
				usage( argv[0] );
				return 2;
			}
			usleep( (useconds_t)( atoi( argv[i++] ) * 1000 ) );
			continue;
		}

		if ( streq( cmd, "key" ) ) {
			CGKeyCode key;
			const char *name;
			const char *action;

			if ( i + 1 >= argc ) {
				usage( argv[0] );
				return 2;
			}
			name = argv[i++];
			action = argv[i++];
			if ( !lookup_key( name, &key ) ) {
				fprintf( stderr, "unknown key: %s\n", name );
				return 2;
			}
			if ( streq( action, "down" ) ) {
				post_key( key, 1 );
			} else if ( streq( action, "up" ) ) {
				post_key( key, 0 );
			} else if ( streq( action, "tap" ) ) {
				post_key( key, 1 );
				usleep( 20000 );
				post_key( key, 0 );
			} else {
				fprintf( stderr, "unknown key action: %s\n", action );
				return 2;
			}
			continue;
		}

		if ( streq( cmd, "mouse" ) ) {
			int dx;
			int dy;
			if ( i + 1 >= argc ) {
				usage( argv[0] );
				return 2;
			}
			dx = atoi( argv[i++] );
			dy = atoi( argv[i++] );
			post_mouse_move( dx, dy );
			continue;
		}

		if ( streq( cmd, "click" ) ) {
			if ( i >= argc ) {
				usage( argv[0] );
				return 2;
			}
			post_click( argv[i++] );
			continue;
		}

		if ( streq( cmd, "gamepad-demo" ) ) {
			int duration_ms;
			if ( i >= argc ) {
				usage( argv[0] );
				return 2;
			}
			duration_ms = atoi( argv[i++] );
			return bridge_gamepad_demo( duration_ms );
		}

		if ( streq( cmd, "bridge-mouse" ) ) {
			const char *button;
			const char *action;
			if ( i + 1 >= argc ) {
				usage( argv[0] );
				return 2;
			}
			button = argv[i++];
			action = argv[i++];
			if ( !streq( button, "left" ) ) {
				fprintf( stderr, "bridge mouse button must be left\n" );
				return 2;
			}
			if ( streq( action, "down" ) || streq( action, "tap" ) ) {
				system_input.buttons |= 1u;
			} else if ( streq( action, "up" ) ) {
				system_input.buttons &= (uint16_t)~1u;
			} else {
				fprintf( stderr, "bridge mouse action must be down, up, or tap\n" );
				return 2;
			}
			if ( send_bridge_system_input() != 0 ) return 5;
			if ( streq( action, "tap" ) ) {
				usleep( GAMEPAD_TAP_USEC );
				system_input.buttons &= (uint16_t)~1u;
				if ( send_bridge_system_input() != 0 ) return 5;
			}
			continue;
		}

		if ( streq( cmd, "bridge-mouse-move" ) ) {
			if ( i + 1 >= argc ) {
				usage( argv[0] );
				return 2;
			}
			system_input.axes[0] = (int16_t)atoi( argv[i++] );
			system_input.axes[1] = (int16_t)atoi( argv[i++] );
			if ( send_bridge_system_input() != 0 ) return 5;
			system_input.axes[0] = 0;
			system_input.axes[1] = 0;
			continue;
		}

		if ( streq( cmd, "hid-gamepad-demo" ) ) {
			int duration_ms;
			if ( i >= argc ) {
				usage( argv[0] );
				return 2;
			}
			duration_ms = atoi( argv[i++] );
			return hid_gamepad_demo( duration_ms );
		}

		if ( streq( cmd, "gamepad" ) ) {
			int controller;
			int control;
			const char *kind;
			const char *value;
			if ( i + 3 >= argc ) {
				usage( argv[0] );
				return 2;
			}
			controller = atoi( argv[i++] ) - 1;
			kind = argv[i++];
			control = atoi( argv[i++] );
			value = argv[i++];
			if ( controller < 0 || controller >= 4 ) {
				fprintf( stderr, "gamepad number must be 1 through 4\n" );
				return 2;
			}
			if ( streq( kind, "axis" ) ) {
				int axisValue = atoi( value );
				if ( control < 0 || control >= GAMEPAD_AXES || axisValue < -32768 || axisValue > 32767 ) {
					fprintf( stderr, "axis must be 0 through 5 and value -32768 through 32767\n" );
					return 2;
				}
				gamepads[controller].axes[control] = (int16_t)axisValue;
				if ( send_bridge_gamepad( controller ) != 0 ) return 5;
			} else if ( streq( kind, "button" ) ) {
				if ( control < 0 || control >= 16 ) {
					fprintf( stderr, "button must be 0 through 15\n" );
					return 2;
				}
				if ( streq( value, "down" ) || streq( value, "tap" ) ) {
					gamepads[controller].buttons |= (uint16_t)( 1u << control );
				} else if ( streq( value, "up" ) ) {
					gamepads[controller].buttons &= (uint16_t)~( 1u << control );
				} else {
					fprintf( stderr, "button action must be down, up, or tap\n" );
					return 2;
				}
				if ( send_bridge_gamepad( controller ) != 0 ) return 5;
				if ( streq( value, "tap" ) ) {
					usleep( GAMEPAD_TAP_USEC );
					gamepads[controller].buttons &= (uint16_t)~( 1u << control );
					if ( send_bridge_gamepad( controller ) != 0 ) return 5;
				}
			} else {
				fprintf( stderr, "gamepad control must be axis or button\n" );
				return 2;
			}
			continue;
		}

		fprintf( stderr, "unknown command: %s\n", cmd );
		usage( argv[0] );
		return 2;
	}

	return 0;
}
