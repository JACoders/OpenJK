/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

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

#include "cg_local.h"

typedef struct clightstyle_s {
	int				length;
	color4ub_t		value;
	color4ub_t		map[MAX_QPATH];
} clightstyle_t;

static	clightstyle_t	cl_lightstyle[MAX_LIGHT_STYLES];
static	int				lastofs;

/*
================
CG_ClearLightStyles
================
*/
void CG_ClearLightStyles (void)
{
	int	i;

	memset( cl_lightstyle, 0, sizeof( cl_lightstyle ) );
	lastofs = -1;

	for ( i=0; i<MAX_LIGHT_STYLES*3; i++ )
		CG_SetLightstyle( i );
}

/*
================
CG_RunLightStyles
================
*/
void CG_RunLightStyles (void)
{
	int ofs, i;
	clightstyle_t *ls;

	ofs = cg.time / 50;
//	if ( ofs == lastofs )
//		return;
	lastofs = ofs;

	for ( i=0, ls=cl_lightstyle; i<MAX_LIGHT_STYLES; i++, ls++ ) {
		byteAlias_t *ba = (byteAlias_t *)&ls->value;

		ls->value[3] = 255;
		if ( !ls->length ) {
			ls->value[0] = ls->value[1] = ls->value[2] = 255;
		}
		else if ( ls->length == 1 ) {
			ls->value[0] = ls->map[0][0];
			ls->value[1] = ls->map[0][1];
			ls->value[2] = ls->map[0][2];
		//	ls->value[3] = ls->map[0][3];
		}
		else {
			ls->value[0] = ls->map[ofs%ls->length][0];
			ls->value[1] = ls->map[ofs%ls->length][1];
			ls->value[2] = ls->map[ofs%ls->length][2];
		//	ls->value[3] = ls->map[ofs%ls->length][3];
		}

		trap->R_SetLightStyle( i, ba->i );
	}
}

void CG_SetLightstyle (int i)
{
	const char	*s;
	int			j, k;

	s = CG_ConfigString( i+CS_LIGHT_STYLES );
	j = strlen (s);
	if (j >= MAX_QPATH)
	{
		Com_Error (ERR_DROP, "svc_lightstyle length=%i", j);
	}

	cl_lightstyle[(i/3)].length = j;
	for (k=0 ; k<j ; k++)
	{
		cl_lightstyle[(i/3)].map[k][(i%3)] = (float)(s[k]-'a')/(float)('z'-'a') * 255.0;
	}
}
