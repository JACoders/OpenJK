//-----------------------------------------------------------------------------
// File: XbSocket.cpp
//
// Desc: Wraps SOCKET object
//
// Hist: 05.17.01 - New for June XDK release
//       08.08.01 - Moved to common framework
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "XbSocket.h"
#include <cassert>




//-----------------------------------------------------------------------------
// Name: CXBSocket()
// Desc: Associate existing socket with object. Object will close the socket
//       when it is destroyed.
//-----------------------------------------------------------------------------
CXBSocket::CXBSocket( SOCKET sock )
:
    m_Socket( sock )
{
}




//-----------------------------------------------------------------------------
// Name: CXBSocket()
// Desc: Create a socket of the given type
//-----------------------------------------------------------------------------
CXBSocket::CXBSocket( SocketType type )
:
    m_Socket( INVALID_SOCKET )
{
    BOOL bSuccess = Open( type );
    assert( bSuccess );
    (VOID)bSuccess;
}






//-----------------------------------------------------------------------------
// Name: CXBSocket()
// Desc: Create a socket of the given type/protocol
//-----------------------------------------------------------------------------
CXBSocket::CXBSocket( INT iType, INT iProtocol )
:
    m_Socket( INVALID_SOCKET )
{
    BOOL bSuccess = Open( iType, iProtocol );
    assert( bSuccess );
    (VOID)bSuccess;
}




//-----------------------------------------------------------------------------
// Name: ~CXBSocket()
// Desc: Close and release socket
//-----------------------------------------------------------------------------
CXBSocket::~CXBSocket()
{
    Close();
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Open a socket of the given type
//-----------------------------------------------------------------------------
BOOL CXBSocket::Open( SocketType type )
{
    switch( type )
    {
    case Type_UDP:
        return Open( SOCK_DGRAM, IPPROTO_UDP );

    case Type_TCP:
        return Open( SOCK_STREAM, IPPROTO_TCP );

    default:
        assert( type == Type_VDP );
        return Open( SOCK_DGRAM, IPPROTO_VDP );
    }
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Open a socket of the given type/protocol
//-----------------------------------------------------------------------------
BOOL CXBSocket::Open( INT iType, INT iProtocol )
{
    Close();
    m_Socket = socket( AF_INET, iType, iProtocol );
    return( m_Socket != INVALID_SOCKET );
}




//-----------------------------------------------------------------------------
// Name: IsOpen()
// Desc: TRUE if socket is open
//-----------------------------------------------------------------------------
BOOL CXBSocket::IsOpen() const
{
    return( m_Socket != INVALID_SOCKET );
}




//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Close socket
//-----------------------------------------------------------------------------
INT CXBSocket::Close()
{
    INT iResult = 0;
    if( m_Socket != INVALID_SOCKET )
    {
        iResult = closesocket( m_Socket );
        m_Socket = INVALID_SOCKET;
    }
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Accept()
// Desc: Permit incoming connection attempt
//-----------------------------------------------------------------------------
SOCKET CXBSocket::Accept( SOCKADDR_IN* pSockAddr )
{
    assert( m_Socket != INVALID_SOCKET );
    INT iSize = sizeof( SOCKADDR_IN );

    SOCKET sockResult = accept( m_Socket, (sockaddr*)(pSockAddr), &iSize );

    if( sockResult != INVALID_SOCKET && pSockAddr != NULL )
        assert( iSize == sizeof( SOCKADDR_IN ) );
    return sockResult;
}




//-----------------------------------------------------------------------------
// Name: Bind()
// Desc: Associate local address with socket
//-----------------------------------------------------------------------------
INT CXBSocket::Bind( const SOCKADDR_IN* pSockAddr )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pSockAddr != NULL );
    assert( pSockAddr->sin_family == AF_INET );

    INT iResult = bind( m_Socket, (const sockaddr*)(pSockAddr), 
                        sizeof( SOCKADDR_IN ) );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Connect()
// Desc: Connect socket
//-----------------------------------------------------------------------------
INT CXBSocket::Connect( const SOCKADDR_IN* pSockAddr )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pSockAddr != NULL );
    assert( pSockAddr->sin_family == AF_INET );

    INT iResult = connect( m_Socket, (const sockaddr*)(pSockAddr), 
                           sizeof( SOCKADDR_IN ) );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: GetSocket()
// Desc: Returns the socket handle
//-----------------------------------------------------------------------------
SOCKET CXBSocket::GetSocket() const
{
    return m_Socket;
}



//-----------------------------------------------------------------------------
// Name: GetSockName()
// Desc: Get socket "name"
//-----------------------------------------------------------------------------
INT CXBSocket::GetSockName( SOCKADDR_IN* pSockAddr ) const
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pSockAddr != NULL );
    INT iSize = sizeof( SOCKADDR_IN );

    INT iResult = getsockname( m_Socket, (sockaddr*)(pSockAddr), &iSize );

    if( iResult != SOCKET_ERROR )
        assert( iSize == sizeof( SOCKADDR_IN ) );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: GetSockOpt()
// Desc: Get socket option
//-----------------------------------------------------------------------------
INT CXBSocket::GetSockOpt( INT iLevel, INT iName, VOID* pValue, INT* piSize ) const
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pValue != NULL );
    assert( piSize != NULL );

    INT iResult = getsockopt( m_Socket, iLevel, iName, (CHAR*)(pValue), piSize );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: IoCtlSocket()
// Desc: Configure socket I/O mode
//-----------------------------------------------------------------------------
INT CXBSocket::IoCtlSocket( LONG nCmd, DWORD* pArg )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pArg != NULL );

    INT iResult = ioctlsocket( m_Socket, nCmd, pArg );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Listen()
// Desc: Listen for incoming connection
//-----------------------------------------------------------------------------
INT CXBSocket::Listen( INT iBacklog )
{
    assert( m_Socket != INVALID_SOCKET );

    INT iResult = listen( m_Socket, iBacklog );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Recv()
// Desc: Receive data on socket
//-----------------------------------------------------------------------------
INT CXBSocket::Recv( VOID* pBuffer, INT iBytes )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pBuffer != NULL );
    assert( iBytes >= 0 );

    INT iResult = recv( m_Socket, (CHAR*)(pBuffer), iBytes, 0 );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: RecvFrom()
// Desc: Receive data on socket and report source address
//-----------------------------------------------------------------------------
INT CXBSocket::RecvFrom( VOID* pBuffer, INT iBytes, SOCKADDR_IN* pSockAddr )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pBuffer != NULL );
    assert( iBytes >= 0 );
    INT iSize = sizeof( SOCKADDR_IN );

    INT iResult = recvfrom( m_Socket, (CHAR*)(pBuffer), iBytes, 0, 
                            (sockaddr*)(pSockAddr), &iSize );
    if( iResult != SOCKET_ERROR && pSockAddr != NULL )
        assert( iSize == sizeof( SOCKADDR_IN ) );

    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Select()
// Desc: Does a select call to check status of socket - returns separate 
//       BOOLs for read, write, and error
//-----------------------------------------------------------------------------
INT CXBSocket::Select( BOOL* pbRead, BOOL* pbWrite, BOOL* pbError )
{
    assert( m_Socket != INVALID_SOCKET );

    INT iResultTotal = 0;

    timeval tv = {0};
    if( pbRead )
    {
        fd_set fdsRead = {0};
        FD_SET( m_Socket, &fdsRead );

        INT iResult = select( 0, &fdsRead, NULL, NULL, &tv );
        assert( iResult != SOCKET_ERROR );

        *pbRead = ( iResult == 1 );
        iResultTotal += iResult;
    }

    if( pbWrite )
    {
        fd_set fdsWrite = {0};
        FD_SET( m_Socket, &fdsWrite );

        INT iResult = select( 0, NULL, &fdsWrite, NULL, &tv );
        assert( iResult != SOCKET_ERROR );

        *pbWrite = ( iResult == 1 );
        iResultTotal += iResult;
    }

    if( pbError )
    {
        fd_set fdsError = {0};
        FD_SET( m_Socket, &fdsError );

        INT iResult = select( 0, NULL, NULL, &fdsError, &tv );
        assert( iResult != SOCKET_ERROR );

        *pbError = ( iResult == 1 );
        iResultTotal += iResult;
    }
    
    return iResultTotal;
}



//-----------------------------------------------------------------------------
// Name: Send()
// Desc: Send data on socket
//-----------------------------------------------------------------------------
INT CXBSocket::Send( const VOID* pBuffer, INT iBytes )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pBuffer != NULL );
    assert( iBytes >= 0 );

    INT iResult = send( m_Socket, (const CHAR*)(pBuffer), iBytes, 0 );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: SendTo()
// Desc: Send data on socket to specific destination
//-----------------------------------------------------------------------------
INT CXBSocket::SendTo( const VOID* pBuffer, INT iBytes, const SOCKADDR_IN* pSockAddr )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pBuffer != NULL );
    assert( iBytes >= 0 );

    INT iResult = sendto( m_Socket, (const CHAR*)(pBuffer), iBytes, 0, 
                          (const sockaddr*)(pSockAddr), sizeof( SOCKADDR_IN ) );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: SetSockOpt()
// Desc: Set socket option
//-----------------------------------------------------------------------------
INT CXBSocket::SetSockOpt( INT iLevel, INT iName, const VOID* pValue, INT iBytes )
{
    assert( m_Socket != INVALID_SOCKET );
    assert( pValue != NULL );

    INT iResult = setsockopt( m_Socket, iLevel, iName, (const CHAR*)(pValue), 
                              iBytes );
    return iResult;
}




//-----------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Disabled sending and/or receiving on socket
//-----------------------------------------------------------------------------
INT CXBSocket::Shutdown( INT iHow )
{
    assert( m_Socket != INVALID_SOCKET );

    INT iResult = shutdown( m_Socket, iHow );
    return iResult;
}
