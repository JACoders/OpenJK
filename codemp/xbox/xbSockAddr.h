//-----------------------------------------------------------------------------
// File: XbSockAddr.h
//
// Desc: Wraps SOCKADDR_IN object
//
// Hist: 05.17.01 - New for June XDK release
//       08.08.01 - Moved to common framework
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBSOCKADDR_H
#define XBSOCKADDR_H

#include <xtl.h>




//-----------------------------------------------------------------------------
// Name: class CXBSockAddr
// Desc: Xbox socket address object
//-----------------------------------------------------------------------------
class CXBSockAddr : private SOCKADDR_IN
{
public:

    explicit CXBSockAddr( const SOCKADDR_IN& sa );
    CXBSockAddr( DWORD inAddr, WORD wPort );
    CXBSockAddr( const IN_ADDR& inAddr, WORD wPort );

    IN_ADDR            GetInAddr() const;
    const SOCKADDR_IN* GetPtr() const;
    DWORD              GetAddr() const;
    WORD               GetPort() const;
    VOID               GetStr( WCHAR*, BOOL bIncludePort=TRUE ) const;

private:

    // Not used, so not defined
    CXBSockAddr();
    CXBSockAddr( const CXBSockAddr& );

};

#endif // XBSOCKADDR_H
