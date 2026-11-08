/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cl_presence.h -- the C1B3R session, for what else talks to C1B3R on it (the datapad's apps)

#pragma once

#include <string>
#include "cl_http.h"

const char	*CL_Presence_Session( void );		// the session's id, NULL while there's none
qboolean	CL_Presence_Post( httpRequest_t *req, const char *path, const std::string &body );
void		CL_Presence_Json( std::string &out, const char *s );	// a quoted JSON string
std::string	CL_Presence_Unfield( const char *in );				// an answer's field as it was sent
