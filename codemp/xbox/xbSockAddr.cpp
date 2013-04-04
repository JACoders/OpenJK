//-----------------------------------------------------------------------------
// File: XbSockAddr.cpp
//
// Desc: Wraps SOCKADDR_IN object
//
// Hist: 05.17.01 - New for June XDK release
//       08.08.01 - Moved to common framework
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "XbSockAddr.h"
#include <cassert>




//-----------------------------------------------------------------------------
// Name: CXBSockAddr()
// Desc: Create from SOCKADDR_IN
//-----------------------------------------------------------------------------
CXBSockAddr::CXBSockAddr( const SOCKADDR_IN& sa )
: 
    sockaddr_in()
{
    assert( sa.sin_family == AF_INET );
    sin_family = AF_INET;
    sin_addr = sa.sin_addr;
    sin_port = sa.sin_port;
}




//-----------------------------------------------------------------------------
// Name: CXBSockAddr()
// Desc: Create from IP address and port
//-----------------------------------------------------------------------------
CXBSockAddr::CXBSockAddr( DWORD inAddr, WORD wPort )
: 
    sockaddr_in()
{
    sin_family = AF_INET;
    sin_addr.s_addr = inAddr;
    sin_port = htons( wPort );
}




//-----------------------------------------------------------------------------
// Name: CXBSockAddr()
// Desc: Create from IN_ADDR and port
//-----------------------------------------------------------------------------
CXBSockAddr::CXBSockAddr( const IN_ADDR& inAddr, WORD wPort )
: 
    sockaddr_in()
{
    sin_family = AF_INET;
    sin_addr = inAddr;
    sin_port = htons( wPort );
}




//-----------------------------------------------------------------------------
// Name: GetInAddr()
// Desc: Extract IN_ADDR
//-----------------------------------------------------------------------------
IN_ADDR CXBSockAddr::GetInAddr() const
{
    return sin_addr;
}




//-----------------------------------------------------------------------------
// Name: GetPtr()
// Desc: Direct (constant) access
//-----------------------------------------------------------------------------
const SOCKADDR_IN* CXBSockAddr::GetPtr() const
{
    return this;
}




//-----------------------------------------------------------------------------
// Name: GetAddr()
// Desc: Socket IP address
//-----------------------------------------------------------------------------
DWORD CXBSockAddr::GetAddr() const
{
    return( ntohl( sin_addr.s_addr ) );
}




//-----------------------------------------------------------------------------
// Name: GetPort()
// Desc: Port number
//-----------------------------------------------------------------------------
WORD CXBSockAddr::GetPort() const
{
    return( ntohs( sin_port ) );
}




//-----------------------------------------------------------------------------
// Name: GetStr()
// Desc: Address in dotted decimal (a.b.c.d) format, with optional port
//       specifier (a.b.c.d:p). Incoming string buffer must have enough
//       room for result (16 WCHARS if no port, 22 if port).
//-----------------------------------------------------------------------------
VOID CXBSockAddr::GetStr( WCHAR* strAddr, BOOL bIncludePort ) const
{
    assert( strAddr != NULL );
    
    INT iChars = wsprintfW( strAddr, L"%d.%d.%d.%d", 
                            sin_addr.S_un.S_un_b.s_b1,
                            sin_addr.S_un.S_un_b.s_b2,
                            sin_addr.S_un.S_un_b.s_b3,
                            sin_addr.S_un.S_un_b.s_b4 );

    if( bIncludePort )
    {
        WCHAR strPort[8];
        wsprintfW( strPort, L":%d", GetPort() );
        lstrcpyW( strAddr + iChars, strPort );
    }
}
