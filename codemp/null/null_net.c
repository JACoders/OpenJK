
#include "../qcommon/qcommon.h"

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	if (!strcmp (s, "localhost")) {
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	return false;
}

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, void *data, netadr_t to ) {
}

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message ) {
	return false;
}
