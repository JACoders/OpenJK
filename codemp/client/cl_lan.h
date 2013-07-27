#pragma once

void LAN_LoadCachedServers( );
void LAN_SaveServersToCache( );
void LAN_ResetPings(int source);
int LAN_AddServer(int source, const char *name, const char *address);
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
