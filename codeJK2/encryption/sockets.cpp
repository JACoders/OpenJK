#include "encryption.h"

#ifdef _ENCRYPTION_

#include <stdio.h>
#include "sockets.h"

class cWinsock Winsock;



cSocket::cSocket(void)
{
	Socket = INVALID_SOCKET;
}

cSocket::cSocket(SOCKET InitSocket)
:Socket(InitSocket)
{
}

cSocket::~cSocket(void)
{
	Free();
}

void cSocket::Free(void)
{
	if (Socket != INVALID_SOCKET)
	{
		shutdown(Socket, 2);	// 2 = SD_BOTH which is defined for winsock2
		closesocket(Socket);

		Socket = INVALID_SOCKET;
	}
}

bool cSocket::socket(int af, int type, int protocol)
{
	Socket = ::socket(af, type, protocol);

	return (Socket != INVALID_SOCKET);
}

bool cSocket::bind(const struct sockaddr FAR *name, int namelen)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::bind(Socket, name, namelen) != SOCKET_ERROR);
}

bool cSocket::listen(int backlog)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::listen(Socket, backlog) != SOCKET_ERROR);
}

SOCKET cSocket::accept(struct sockaddr FAR *addr, int FAR *addrlen)
{
	if (Socket == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	return ::accept(Socket, addr, addrlen);
}

bool cSocket::connect(struct sockaddr FAR *addr, int FAR addrlen)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::connect(Socket, addr, addrlen) != SOCKET_ERROR);
}

int cSocket::recv(char FAR *buf, int len, int flags)
{
	if (Socket == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	return ::recv(Socket, buf, len, flags);
}

int cSocket::send(const char FAR *buf, int len, int flags)
{
	if (Socket == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	return ::send(Socket, buf, len, flags);
}



bool cSocket::ioctlsocket(long cmd, u_long FAR *argp)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::ioctlsocket(Socket, cmd, argp) != SOCKET_ERROR);
}

bool cSocket::setsockopt(int level, int optname, const char FAR *optval, int optlen)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::setsockopt(Socket, level, optname, optval, optlen) != SOCKET_ERROR);
}

int cSocket::getsockopt(int level, int optname, char FAR *optval, int FAR *optlen)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return ::getsockopt(Socket, level, optname, optval, optlen);
}

bool cSocket::getpeername(struct sockaddr FAR *name, int FAR *namelen)
{
	if (Socket == INVALID_SOCKET)
	{
		return false;
	}

	return (::getpeername(Socket, name, namelen) != SOCKET_ERROR);
}


bool cSocket::Create(u_short Port)
{
	struct sockaddr_in addr;

    addr.sin_port = Winsock.htons(Port);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = PF_INET;

	Free();

	if (socket())
	{
		if (bind((const struct sockaddr *)&addr, sizeof(addr)))
		{
			return true;
		}

		Free();
	}

	return false;
}

bool cSocket::Connect(unsigned char U1, unsigned char U2, unsigned char U3, unsigned char U4, unsigned short Port)
{
	struct sockaddr_in addr;

    addr.sin_port = Winsock.htons(Port);
    addr.sin_addr.s_net = U1;
    addr.sin_addr.s_host = U2;
    addr.sin_addr.s_lh = U3;
    addr.sin_addr.s_impno = U4;
    addr.sin_family = PF_INET;

	Free();

	if (socket())
	{
		if (connect((struct sockaddr *)&addr, sizeof(addr)))
		{
			return true;
		}

		Free();
	}

	return false;
}

cSocket *cSocket::GetConnection(void)
{
	SOCKET	new_socket;

	new_socket = accept();
	if (new_socket != INVALID_SOCKET)
	{
		return new cSocket(new_socket);
	}

	return NULL;
}

bool cSocket::SetBlocking(bool Enabled)
{
    unsigned long	blockval = (Enabled != true);

	return ioctlsocket(FIONBIO, &blockval);
}

bool cSocket::SetKeepAlive(bool Enabled)
{
    int				keepaliveval = (Enabled == true);

	return setsockopt(SOL_SOCKET, SO_KEEPALIVE, (char*)&keepaliveval, sizeof(keepaliveval));
}

bool cSocket::SetLinger(bool Enabled, u_short TimeLimit)
{
	struct linger lingerval;
	
	lingerval.l_onoff = (Enabled == true);
	lingerval.l_linger = TimeLimit;

	return setsockopt(SOL_SOCKET, SO_LINGER, (char*)&lingerval, sizeof(lingerval));
}

bool cSocket::SetSendBufferSize(int Size)
{
	return setsockopt(SOL_SOCKET, SO_SNDBUF, (char*)&Size, sizeof(Size));
}


















cConnection::cConnection(cSocket *Init_Socket, Connect_Callback InitCallback, bool InitReading)
:Socket(Init_Socket),
 Callback(InitCallback),
 Reading(InitReading)
{
	BufferReset = false;
}

cConnection::~cConnection(void)
{
	if (Socket)
	{
		delete Socket;
	}
}

void cConnection::Print(char *Format, ...)
{
	char	temp[1024];
	va_list argptr;
	int		size;

	va_start (argptr, Format);
	size = _vsnprintf (temp, sizeof(temp), Format, argptr);
	va_end (argptr);

	Buffer.Add(temp, size);
}

bool cConnection::Read(void)
{
	char	temp[8193];
	int		size;

	size = Socket->recv(temp, sizeof(temp)-1);
	if (size >= 0)
	{
		Buffer.Add(temp, size);

		temp[size] = 0;
	}
	if (Buffer.Get() && strstr(Buffer.Get(), "\r\n\r\n"))
	{	// found the blank line
		temp[0] = 0;
		Buffer.Add(temp, 1);

		Reading = false;
		Buffer.FreeBeforeNextAdd();

		if (Callback)
		{
			Callback(this);
		}
		return ReadCallback();
	}

	return false;
}

bool cConnection::Write(void)
{
	int		size;

	if (!BufferReset)
	{
		BufferReset = true;
		Buffer.Read();
	}

	size = Socket->send(Buffer.GetWithPos(), Buffer.GetRemaining() );
	if (size != SOCKET_ERROR)
	{
		if (!Buffer.Read(size))
		{
			return WriteCallback();
		}
	}

	return false;
}

bool cConnection::Handle(void)
{
	if (Reading)
	{
		return Read();
	}
	else
	{
		return Write();
	}
}
























cWinsock::cWinsock(void)
{
	WinsockStarted = false;
}

cWinsock::~cWinsock(void)
{
	Shutdown();
}

void cWinsock::Init(void)
{
	WSADATA		dat;
	int			rv;

	if (WinsockStarted)
	{
		return;
	}

#ifdef _USE_WINSOCK2_
	rv = WSAStartup(MAKEWORD(2,0), &dat);
#else
	rv = WSAStartup(MAKEWORD(1,1), &dat);
#endif

	if (rv == 0)
	{
		WinsockStarted = true;
	}
}

void cWinsock::Shutdown(void)
{
	if (WinsockStarted)
	{
		WSACleanup();
		WinsockStarted = false;
	}
}

struct servent FAR *cWinsock::getservbyname(const char FAR *name, const char FAR *proto)
{
	return ::getservbyname(name, proto);
}

u_short cWinsock::htons(u_short hostshort)
{
	return ::htons(hostshort);
}

struct hostent FAR *cWinsock::gethostbyaddr(const char FAR *addr, int len, int type)
{
	return ::gethostbyaddr(addr, len, type);
}
#endif
