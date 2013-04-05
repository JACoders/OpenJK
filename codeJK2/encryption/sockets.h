#ifndef __SOCKETS_H
#define __SOCKETS_H

#include <windows.h>
#include <winsock.h>
#include "buffer.h"

class cSocket
{
private:
	SOCKET	Socket;

public:
					cSocket(void);
					cSocket(SOCKET InitSocket);
					~cSocket(void);

	const	SOCKET	GetSocket(void) { return Socket; }

	void			Free(void);

	// straight winsock commands
	bool			socket(int af = PF_INET, int type = SOCK_STREAM, int protocol = IPPROTO_TCP);
	bool			bind(const struct sockaddr FAR *name, int namelen);
	bool			listen(int backlog = 5);
	SOCKET			accept(struct sockaddr FAR *addr = NULL, int FAR *addrlen = 0);
	bool			connect(struct sockaddr FAR *addr, int FAR addrlen);
	int				recv(char FAR *buf, int len, int flags = 0);
	int				send(const char FAR *buf, int len, int flags = 0);
  
	bool			ioctlsocket(long cmd, u_long FAR *argp);
	bool			setsockopt(int level, int optname, const char FAR *optval, int optlen); 
	int				getsockopt(int level, int optname, char FAR *optval, int FAR *optlen);
 	bool			getpeername(struct sockaddr FAR *name, int FAR *namelen);
	bool			getpeername(struct sockaddr_in FAR *name, int FAR *namelen) 
	{
		return getpeername((struct sockaddr FAR *)name, namelen);
	}
 
	// convience functions
	bool			Create(u_short Port);
	bool			Connect(unsigned char u1, unsigned char u2, unsigned char u3, unsigned char u4, unsigned short port);
	cSocket			*GetConnection(void);

	bool			SetBlocking(bool Enabled = false);
	bool			SetKeepAlive(bool Enabled = false);
	bool			SetLinger(bool Enabled = true, u_short TimeLimit = 0);
	bool			SetSendBufferSize(int Size);
};

class cConnection;

typedef void (*Connect_Callback)(cConnection *);

class cConnection
{
protected:
	cSocket				*Socket;
	bool				Reading;
	bool				BufferReset;
	cBuffer				Buffer;
	Connect_Callback	Callback;

public:
			cConnection(cSocket *Init_Socket, Connect_Callback InitCallback = NULL, bool InitReading = true);
			~cConnection(void);

	cBuffer				&GetBuffer(void) { return Buffer; }
	void				Print(char *Format, ...);
	bool				Write(void);
	bool				Read(void);
	bool				Handle(void);

	virtual bool		ReadCallback(void) { return true; }
	virtual bool		WriteCallback(void) { return true; }
};

class cWinsock
{
private:
	bool	WinsockStarted;

public:
	cWinsock(void);
	~cWinsock(void);

	void	Init(void);
	void	Shutdown(void);

	struct servent FAR	*getservbyname(const char FAR *name, const char FAR *proto = "tcp");
	u_short				htons(u_short hostshort);
	struct hostent FAR	*gethostbyaddr(const char FAR *addr, int len, int type = PF_INET);
};

#endif // __SOCKETS_H
