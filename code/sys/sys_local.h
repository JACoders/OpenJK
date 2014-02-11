#ifndef SYS_LOCAL_H_
#define SYS_LOCAL_H_

void 		IN_Init( void *windowData );
void 		IN_Frame (void);
void 		IN_Shutdown( void );
void 		IN_Restart( void );

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );
char		*Sys_ConsoleInput (void);
void 		Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

void Sys_Exit( int ex ) __attribute__((noreturn));

#endif /* SYS_LOCAL_H_ */
