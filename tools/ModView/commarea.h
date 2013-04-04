// Filename:-	CommArea.h
//
// headers for inter-program communication
//

#ifndef COMMAREA_H
#define COMMAREA_H

// setup functions...
//
LPCSTR	CommArea_ServerInitOnceOnly(void);
LPCSTR	CommArea_ClientInitOnceOnly(void);
void	CommArea_ShutDown(void);
//
// size-limit internal buffer-query functions...
//
int		CommArea_GetMaxDataSize(void);
int		CommArea_GetMaxCommandStrlen(void);
int		CommArea_GetMaxErrorStrlen(void);
//
// message-pending query functions...
//
bool	CommArea_IsIdle(void);
LPCSTR	CommArea_IsCommandWaiting(byte **ppbDataPassback, int *piDatasizePassback);
LPCSTR	CommArea_IsErrorWaiting(void);
LPCSTR	CommArea_IsAckWaiting(byte **ppbDataPassback = NULL, int *piDatasizePassback = NULL);
//
// message-acknowledge functions...
//
LPCSTR	CommArea_CommandAck(LPCSTR psCommand = NULL, byte *pbData = NULL, int iDataSize = 0);
LPCSTR	CommArea_CommandClear(void);
LPCSTR	CommArea_CommandError(LPCSTR psError);
//
// message/command-send functions...
//
LPCSTR	CommArea_IssueCommand(LPCSTR psCommand, byte *pbData = NULL, int iDataSize = 0);


#endif	// #ifndef COMMAREA_H

/////////////// eof /////////////

