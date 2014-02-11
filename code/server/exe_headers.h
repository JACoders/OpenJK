/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// stuff added for PCH files.  I want to have a lot of stuff included here so the PCH is pretty rich,
//	but without exposing too many extra protos, so for now (while I experiment)...
//

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "../client/client.h"
#include "../server/server.h"

#ifdef _MSC_VER
#pragma hdrstop
#endif

