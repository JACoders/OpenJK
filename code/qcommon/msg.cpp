/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "q_shared.h"
#include "qcommon.h"
#include "../server/server.h"

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/


void MSG_Init( msg_t *buf, byte *data, int length ) {
	memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void MSG_Clear( msg_t *buf ) {
	buf->cursize = 0;
	buf->overflowed = qfalse;
	buf->bit = 0;
}


void MSG_BeginReading( msg_t *msg ) {
	msg->readcount = 0;
	msg->bit = 0;
}


void MSG_ReadByteAlign( msg_t *buf ) {
	// round up to the next byte
	if ( buf->bit ) {
		buf->bit = 0;
		buf->readcount++;
	}
}

void *MSG_GetSpace( msg_t *buf, int length ) {
	void	*data;

	// round up to the next byte
	if ( buf->bit ) {
		buf->bit = 0;
		buf->cursize++;
	}

	if ( buf->cursize + length > buf->maxsize )	{
		if ( !buf->allowoverflow ) {
			Com_Error (ERR_FATAL, "MSG_GetSpace: overflow without allowoverflow set");
		}
		if ( length > buf->maxsize ) {
			Com_Error (ERR_FATAL, "MSG_GetSpace: %i is > full buffer size", length);
		}
		Com_Printf ("MSG_GetSpace: overflow\n");
		MSG_Clear (buf); 
		buf->overflowed = qtrue;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;
	
	return data;
}

void MSG_WriteData( msg_t *buf, const void *data, int length ) {
	memcpy (MSG_GetSpace(buf,length),data,length);		
}


/*
=============================================================================

bit functions
  
=============================================================================
*/

int	overflows;

// negative bit values include signs
void MSG_WriteBits( msg_t *msg, int value, int bits ) {
	int		put;
	int		fraction;

	// this isn't an exact overflow check, but close enough
	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = qtrue;
#ifndef FINAL_BUILD
		Com_Printf (S_COLOR_RED"MSG_WriteBits: buffer Full writing %d in %d bits\n", value, bits);
#endif
		return;
	}

	if ( bits == 0 || bits < -31 || bits > 32 ) {
		Com_Error( ERR_DROP, "MSG_WriteBits: bad bits %i", bits );
	}

	// check for overflows
	if ( bits != 32 ) {
		if ( bits > 0 ) {
			if ( value > ( ( 1 << bits ) - 1 ) || value < 0 ) {
				overflows++;
#ifndef FINAL_BUILD
#ifdef _DEBUG
				Com_Printf (S_COLOR_RED"MSG_WriteBits: overflow writing %d in %d bits\n", value, bits);
#endif
#endif
			}
		} else {
			int	r;

			r = 1 << (bits-1);

			if ( value >  r - 1 || value < -r ) {
				overflows++;
#ifndef FINAL_BUILD
#ifdef _DEBUG
				Com_Printf (S_COLOR_RED"MSG_WriteBits: overflow writing %d in %d bits\n", value, bits);
#endif
#endif
			}
		}
	}
	if ( bits < 0 ) {
		bits = -bits;
	}

	while ( bits ) {
		if ( msg->bit == 0 ) {
			msg->data[msg->cursize] = 0;
			msg->cursize++;
		}
		put = 8 - msg->bit;
		if ( put > bits ) {
			put = bits;
		}
		fraction = value & ( ( 1 << put ) - 1 );
		msg->data[msg->cursize - 1] |= fraction << msg->bit;
		bits -= put;
		value >>= put;
		msg->bit = ( msg->bit + put ) & 7;
	}
}

int MSG_ReadBits( msg_t *msg, int bits ) {
	int		value;
	int		valueBits;
	int		get;
	int		fraction;
	qboolean	sgn;

	value = 0;
	valueBits = 0;

	if ( bits < 0 ) {
		bits = -bits;
		sgn = qtrue;
	} else {
		sgn = qfalse;
	}

	while ( valueBits < bits ) {
		if ( msg->bit == 0 ) {
			msg->readcount++;
			assert (msg->readcount <= msg->cursize);
		}
		get = 8 - msg->bit;
		if ( get > (bits - valueBits) ) {
			get = (bits - valueBits);
		}
		fraction = msg->data[msg->readcount - 1];
		fraction >>= msg->bit;
		fraction &= ( 1 << get ) - 1;
		value |= fraction << valueBits;

		valueBits += get;
		msg->bit = ( msg->bit + get ) & 7;
	}

	if ( sgn ) {
		if ( value & ( 1 << ( bits - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << bits ) - 1 );
		}
	}

	return value;
}



//================================================================================

//
// writing functions
//

void MSG_WriteByte( msg_t *sb, int c ) {
#ifdef PARANOID
	if (c < 0 || c > 255)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif

	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteShort( msg_t *sb, int c ) {
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif

	MSG_WriteBits( sb, c, 16 );
}

static void MSG_WriteSShort( msg_t *sb, int c ) {
	MSG_WriteBits( sb, c, -16 );
}

void MSG_WriteLong( msg_t *sb, int c ) {
	MSG_WriteBits( sb, c, 32 );
}

void MSG_WriteString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData (sb, "", 1);
	} else {
		int		l, i;
		char	string[MAX_STRING_CHARS];

		l = strlen( s );
		if ( l >= MAX_STRING_CHARS ) {
			Com_Printf( "MSG_WriteString: MAX_STRING_CHARS" );
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		// get rid of 0xff chars, because old clients don't like them
		for ( i = 0 ; i < l ; i++ ) {
			if ( ((byte *)string)[i] > 127 ) {
				string[i] = '.';
			}
		}

		MSG_WriteData (sb, string, l+1);
	}
}



//============================================================

//
// reading functions
//

// returns -1 if no more characters are available
int MSG_ReadByte( msg_t *msg ) {
	int	c;
	
	if ( msg->readcount+1 > msg->cursize ) {
		c = -1;
	} else {
		c = (unsigned char)MSG_ReadBits( msg, 8 );
	}
	
	return c;
}

int MSG_ReadShort( msg_t *msg ) {
	int	c;
	
	if ( msg->readcount+2 > msg->cursize ) {
		c = -1;
	} else {
		c = MSG_ReadBits( msg, 16 );
	}

	return c;
}

static int MSG_ReadSShort( msg_t *msg ) {
	int	c;
	
	if ( msg->readcount+2 > msg->cursize ) {
		c = -1;
	} else {
		c = MSG_ReadBits( msg, -16 );
	}

	return c;
}

int MSG_ReadLong( msg_t *msg ) {
	int	c;
	
	if ( msg->readcount+4 > msg->cursize ) {
		c = -1;
	} else {
		c = MSG_ReadBits( msg, 32 );
	}
	
	return c;
}

char *MSG_ReadString( msg_t *msg ) {
	static const int STRING_SIZE = MAX_STRING_CHARS;
	static char	string[STRING_SIZE];
	int		l,c;
	
	MSG_ReadByteAlign( msg );
	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if ( c == -1 || c == 0 ) {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}

		string[l] = c;
		l++;
	} while (l < STRING_SIZE - 1);
	
	string[l] = 0;
	
	return string;
}

char *MSG_ReadStringLine( msg_t *msg ) {
	static const int STRING_SIZE = MAX_STRING_CHARS;
	static char	string[STRING_SIZE];
	int		l,c;

	MSG_ReadByteAlign( msg );
	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if (c == -1 || c == 0 || c == '\n') {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}
		string[l] = c;
		l++;
	} while (l < STRING_SIZE - 1);
	
	string[l] = 0;
	
	return string;
}


void MSG_ReadData( msg_t *msg, void *data, int len ) {
	int		i;

	MSG_ReadByteAlign( msg );
	for (i=0 ; i<len ; i++) {
		((byte *)data)[i] = MSG_ReadByte (msg);
	}
}


/*
=============================================================================

delta functions
  
=============================================================================
*/

extern cvar_t *cl_shownet;

#define	LOG(x) if( cl_shownet->integer == 4 ) { Com_Printf("%s ", x ); };

void MSG_WriteDelta( msg_t *msg, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		MSG_WriteBits( msg, 0, 1 );
		return;
	}
	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, newV, bits );
}

int	MSG_ReadDelta( msg_t *msg, int oldV, int bits ) {
	if ( MSG_ReadBits( msg, 1 ) ) {
		return MSG_ReadBits( msg, bits );
	}
	return oldV;
}

void MSG_WriteDeltaFloat( msg_t *msg, float oldV, float newV ) {
	byteAlias_t fi;
	if ( oldV == newV ) {
		MSG_WriteBits( msg, 0, 1 );
		return;
	}
	fi.f = newV;
	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, fi.i, 32 );
}

float MSG_ReadDeltaFloat( msg_t *msg, float oldV ) {
	if ( MSG_ReadBits( msg, 1 ) ) {
		byteAlias_t fi;

		fi.i = MSG_ReadBits( msg, 32 );
		return fi.f;
	}
	return oldV;
}


/*
============================================================================

usercmd_t communication

============================================================================
*/

// ms is allways sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define CM_WEAPON	(1<<7)

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *to ) {
	MSG_WriteDelta( msg, from->serverTime, to->serverTime, 32 );
	MSG_WriteDelta( msg, from->angles[0], to->angles[0], 16 );
	MSG_WriteDelta( msg, from->angles[1], to->angles[1], 16 );
	MSG_WriteDelta( msg, from->angles[2], to->angles[2], 16 );
	MSG_WriteDelta( msg, from->forwardmove, to->forwardmove, -8 );
	MSG_WriteDelta( msg, from->rightmove, to->rightmove, -8 );
	MSG_WriteDelta( msg, from->upmove, to->upmove, -8 );
	MSG_WriteDelta( msg, from->buttons, to->buttons, 16 );//FIXME:  We're only really using 9 bits...can this be changed to that?
	MSG_WriteDelta( msg, from->weapon, to->weapon, 8 );
	MSG_WriteDelta( msg, from->generic_cmd, to->generic_cmd, 8 );
}


/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *to ) {
	to->serverTime = MSG_ReadDelta( msg, from->serverTime, 32);
	to->angles[0] = MSG_ReadDelta( msg, from->angles[0], 16);
	to->angles[1] = MSG_ReadDelta( msg, from->angles[1], 16);
	to->angles[2] = MSG_ReadDelta( msg, from->angles[2], 16);
	to->forwardmove = MSG_ReadDelta( msg, from->forwardmove, -8);
	to->rightmove = MSG_ReadDelta( msg, from->rightmove, -8);
	to->upmove = MSG_ReadDelta( msg, from->upmove, -8);
	to->buttons = MSG_ReadDelta( msg, from->buttons, 16);//FIXME:  We're only really using 9 bits...can this be changed to that?
	to->weapon = MSG_ReadDelta( msg, from->weapon, 8);
	to->generic_cmd = MSG_ReadDelta( msg, from->generic_cmd, 8);
}

/*
=============================================================================

entityState_t communication
  
=============================================================================
*/

typedef struct {
	const char	*name;
	size_t		offset;
	int		bits;		// 0 = float
} netField_t;

// using the stringizing operator to save typing...
#define	NETF(x) #x,offsetof(entityState_t, x)

#if 0	// Removed by BTO (VV)
const netField_t	entityStateFields[] = 
{
{ NETF(eType), 8 },
{ NETF(eFlags), 32 },

{ NETF(pos.trType), 8 },
{ NETF(pos.trTime), 32 },
{ NETF(pos.trDuration), 32 },
{ NETF(pos.trBase[0]), 0 },
{ NETF(pos.trBase[1]), 0 },
{ NETF(pos.trBase[2]), 0 },
{ NETF(pos.trDelta[0]), 0 },
{ NETF(pos.trDelta[1]), 0 },
{ NETF(pos.trDelta[2]), 0 },

{ NETF(apos.trType), 8 },
{ NETF(apos.trTime), 32 },
{ NETF(apos.trDuration), 32 },
{ NETF(apos.trBase[0]), 0 },
{ NETF(apos.trBase[1]), 0 },
{ NETF(apos.trBase[2]), 0 },
{ NETF(apos.trDelta[0]), 0 },
{ NETF(apos.trDelta[1]), 0 },
{ NETF(apos.trDelta[2]), 0 },

{ NETF(time), 32 },
{ NETF(time2), 32 },

{ NETF(origin[0]), 0 },
{ NETF(origin[1]), 0 },
{ NETF(origin[2]), 0 },

{ NETF(origin2[0]), 0 },
{ NETF(origin2[1]), 0 },
{ NETF(origin2[2]), 0 },

{ NETF(angles[0]), 0 },
{ NETF(angles[1]), 0 },
{ NETF(angles[2]), 0 },

{ NETF(angles2[0]), 0 },
{ NETF(angles2[1]), 0 },
{ NETF(angles2[2]), 0 },

{ NETF(otherEntityNum), GENTITYNUM_BITS },
//{ NETF(otherEntityNum2), GENTITYNUM_BITS },
{ NETF(groundEntityNum), GENTITYNUM_BITS },

{ NETF(constantLight), 32 },
{ NETF(loopSound), 16 },
{ NETF(modelindex), 9 },	//0 to 511
{ NETF(modelindex2), 8 },
{ NETF(modelindex3), 8 },
{ NETF(clientNum), 32 },
{ NETF(frame), 16 },

{ NETF(solid), 24 },

{ NETF(event), 10 },
{ NETF(eventParm), 16 },

{ NETF(powerups), 16 },
{ NETF(weapon), 8 },
{ NETF(legsAnim), 16 },
{ NETF(legsAnimTimer), 8 },
{ NETF(torsoAnim), 16 },
{ NETF(torsoAnimTimer), 8 },
{ NETF(scale), 8 },

{ NETF(saberInFlight), 4 },
{ NETF(saberActive), 4 },
{ NETF(vehicleArmor), 32 },
{ NETF(vehicleAngles[0]), 0 },
{ NETF(vehicleAngles[1]), 0 },
{ NETF(vehicleAngles[2]), 0 },
{ NETF(m_iVehicleNum), 32 },

/*
Ghoul2 Insert Start
*/
{ NETF(modelScale[0]), 0 },
{ NETF(modelScale[1]), 0 },
{ NETF(modelScale[2]), 0 },
{ NETF(radius), 16 },
{ NETF(boltInfo), 32 },
//{ NETF(ghoul2), 32 },

{ NETF(isPortalEnt), 1 },

};
#endif


// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

void MSG_WriteField (msg_t *msg, const int *toF, const netField_t *field)
{
	int			trunc;
	float		fullFloat;

	if ( field->bits == -1)
	{	// a -1 in the bits field means it's a float that's always between -1 and 1
		int temp = *(float *)toF * 32767;
 		MSG_WriteBits( msg, temp, -16 );
	}
	else
 	if ( field->bits == 0 ) {
 		// float
 		fullFloat = *(float *)toF;
 		trunc = (int)fullFloat;

		if (fullFloat == 0.0f) {
			MSG_WriteBits( msg, 0, 1 );	//it's a zero
		} else {
			MSG_WriteBits( msg, 1, 1 );	//not a zero
			if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && 
				trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
				// send as small integer
				MSG_WriteBits( msg, 0, 1 );
				MSG_WriteBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
			} else {
				// send as full floating point value
				MSG_WriteBits( msg, 1, 1 );
				MSG_WriteBits( msg, *toF, 32 );
			}
		}
 	} else {
		if (*toF == 0) {
			MSG_WriteBits( msg, 0, 1 );	//it's a zero
		} else {
			MSG_WriteBits( msg, 1, 1 );	//not a zero
			// integer
			MSG_WriteBits( msg, *toF, field->bits );
		}
 	}
}

void MSG_ReadField (msg_t *msg, int *toF, const netField_t *field, int print)
{
	int			trunc;

	if ( field->bits == -1)
	{	// a -1 in the bits field means it's a float that's always between -1 and 1
		int temp = MSG_ReadBits( msg, -16);
		*(float *)toF = (float)temp / 32767;
	}
	else
  	if ( field->bits == 0 ) {
  		// float
		if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				*(float *)toF = 0.0f; 
		} else {
			if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				// integral float
				trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
				// bias to allow equal parts positive and negative
				trunc -= FLOAT_INT_BIAS;
				*(float *)toF = trunc; 
				if ( print ) {
					Com_Printf( "%s:%i ", field->name, trunc );
				}
			} else {
				// full floating point value
				*toF = MSG_ReadBits( msg, 32 );
				if ( print ) {
					Com_Printf( "%s:%f ", field->name, *(float *)toF );
				}
			}
		}
	} else {
		if ( MSG_ReadBits( msg, 1 ) == 0 ) {
			*toF = 0;
		} else {
			// integer
			*toF = MSG_ReadBits( msg, field->bits );
			if ( print ) {
				Com_Printf( "%s:%i ", field->name, *toF );
			}
		}
	}
}


/*
==================
MSG_WriteDeltaEntity


GENTITYNUM_BITS 1 : remove this entity
GENTITYNUM_BITS 0 1 SMALL_VECTOR_BITS <data>
GENTITYNUM_BITS 0 0 LARGE_VECTOR_BITS >data>

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
#if 0 // Removed by BTO (VV)
void MSG_WriteDeltaEntity( msg_t *msg, struct entityState_s *from, struct entityState_s *to, 
						   qboolean force ) {
	int			c;
	int			i;
	const netField_t	*field;
	int			*fromF, *toF;
	int			blah;
	bool		stuffChanged = false;
	const int numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);
	byte		changeVector[(numFields/8) + 1];


	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the entityState_t
	// struct without updating the message fields
	blah = sizeof( *from );
	assert( numFields + 1 == blah/4); 

	c = msg->cursize;

	// a NULL to is a delta remove message
	if ( to == NULL ) {
		if ( from == NULL ) {
			return;
		}
		MSG_WriteBits( msg, from->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 1, 1 );
		return;
	}

	if ( to->number < 0 || to->number >= MAX_GENTITIES ) {
		Com_Error (ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number );
	}

	memset(changeVector, 0, sizeof(changeVector));

	// build the change vector as bytes so it is endien independent
	for ( i = 0, field = entityStateFields ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		if ( *fromF != *toF ) {
			changeVector[ i>>3 ] |= 1 << ( i & 7 );
			stuffChanged = true;
		}
	}
	
	if ( stuffChanged ) 
	{
		MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 0, 1 );			// not removed
		MSG_WriteBits( msg, 1, 1 );			// we have a delta

		// we need to write the entire delta
		for ( i = 0 ; i + 8 <= numFields ; i += 8 ) {
			MSG_WriteByte( msg, changeVector[i>>3] );
		}
		if ( numFields & 7 ) {
			MSG_WriteBits( msg, changeVector[i>>3], numFields & 7 );
		}

		for ( i = 0, field = entityStateFields ; i < numFields ; i++, field++ ) {
			fromF = (int *)( (byte *)from + field->offset );
			toF = (int *)( (byte *)to + field->offset );

			if ( *fromF == *toF ) {
				continue;
			}

			MSG_WriteField(msg, toF, field);
		}
	}
	else
	{
		// nothing at all changed
		// write two bits for no change
		MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 0, 1 );		// not removed
		MSG_WriteBits( msg, 0, 1 );		// no delta
	}

	c = msg->cursize - c;
}
#endif


extern serverStatic_t svs;
void MSG_WriteEntity( msg_t *msg, struct entityState_s *to, int removeNum)
{

	if ( to == NULL ) {
		MSG_WriteBits(msg, removeNum, GENTITYNUM_BITS);
		MSG_WriteBits(msg, 1, 1); //removed
		return;
	} else {
		MSG_WriteBits(msg, to->number, GENTITYNUM_BITS);
		MSG_WriteBits(msg, 0, 1); //not removed
	}
	assert(( to - svs.snapshotEntities ) >= 0 && ( to - svs.snapshotEntities ) < 512);
	MSG_WriteLong(msg, to - svs.snapshotEntities);
}

void MSG_ReadEntity( msg_t *msg, entityState_t *to)
{
	// check for a remove
	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		memset( to, 0, sizeof( *to ) );	
		to->number = MAX_GENTITIES - 1;
		return;
	}

	//No remove, read data
	int index;
	index = MSG_ReadLong(msg);
	assert(index >= 0 && index < svs.numSnapshotEntities);
	*to = svs.snapshotEntities[index];
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
==================
*/
extern	cvar_t	*cl_shownet;

#if 0 // Removed by BTO (VV)
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, int number) 
{
	int			i;
	const netField_t	*field;
	int			*fromF, *toF;
	int			print = 0;
	int			startBit, endBit;
	const int numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);
	byte		expandedVector[(numFields/8) + 1];

	if ( number < 0 || number >= MAX_GENTITIES) {
		Com_Error( ERR_DROP, "Bad delta entity number: %i", number );
	}

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// check for a remove
	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		memset( to, 0, sizeof( *to ) );	
		to->number = MAX_GENTITIES - 1;
		if ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) {
			Com_Printf( "%3i: #%-3i remove\n", msg->readcount, number );
		}
		return;
	}

	// check for no delta
	if ( MSG_ReadBits( msg, 1 ) != 0 )
	{  
		const int numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);

		// shownet 2/3 will interleave with other printed info, -1 will
		// just print the delta records`
		if ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) {
			print = 1;
			Com_Printf( "%3i: #%-3i ", msg->readcount, to->number );
		} else {
			print = 0;
		}

		// we need to write the entire delta
		for ( i = 0 ; i + 8 <= numFields ; i += 8 ) {
			expandedVector[i>>3] = MSG_ReadByte( msg );
		}
		if ( numFields & 7 ) {
			expandedVector[i>>3] = MSG_ReadBits( msg, numFields & 7 );
		}

		to->number = number;

		for ( i = 0, field = entityStateFields ; i < numFields ; i++, field++ ) {
			fromF = (int *)( (byte *)from + field->offset );
			toF = (int *)( (byte *)to + field->offset );

			if ( ! ( expandedVector[ i >> 3 ] & ( 1 << ( i & 7 ) ) ) ) {
				// no change
				*toF = *fromF;
			} else {
				MSG_ReadField(msg, toF, field, print);
			}
		}
	}
	else
	{
		memcpy(to, from,sizeof(entityState_t));
		to->number = number;
 	}
	if ( print ) {
		if ( msg->bit == 0 ) {
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		} else {
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}
		Com_Printf( " (%i bits)\n", endBit - startBit  );
	}
}
#endif

/*
Ghoul2 Insert End
*/

/*
============================================================================

plyer_state_t communication

============================================================================
*/

// using the stringizing operator to save typing...
#define	PSF(x) #x,offsetof(playerState_t, x)

static const netField_t	playerStateFields[] = 
{
{ PSF(commandTime), 32 },
{ PSF(pm_type), 8 },
{ PSF(bobCycle), 8 },
{ PSF(pm_flags), 32 },
{ PSF(pm_time), -16 },
{ PSF(origin[0]), 0 },
{ PSF(origin[1]), 0 },
{ PSF(origin[2]), 0 },
{ PSF(velocity[0]), 0 },
{ PSF(velocity[1]), 0 },
{ PSF(velocity[2]), 0 },
{ PSF(weaponTime), -16 },
{ PSF(weaponChargeTime), 32 }, //? really need 32 bits??
{ PSF(gravity), 16 },
{ PSF(leanofs), -8 },
{ PSF(friction), 16 },
{ PSF(speed), 16 },
{ PSF(delta_angles[0]), 16 },
{ PSF(delta_angles[1]), 16 },
{ PSF(delta_angles[2]), 16 },
{ PSF(groundEntityNum), GENTITYNUM_BITS },
//{ PSF(animationTimer), 16 },
{ PSF(legsAnim), 16 },
{ PSF(torsoAnim), 16 },
{ PSF(movementDir), 4 },
{ PSF(eFlags), 32 },
{ PSF(eventSequence), 16 },
{ PSF(events[0]), 8 },
{ PSF(events[1]), 8 },
{ PSF(eventParms[0]), -9 },
{ PSF(eventParms[1]), -9 },
{ PSF(externalEvent), 8 },
{ PSF(externalEventParm), 8 },
{ PSF(clientNum), 32 },
{ PSF(weapon), 5 },
{ PSF(weaponstate),	  4 },
{ PSF(batteryCharge),	16 },
{ PSF(viewangles[0]), 0 },
{ PSF(viewangles[1]), 0 },
{ PSF(viewangles[2]), 0 },
{ PSF(viewheight), -8 },
{ PSF(damageEvent), 8 },
{ PSF(damageYaw), 8 },
{ PSF(damagePitch), -8 },
{ PSF(damageCount), 8 },
#ifdef JK2_MODE
{ PSF(saberColor), 8 },
{ PSF(saberActive), 8 },
{ PSF(saberLength), 32 },
{ PSF(saberLengthMax), 32 },
#endif
{ PSF(forcePowersActive), 32},
{ PSF(saberInFlight), 8 },
#ifdef JK2_MODE
{ PSF(vehicleModel), 32 },
#endif

/*{ PSF(vehicleIndex), 32 },			// WOAH, what do we do with this stuff???
{ PSF(vehicleArmor), 32 },
{ PSF(vehicleAngles[0]), 0 },
{ PSF(vehicleAngles[1]), 0 },
{ PSF(vehicleAngles[2]), 0 },*/

{ PSF(viewEntity), 32 },
{ PSF(serverViewOrg[0]), 0 },
{ PSF(serverViewOrg[1]), 0 },
{ PSF(serverViewOrg[2]), 0 },
{ PSF(forceRageRecoveryTime), 32 },
};

/*
=============
MSG_WriteDeltaPlayerstate

=============
*/
void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to ) {
	int				i;
	playerState_t	dummy;
	int				statsbits;
	int				persistantbits;
	int				ammobits;
	int				powerupbits;
	int				numFields;
	int				c;
	const netField_t	*field;
	int				*fromF, *toF;

	if (!from) {
		from = &dummy;
		memset (&dummy, 0, sizeof(dummy));
	}

	c = msg->cursize;

	numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );
	for ( i = 0, field = playerStateFields ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( *fromF == *toF ) {
			MSG_WriteBits( msg, 0, 1 );	// no change
			continue;
		}

		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteField (msg, toF, field);
	}
	c = msg->cursize - c;


	//
	// send the arrays
	//
	statsbits = 0;
	for (i=0 ; i<MAX_STATS ; i++) {
		if (to->stats[i] != from->stats[i]) {
			statsbits |= 1<<i;
		}
	}
	if ( statsbits ) {
		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteShort( msg, statsbits );
		for (i=0 ; i<MAX_STATS ; i++)
			if (statsbits & (1<<i) )
				MSG_WriteBits(msg, to->stats[i], 32);
	} else {
		MSG_WriteBits( msg, 0, 1 );	// no change
	}


	persistantbits = 0;
	for (i=0 ; i<MAX_PERSISTANT ; i++) {
		if (to->persistant[i] != from->persistant[i]) {
			persistantbits |= 1<<i;
		}
	}
	if ( persistantbits ) {
		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteShort( msg, persistantbits );
		for (i=0 ; i<MAX_PERSISTANT ; i++)
			if (persistantbits & (1<<i) )
				MSG_WriteSShort (msg, to->persistant[i]);
	} else {
		MSG_WriteBits( msg, 0, 1 );	// no change
	}


	ammobits = 0;
	for (i=0 ; i<MAX_AMMO ; i++) {
		if (to->ammo[i] != from->ammo[i]) {
			ammobits |= 1<<i;
		}
	}
	if ( ammobits ) {
		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteShort( msg, ammobits );
		for (i=0 ; i<MAX_AMMO ; i++)
			if (ammobits & (1<<i) )
				MSG_WriteSShort (msg, to->ammo[i]);
	} else {
		MSG_WriteBits( msg, 0, 1 );	// no change
	}

	powerupbits = 0;
	for (i=0 ; i<MAX_POWERUPS ; i++) {
		if (to->powerups[i] != from->powerups[i]) {
			powerupbits |= 1<<i;
		}
	}
	if ( powerupbits ) {
		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteShort( msg, powerupbits );
		for (i=0 ; i<MAX_POWERUPS ; i++)
			if (powerupbits & (1<<i) )
				MSG_WriteLong( msg, to->powerups[i] );
	} else {
		MSG_WriteBits( msg, 0, 1 );	// no change
	}


	statsbits = 0;
	for (i=0 ; i<MAX_INVENTORY ; i++) 
	{
		if (to->inventory[i] != from->inventory[i]) 
		{
			statsbits |= 1<<i;
		}
	}
	if ( statsbits ) 
	{
		MSG_WriteBits( msg, 1, 1 );	// changed
		MSG_WriteShort( msg, statsbits );
		for (i=0 ; i<MAX_INVENTORY ; i++)
		{
			if (statsbits & (1<<i) )
			{
				MSG_WriteShort (msg, to->inventory[i]);
			}
		}
	} 
	else 
	{
		MSG_WriteBits( msg, 0, 1 );	// no change
	}
}


/*
===================
MSG_ReadDeltaPlayerstate
===================
*/
void MSG_ReadDeltaPlayerstate (msg_t *msg, playerState_t *from, playerState_t *to ) {
	int			i;
	int			bits;
	const netField_t	*field;
	int			numFields;
	int			startBit, endBit;
	int			print;
	int			*fromF, *toF;
	playerState_t	dummy;

	if ( !from ) {
		from = &dummy;
		memset( &dummy, 0, sizeof( dummy ) );
	}
	*to = *from;

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) {
		print = 1;
		Com_Printf( "%3i: playerstate ", msg->readcount );
	} else {
		print = 0;
	}

	numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );
	for ( i = 0, field = playerStateFields ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( ! MSG_ReadBits( msg, 1 ) ) {
			// no change
			*toF = *fromF;
		} else {
			MSG_ReadField( msg, toF, field, print);
		}
	}

	// read the arrays

	// parse stats
	if ( MSG_ReadBits( msg, 1 ) ) {
		LOG("PS_STATS");
		bits = MSG_ReadShort (msg);
		for (i=0 ; i<MAX_STATS ; i++) {
			if (bits & (1<<i) ) {
				to->stats[i] = MSG_ReadBits(msg,32);
			}
		}
	}

	// parse persistant stats
	if ( MSG_ReadBits( msg, 1 ) ) {
		LOG("PS_PERSISTANT");
		bits = MSG_ReadShort (msg);
		for (i=0 ; i<MAX_PERSISTANT ; i++) {
			if (bits & (1<<i) ) {
				to->persistant[i] = MSG_ReadSShort(msg);
			}
		}
	}

	// parse ammo
	if ( MSG_ReadBits( msg, 1 ) ) {
		LOG("PS_AMMO");
		bits = MSG_ReadShort (msg);
		for (i=0 ; i<MAX_AMMO ; i++) {
			if (bits & (1<<i) ) {
				to->ammo[i] = MSG_ReadSShort(msg);
			}
		}
	}

	// parse powerups
	if ( MSG_ReadBits( msg, 1 ) ) {
		LOG("PS_POWERUPS");
		bits = MSG_ReadShort (msg);
		for (i=0 ; i<MAX_POWERUPS ; i++) {
			if (bits & (1<<i) ) {
				to->powerups[i] = MSG_ReadLong(msg);
			}
		}
	}

	// parse inventory
	if ( MSG_ReadBits( msg, 1 ) ) {
		LOG("PS_INVENTORY");
		bits = MSG_ReadShort (msg);
		for (i=0 ; i<MAX_INVENTORY ; i++) {
			if (bits & (1<<i) ) {
				to->inventory[i] = MSG_ReadShort(msg);
			}
		}
	}

	if ( print ) {
		if ( msg->bit == 0 ) {
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		} else {
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}
		Com_Printf( " (%i bits)\n", endBit - startBit  );
	}
}


//===========================================================================
