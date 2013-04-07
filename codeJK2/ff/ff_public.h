#ifndef __FF_PUBLIC_H
#define __FF_PUBLIC_H

#define FF_HANDLE_NULL		0
#define FF_CLIENT_LOCAL		(-2)
#define FF_CLIENT( client ) (FF_CLIENT_LOCAL - client)

typedef int	ffHandle_t;

/*
enum FFChannel_e
{	FF_CHANNEL_WEAPON
,	FF_CHANNEL_MENU
,	FF_CHANNEL_TOUCH
,	FF_CHANNEL_DAMAGE
,	FF_CHANNEL_VEHICLE
,	FF_CHANNEL_MAX
};
*/
#define FF_CHANNEL_WEAPON	0
#define FF_CHANNEL_MENU		1
#define FF_CHANNEL_TOUCH	2
#define FF_CHANNEL_DAMAGE	3
#define FF_CHANNEL_BODY		4
#define FF_CHANNEL_FORCE	5
#define FF_CHANNEL_FOOT		6
#define FF_CHANNEL_MAX		7

#ifdef _FF

/*
inline qboolean operator &= ( qboolean &lvalue, qboolean rvalue )
{
	lvalue = qboolean( (int)lvalue && (int)rvalue );
	return lvalue;
}
*/

#include "../ff/ff.h"		// basic system functions
#include "../ff/ff_snd.h"	// sound system similarities
#endif // _FF

#endif // __FF_PUBLIC_H