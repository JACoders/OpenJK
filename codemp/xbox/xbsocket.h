//-----------------------------------------------------------------------------
// File: XbSocket.h
//
// Desc: Wraps SOCKET object
//
// Hist: 05.17.01 - New for June XDK release
//       08.08.01 - Moved to common framework
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBSOCKET_H
#define XBSOCKET_H

#include <xtl.h>




//-----------------------------------------------------------------------------
// Name: class Socket
// Desc: Xbox socket object
//-----------------------------------------------------------------------------
class CXBSocket
{
    SOCKET m_Socket;

public:

    enum SocketType
    {
        Type_UDP,
        Type_TCP,
        Type_VDP
    };

    explicit CXBSocket( SOCKET = INVALID_SOCKET );
    explicit CXBSocket( SocketType );
    CXBSocket( INT iType, INT iProtocol );
    ~CXBSocket();

    BOOL   Open( SocketType );
    BOOL   Open( INT iType, INT iProtocol );
    BOOL   IsOpen() const;
    INT    Close();
    SOCKET Accept( SOCKADDR_IN* = NULL );
    INT    Bind( const SOCKADDR_IN* );
    INT    Connect( const SOCKADDR_IN* );
    SOCKET GetSocket() const;
    INT    GetSockName( SOCKADDR_IN* ) const;
    INT    GetSockOpt( INT iLevel, INT iName, VOID* pValue, INT* piSize ) const;
    INT    IoCtlSocket( LONG nCmd, DWORD* pArg );
    INT    Listen( INT iBacklog = SOMAXCONN );
    INT    Recv( VOID* pBuffer, INT iBytes );
    INT    RecvFrom( VOID* pBuffer, INT iBytes, SOCKADDR_IN* = NULL );
    INT    Select( BOOL* pbRead, BOOL* pbWrite, BOOL* pbError );
    INT    Send( const VOID* pBuffer, INT iBytes );
    INT    SendTo( const VOID* pBuffer, INT iBytes, const SOCKADDR_IN* = NULL );
    INT    SetSockOpt( INT iLevel, INT iName, const VOID* pValue, INT iBytes );
    INT    Shutdown( INT iHow );

private:

    CXBSocket( const CXBSocket& );
    CXBSocket& operator=( const CXBSocket& );

};

#endif // XBSOCKET_H
