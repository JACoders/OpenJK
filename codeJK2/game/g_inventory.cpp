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

// g_inventory.cpp

#include "g_headers.h"

#include "g_local.h"

/*
================
Goodie Keys
================
*/
qboolean INV_GoodieKeyGive( gentity_t *target )
{
	if ( !target || !target->client )
	{
		return (qfalse);
	}

	target->client->ps.inventory[INV_GOODIE_KEY]++;
	return (qtrue);

}

qboolean INV_GoodieKeyTake( gentity_t *target )
{
	if ( !target || !target->client )
	{
		return (qfalse);
	}

	if (target->client->ps.inventory[INV_GOODIE_KEY])
	{
		target->client->ps.inventory[INV_GOODIE_KEY]--;
		return (qtrue);
	}

	//had no keys
	return (qfalse);
}

int INV_GoodieKeyCheck( gentity_t *target )
{
	if ( !target || !target->client )
	{
		return (qfalse);
	}

	if ( target->client->ps.inventory[INV_GOODIE_KEY] )
	{//found a key
		return (INV_GOODIE_KEY);
	}

	//no keys
	return (qfalse);
}

/*
================
Security Keys
================
*/
qboolean INV_SecurityKeyGive( gentity_t *target, const char *keyname )
{
	if ( target == NULL || keyname == NULL || target->client == NULL )
	{
		return qfalse;
	}

	for ( int i = 0; i < MAX_SECURITY_KEYS; i++ )
	{
		if ( target->client->ps.security_key_message[i][0] == '\0' )
		{//fill in the first empty slot we find with this key
			target->client->ps.inventory[INV_SECURITY_KEY]++;	// He got the key
			Q_strncpyz(target->client->ps.security_key_message[i], keyname,
				sizeof(target->client->ps.security_key_message[i]) );
			return qtrue;
		}
	}
	//couldn't find an empty slot
	return qfalse;
}

void INV_SecurityKeyTake( gentity_t *target, const char *keyname )
{
	if ( !target || !keyname || !target->client )
	{
		return;
	}

	for ( int i = 0; i < MAX_SECURITY_KEYS; i++ )
	{
		if ( target->client->ps.security_key_message[i][0] != '\0' )
		{
			if ( !Q_stricmp( keyname, target->client->ps.security_key_message[i] ) )
			{
				target->client->ps.inventory[INV_SECURITY_KEY]--;	// Take the key
				target->client->ps.security_key_message[i][0] = '\0';
				return;
			}
		}
		/*
		//don't do this because we could have removed one that's between 2 valid ones
		else
		{
			break;
		}
		*/
	}
}

qboolean INV_SecurityKeyCheck( gentity_t *target, const char *keyname )
{
	if ( !target || !keyname || !target->client )
	{
		return (qfalse);
	}

	for ( int i = 0; i < MAX_SECURITY_KEYS; i++ )
	{
		if ( target->client->ps.inventory[INV_SECURITY_KEY] && target->client->ps.security_key_message[i][0] != '\0' )
		{
			if ( !Q_stricmp( keyname, target->client->ps.security_key_message[i] ) )
			{
				return (qtrue);
			}
		}
		/*
		//don't do this because we could have removed one that's between 2 valid ones
		else
		{
			break;
		}
		*/
	}

	return (qfalse);
}
