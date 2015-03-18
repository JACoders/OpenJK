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

#pragma once

#define BONE_ANGLES_PREMULT			0x0001
#define BONE_ANGLES_POSTMULT		0x0002
#define BONE_ANGLES_REPLACE			0x0004

//added for a trace optimization. set in routines where a bone is
//set to be transformed in any way. -rww
#define	BONE_NEED_TRANSFORM			0x8000

//rww - RAGDOLL_BEGIN
#define BONE_ANGLES_RAGDOLL			0x2000  // the rag flags give more details
//rww - RAGDOLL_END
#define BONE_ANGLES_IK				0x4000  // the rag flags give more details

#define BONE_ANGLES_TOTAL			( BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT | BONE_ANGLES_REPLACE )
#define BONE_ANIM_OVERRIDE			0x0008
#define BONE_ANIM_OVERRIDE_LOOP		0x0010
#define BONE_ANIM_OVERRIDE_FREEZE	( 0x0040 + BONE_ANIM_OVERRIDE )
#define BONE_ANIM_BLEND				0x0080
#define BONE_ANIM_TOTAL				( BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND)


// defines to setup the
#define		ENTITY_WIDTH 12
#define		MODEL_WIDTH	10
#define		BOLT_WIDTH	10

#define		MODEL_AND	((1<<MODEL_WIDTH)-1)
#define		BOLT_AND	((1<<BOLT_WIDTH)-1)
#define		ENTITY_AND	((1<<ENTITY_WIDTH)-1)

#define		BOLT_SHIFT	0
#define		MODEL_SHIFT	(BOLT_SHIFT + BOLT_WIDTH)
#define		ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)
