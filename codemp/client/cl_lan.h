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

#pragma once

void LAN_LoadCachedServers( );
void LAN_SaveServersToCache( );
void LAN_ResetPings(int source);
int LAN_AddServer(int source, const char *name, const char *address);
int LAN_AddFavAddr( const char *address );
void LAN_RemoveServer(int source, const char *addr);
int LAN_GetServerCount( int source );
void LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int LAN_GetServerPing( int source, int n );
int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
int LAN_GetPingQueueCount( void );
void LAN_ClearPing( int n );
void LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void LAN_GetPingInfo( int n, char *buf, int buflen );
void LAN_MarkServerVisible(int source, int n, qboolean visible );
int LAN_ServerIsVisible(int source, int n );
qboolean LAN_UpdateVisiblePings(int source );
int LAN_GetServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
