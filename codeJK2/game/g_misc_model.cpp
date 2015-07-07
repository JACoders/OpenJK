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

#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "bg_public.h"

extern cvar_t *g_spskill;

//
// Helper functions
//
//------------------------------------------------------------
void SetMiscModelModels( char *modelNameString, gentity_t *ent, qboolean damage_model )
{
	char	damageModel[MAX_QPATH];
	char	chunkModel[MAX_QPATH];
	int		len;

	//Main model
	ent->s.modelindex = G_ModelIndex( modelNameString );

	if ( damage_model )
	{
		len = strlen( modelNameString ) - 4; // extract the extension

		//Dead/damaged model
		strncpy( damageModel, modelNameString, len );
		damageModel[len] = 0;
		strncpy( chunkModel, damageModel, sizeof(chunkModel));
		strcat( damageModel, "_d1.md3" );
		ent->s.modelindex2 = G_ModelIndex( damageModel );

		ent->spawnflags |= 4; // deadsolid

		//Chunk model
		strcat( chunkModel, "_c1.md3" );
		ent->s.modelindex3 = G_ModelIndex( chunkModel );
	}
}

//------------------------------------------------------------
void SetMiscModelDefaults( gentity_t *ent, useFunc_t use_func, char *material, int solid_mask,int animFlag,
									qboolean take_damage, qboolean damage_model = qfalse )
{
	// Apply damage and chunk models if they exist
	SetMiscModelModels( ent->model, ent, damage_model );

	ent->s.eFlags = animFlag;
	ent->svFlags |= SVF_PLAYER_USABLE;
	ent->contents = solid_mask;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	gi.linkentity (ent);

	// Set a generic use function

	ent->e_UseFunc = use_func;
/*	if (use_func == useF_health_use)
	{
		G_SoundIndex("sound/player/suithealth.wav");
	}
	else if (use_func == useF_ammo_use )
	{
		G_SoundIndex("sound/player/suitenergy.wav");
	}
*/
	G_SpawnInt( "material", material, (int *)&ent->material );

	if (ent->health)
	{
		ent->max_health = ent->health;
		ent->takedamage = take_damage;
		ent->e_PainFunc = painF_misc_model_breakable_pain;
		ent->e_DieFunc  = dieF_misc_model_breakable_die;
	}
}

void HealthStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 100;
			break;
		case 1:	//	MEDIUM
			ent->count = 75;
			break;
		default :
		case 2:	//	HARD
			ent->count = 50;
			break;
		}
	}
}


void CrystalAmmoSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 75;
			break;
		case 1:	//	MEDIUM
			ent->count = 75;
			break;
		default :
		case 2:	//	HARD
			ent->count = 75;
			break;
		}
	}
}


//------------------------------------------------------------

//------------------------------------------------------------
/*QUAKED misc_model_ghoul (1 0 0) (-16 -16 -37) (16 16 32)
"model"		arbitrary .glm file to display
"health" - how much health the model has - default 60 (zero makes non-breakable)
*/
//------------------------------------------------------------
#include "anims.h"
extern qboolean G_ParseAnimFileSet( const char *filename, const char *animCFG, int *animFileIndex );
int temp_animFileIndex;
void set_MiscAnim( gentity_t *ent)
{
	animation_t *animations = level.knownAnimFileSets[temp_animFileIndex].animations;
	if (ent->playerModel & 1)
	{
		int anim = BOTH_STAND3;
		float animSpeed = 50.0f / animations[anim].frameLerp;

		// yes, its the same animation, so work out where we are in the leg anim, and blend us
		gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
							(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
							BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND , animSpeed, (cg.time?cg.time:level.time), -1, 350);
	}
	else
	{
		int anim = BOTH_PAIN3;
		float animSpeed = 50.0f / animations[anim].frameLerp;
		gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
						(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
						BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND, animSpeed, (cg.time?cg.time:level.time), -1, 350);
	}
	ent->nextthink = level.time + 900;
	ent->playerModel++;

}

void SP_misc_model_ghoul( gentity_t *ent )
{
#if 1
	ent->s.modelindex = G_ModelIndex( ent->model );
	gi.G2API_InitGhoul2Model(ent->ghoul2, ent->model, ent->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0);
	ent->s.radius = 50;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	qboolean bHasScale = G_SpawnVector( "modelscale_vec", "1 1 1", ent->s.modelScale );
	if ( !bHasScale ) {
		float temp;
		G_SpawnFloat( "modelscale", "0", &temp );
		if ( temp != 0.0f ) {
			ent->s.modelScale[0] = ent->s.modelScale[1] = ent->s.modelScale[2] = temp;
			bHasScale = qtrue;
		}
	}
	if ( bHasScale ) {
		//scale the x axis of the bbox up.
		ent->maxs[0] *= ent->s.modelScale[0];
		ent->mins[0] *= ent->s.modelScale[0];

		//scale the y axis of the bbox up.
		ent->maxs[1] *= ent->s.modelScale[1];
		ent->mins[1] *= ent->s.modelScale[1];

		//scale the z axis of the bbox up and adjust origin accordingly
		ent->maxs[2] *= ent->s.modelScale[2];
		float oldMins2 = ent->mins[2];
		ent->mins[2] *= ent->s.modelScale[2];
		ent->s.origin[2] += (oldMins2 - ent->mins[2]);
	}

	gi.linkentity (ent);
#else
	char name1[200] = "models/players/kyle/model.glm";
	ent->s.modelindex = G_ModelIndex( name1 );

	gi.G2API_InitGhoul2Model(ent->ghoul2, name1, ent->s.modelindex);
	ent->s.radius = 150;

			// we found the model ok - load it's animation config
 	if ( !G_ParseAnimFileSet( "kyle", "_humanoid", &temp_animFileIndex ) )
 	{
 		Com_Printf( S_COLOR_RED"Failed to load animation file set models/players/jedi/animation.cfg\n");
 	}


	ent->s.angles[0] = 0;
	ent->s.angles[1] = 90;
	ent->s.angles[2] = 0;

	ent->s.origin[2] = 20;
	ent->s.origin[1] = 80;
//	ent->s.modelScale[0] = ent->s.modelScale[1] = ent->s.modelScale[2] = 0.8f;

	VectorSet (ent->mins, -16, -16, -37);
	VectorSet (ent->maxs, 16, 16, 32);
//#if _DEBUG
//loadsavecrash
//	VectorCopy(ent->mins, ent->s.mins);
//	VectorCopy(ent->maxs, ent->s.maxs);
//#endif
	ent->contents = CONTENTS_BODY;
	ent->clipmask = MASK_NPCSOLID;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->health = 1000;

//	ent->s.modelindex = G_ModelIndex( "models/weapons2/blaster_r/g2blaster_w.glm" );
//	gi.G2API_InitGhoul2Model(ent->ghoul2, "models/weapons2/blaster_r/g2blaster_w.glm", ent->s.modelindex);
//	gi.G2API_AddBolt(&ent->ghoul2[0], "*weapon");
//	gi.G2API_AttachG2Model(&ent->ghoul2[1],&ent->ghoul2[0], 0, 0);

	gi.linkentity (ent);

	animation_t *animations = level.knownAnimFileSets[temp_animFileIndex].animations;
	int anim = BOTH_STAND3;
	float animSpeed = 50.0f / animations[anim].frameLerp;
	gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame,
					(animations[anim].numFrames -1 )+ animations[anim].firstFrame,
					BONE_ANIM_OVERRIDE_FREEZE , animSpeed, cg.time);

//	int test = gi.G2API_GetSurfaceRenderStatus(&ent->ghoul2[0], "l_hand");
//	gi.G2API_SetSurfaceOnOff(&ent->ghoul2[0], "l_arm",0x00000100);
//	test = gi.G2API_GetSurfaceRenderStatus(&ent->ghoul2[0], "l_hand");

//	gi.G2API_SetNewOrigin(&ent->ghoul2[0], gi.G2API_AddBolt(&ent->ghoul2[0], "rhang_tag_bone"));
//	ent->s.apos.trDelta[1] = 10;
//	ent->s.apos.trType = TR_LINEAR;


	ent->nextthink = level.time + 1000;
	ent->e_ThinkFunc = thinkF_set_MiscAnim;
#endif
}


#define RACK_BLASTER	1
#define RACK_REPEATER	2
#define RACK_ROCKET		4

/*QUAKED misc_model_gun_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER REPEATER ROCKET
#MODELNAME="models/map_objects/kejim/weaponsrack.md3"

NOTE: can mix and match these spawnflags to get multi-weapon racks.  If only one type is checked the rack will be full of those weapons
BLASTER - Puts one or more blaster guns on the rack.
REPEATER - Puts one or more repeater guns on the rack.
ROCKET - Puts one or more rocket launchers on the rack.
*/

void GunRackAddItem( gitem_t *gun, vec3_t org, vec3_t angs, float ffwd, float fright, float fup )
{
	vec3_t		fwd, right;
	gentity_t	*it_ent = G_Spawn();
	qboolean	rotate = qtrue;

	AngleVectors( angs, fwd, right, NULL );

	if ( it_ent && gun )
	{
		// FIXME: scaling the ammo will probably need to be tweaked to a reasonable amount...adjust as needed
		// Set base ammo per type
		if ( gun->giType == IT_WEAPON )
		{
			it_ent->spawnflags |= 16;// VERTICAL

			switch( gun->giTag )
			{
			case WP_BLASTER:
				it_ent->count = 15;
				break;
			case WP_REPEATER:
				it_ent->count = 100;
				break;
			case WP_ROCKET_LAUNCHER:
				it_ent->count = 4;
				break;
			}
		}
		else
		{
			rotate = qfalse;

			// must deliberately make it small, or else the objects will spawn inside of each other.
			VectorSet( it_ent->maxs, 6.75f, 6.75f, 6.75f );
			VectorScale( it_ent->maxs, -1, it_ent->mins );
		}

		it_ent->spawnflags |= 1;// ITMSF_SUSPEND
		it_ent->classname = gun->classname;
		G_SpawnItem( it_ent, gun );

		// FinishSpawningItem handles everything, so clear the thinkFunc that was set in G_SpawnItem
		FinishSpawningItem( it_ent );

		if ( gun->giType == IT_AMMO )
		{
			if ( gun->giTag == AMMO_BLASTER ) // I guess this just has to use different logic??
			{
				if ( g_spskill->integer >= 2 )
				{
					it_ent->count += 10; // give more on higher difficulty because there will be more/harder enemies?
				}
			}
			else
			{
				// scale ammo based on skill
				switch ( g_spskill->integer )
				{
				case 0: // do default
					break;
				case 1:
					it_ent->count *= 0.75f;
					break;
				case 2:
					it_ent->count *= 0.5f;
					break;
				}
			}
		}

		it_ent->nextthink = 0;

		VectorCopy( org, it_ent->s.origin );
		VectorMA( it_ent->s.origin, fright, right, it_ent->s.origin );
		VectorMA( it_ent->s.origin, ffwd, fwd, it_ent->s.origin );
		it_ent->s.origin[2] += fup;

		VectorCopy( angs, it_ent->s.angles );

		// by doing this, we can force the amount of ammo we desire onto the weapon for when it gets picked-up
		it_ent->flags |= ( FL_DROPPED_ITEM | FL_FORCE_PULLABLE_ONLY );
		it_ent->physicsBounce = 0.1f;

		for ( int t = 0; t < 3; t++ )
		{
			if ( rotate )
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 180 + crandom() * 14 );
				}
				else
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + crandom() * 4 );
				}
			}
			else
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 90 + crandom() * 4 );
				}
			}
		}

		G_SetAngles( it_ent, it_ent->s.angles );
		G_SetOrigin( it_ent, it_ent->s.origin );
		gi.linkentity( it_ent );
	}
}

//---------------------------------------------
void SP_misc_model_gun_rack( gentity_t *ent )
{
	gitem_t		*blaster = NULL, *repeater = NULL, *rocket = NULL;
	int			ct = 0;
	float		ofz[3];
	gitem_t		*itemList[3];

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_REPEATER | RACK_ROCKET )))
	{
		blaster	= FindItemForWeapon( WP_BLASTER );
	}

	if (( ent->spawnflags & RACK_REPEATER ))
	{
		repeater = FindItemForWeapon( WP_REPEATER );
	}

	if (( ent->spawnflags & RACK_ROCKET ))
	{
		rocket = FindItemForWeapon( WP_ROCKET_LAUNCHER );
	}

	//---------weapon types
	if ( blaster )
	{
		ofz[ct] = 23.0f;
		itemList[ct++] = blaster;
	}

	if ( repeater )
	{
		ofz[ct] = 24.5f;
		itemList[ct++] = repeater;
	}

	if ( rocket )
	{
		ofz[ct] = 25.5f;
		itemList[ct++] = rocket;
	}

	if ( ct ) //..should always have at least one item on their, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			ofz[ct] = ofz[0];
			itemList[ct] = itemList[0]; // first weapon ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for ( int i = 0; i < ct; i++ )
		{
			GunRackAddItem( itemList[i], ent->s.origin, ent->s.angles, crandom() * 2, ( i - 1 ) * 9 + crandom() * 2, ofz[i] );
		}
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrack.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->contents = CONTENTS_SOLID;

	gi.linkentity( ent );
}

#define RACK_METAL_BOLTS	2
#define RACK_ROCKETS		4
#define RACK_WEAPONS		8
#define RACK_HEALTH			16
#define RACK_PWR_CELL		32
#define RACK_NO_FILL		64

/*QUAKED misc_model_ammo_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER METAL_BOLTS ROCKETS WEAPON HEALTH PWR_CELL NO_FILL
#MODELNAME="models/map_objects/kejim/weaponsrung.md3"

NOTE: can mix and match these spawnflags to get multi-ammo racks.  If only one type is checked the rack will be full of that ammo.  Only three ammo packs max can be displayed.


BLASTER - Adds one or more ammo packs that are compatible with Blasters and the Bryar pistol.
METAL_BOLTS - Adds one or more metal bolt ammo packs that are compatible with the heavy repeater and the flechette gun
ROCKETS - Puts one or more rocket packs on a rack.
WEAPON - adds a weapon matching a selected ammo type to the rack.
HEALTH - adds a health pack to the top shelf of the ammo rack
PWR_CELL - Adds one or more power cell packs that are compatible with the Disuptor, bowcaster, and demp2
NO_FILL - Only puts selected ammo on the rack, it never fills up all three slots if only one or two items were checked
*/

extern gitem_t	*FindItemForAmmo( ammo_t ammo );

//---------------------------------------------
void SP_misc_model_ammo_rack( gentity_t *ent )
{
// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_BLASTER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_BLASTER ));
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_REPEATER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_METAL_BOLTS ));
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( FindItemForWeapon( WP_ROCKET_LAUNCHER ));
		}
		RegisterItem( FindItemForAmmo( AMMO_ROCKETS ));
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		RegisterItem( FindItemForAmmo( AMMO_POWERCELL ));
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		RegisterItem( FindItem( "item_medpak_instant" ));
	}

	ent->e_ThinkFunc = thinkF_spawn_rack_goods;
	ent->nextthink = level.time + 100;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->contents = CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;//so use traces can go through them

	gi.linkentity( ent );
}

// AMMO RACK!!
void spawn_rack_goods( gentity_t *ent )
{
	float		v_off = 0;
	gitem_t		*blaster = NULL, *metal_bolts = NULL, *rockets = NULL, *it = NULL;
	gitem_t		*am_blaster = NULL, *am_metal_bolts = NULL, *am_rockets = NULL, *am_pwr_cell = NULL;
	gitem_t		*health = NULL;
	int			pos = 0, ct = 0;
	gitem_t		*itemList[4]; // allocating 4, but we only use 3.  done so I don't have to validate that the array isn't full before I add another

	gi.unlinkentity( ent );

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			blaster	= FindItemForWeapon( WP_BLASTER );
		}
		am_blaster	= FindItemForAmmo( AMMO_BLASTER );
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			metal_bolts = FindItemForWeapon( WP_REPEATER );
		}
		am_metal_bolts = FindItemForAmmo( AMMO_METAL_BOLTS );
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			rockets = FindItemForWeapon( WP_ROCKET_LAUNCHER );
		}
		am_rockets = FindItemForAmmo( AMMO_ROCKETS );
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		am_pwr_cell = FindItemForAmmo( AMMO_POWERCELL );
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		health = FindItem( "item_medpak_instant" );
		RegisterItem( health );
	}

	//---------Ammo types
	if ( am_blaster )
	{
		itemList[ct++] = am_blaster;
	}

	if ( am_metal_bolts )
	{
		itemList[ct++] = am_metal_bolts;
	}

	if ( am_pwr_cell )
	{
		itemList[ct++] = am_pwr_cell;
	}

	if ( am_rockets )
	{
		itemList[ct++] = am_rockets;
	}

	if ( !(ent->spawnflags & RACK_NO_FILL) && ct ) //double negative..should always have at least one item on there, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			itemList[ct] = itemList[0]; // first item ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for ( int i = 0; i < ct; i++ )
		{
			GunRackAddItem( itemList[i], ent->s.origin, ent->s.angles, crandom() * 0.5f, (i-1)* 8, 7.0f );
		}
	}

	// -----Weapon option
	if ( ent->spawnflags & RACK_WEAPONS )
	{
		if ( !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
		{
			// nothing was selected, so we assume blaster pack
			it = blaster;
		}
		else
		{
			// if weapon is checked...and so are one or more ammo types, then pick a random weapon to display..always give weaker weapons first
			if ( blaster )
			{
				it = blaster;
				v_off = 25.5f;
			}
			else if ( metal_bolts )
			{
				it = metal_bolts;
				v_off = 27.0f;
			}
			else if ( rockets )
			{
				it = rockets;
				v_off = 28.0f;
			}
		}

		if ( it )
		{
			// since we may have to put up a health pack on the shelf, we should know where we randomly put
			//	the gun so we don't put the pack on the same spot..so pick either the left or right side
			pos = ( random() > .5 ) ? -1 : 1;

			GunRackAddItem( it, ent->s.origin, ent->s.angles, crandom() * 2, ( random() * 6 + 4 ) * pos, v_off );
		}
	}

	// ------Medpack
	if (( ent->spawnflags & RACK_HEALTH ) && health )
	{
		if ( !pos )
		{
			// we haven't picked a side already...
			pos = ( random() > .5 ) ? -1 : 1;
		}
		else
		{
			// switch to the opposite side
			pos *= -1;
		}

		GunRackAddItem( health, ent->s.origin, ent->s.angles, crandom() * 0.5f, ( random() * 4 + 4 ) * pos, 24 );
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrung.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	gi.linkentity( ent );
}

#define DROP_MEDPACK	1
#define DROP_SHIELDS	2
#define DROP_BACTA		4
#define DROP_BATTERIES	8

/*QUAKED misc_model_cargo_small (1 0 0.25) (-14 -14 -4) (14 14 30) MEDPACK SHIELDS BACTA BATTERIES
#MODELNAME="models/map_objects/kejim/cargo_small.md3"

  Cargo crate that can only be destroyed by heavy class weapons ( turrets, emplaced guns, at-st )  Can spawn useful things when it breaks

MEDPACK - instant use medpacks
SHIELDS - instant shields
BACTA - bacta tanks
BATTERIES -

"health" - how much damage to take before blowing up ( default 25 )
"splashRadius" - damage range when it explodes ( default 96 )
"splashDamage" - damage to do within explode range ( default 1 )

*/
extern gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity, char *target );

void misc_model_cargo_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	int		flags;
	vec3_t	org, temp;
	gitem_t *health = NULL, *shields = NULL, *bacta = NULL, *batteries = NULL;

	// copy these for later
	flags = self->spawnflags;
	VectorCopy( self->currentOrigin, org );

	// we already had spawn flags, but we don't care what they were...we just need to set up the flags we want for misc_model_breakable_die
	self->spawnflags = 8; // NO_DMODEL

	// pass through to get the effects and such
	misc_model_breakable_die( self, inflictor, attacker, damage, mod );

	// now that the model is broken, we can safely spawn these in it's place without them being in solid
	temp[2] = org[2] + 16;

	// annoying, but spawn each thing in its own little quadrant so that they don't end up on top of each other
	if (( flags & DROP_MEDPACK ))
	{
		health = FindItem( "item_medpak_instant" );

		if ( health )
		{
			temp[0] = org[0] + crandom() * 8 + 16;
			temp[1] = org[1] + crandom() * 8 + 16;

			LaunchItem( health, temp, (float *)vec3_origin, NULL );
		}
	}
	if (( flags & DROP_SHIELDS ))
	{
		shields = FindItem( "item_shield_sm_instant" );

		if ( shields )
		{
			temp[0] = org[0] + crandom() * 8 - 16;
			temp[1] = org[1] + crandom() * 8 + 16;

			LaunchItem( shields, temp, (float *)vec3_origin, NULL );
		}
	}

	if (( flags & DROP_BACTA ))
	{
		bacta = FindItem( "item_bacta" );

		if ( bacta )
		{
			temp[0] = org[0] + crandom() * 8 - 16;
			temp[1] = org[1] + crandom() * 8 - 16;

			LaunchItem( bacta, temp, (float *)vec3_origin, NULL );
		}
	}

	if (( flags & DROP_BATTERIES ))
	{
		batteries = FindItem( "item_battery" );

		if ( batteries )
		{
			temp[0] = org[0] + crandom() * 8 + 16;
			temp[1] = org[1] + crandom() * 8 - 16;

			LaunchItem( batteries, temp, (float *)vec3_origin, NULL );
		}
	}
}

//---------------------------------------------
void SP_misc_model_cargo_small( gentity_t *ent )
{
	G_SpawnInt( "splashRadius", "96", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "1", &ent->splashDamage );

	if (( ent->spawnflags & DROP_MEDPACK ))
	{
		RegisterItem( FindItem( "item_medpak_instant" ));
	}

	if (( ent->spawnflags & DROP_SHIELDS ))
	{
		RegisterItem( FindItem( "item_shield_sm_instant" ));
	}

	if (( ent->spawnflags & DROP_BACTA ))
	{
		RegisterItem( FindItem( "item_bacta" ));
	}

	if (( ent->spawnflags & DROP_BATTERIES ))
	{
		RegisterItem( FindItem( "item_battery" ));
	}

	G_SpawnInt( "health", "25", &ent->health );

	SetMiscModelDefaults( ent, useF_NULL, "11", CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, 0, qtrue, qfalse );
	ent->s.modelindex2 = G_ModelIndex("/models/map_objects/kejim/cargo_small.md3");	// Precache model

	// we only take damage from a heavy weapon class missile
	ent->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;

	ent->e_DieFunc = dieF_misc_model_cargo_die;

	ent->radius = 1.5f; // scale number of chunks spawned
}

