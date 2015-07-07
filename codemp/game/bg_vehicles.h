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

#include "qcommon/q_shared.h"

typedef struct Vehicle_s Vehicle_t;
typedef struct bgEntity_s bgEntity_t;

typedef enum
{
	VH_NONE = 0,	//0 just in case anyone confuses VH_NONE and VEHICLE_NONE below
	VH_WALKER,		//something you ride inside of, it walks like you, like an AT-ST
	VH_FIGHTER,		//something you fly inside of, like an X-Wing or TIE fighter
	VH_SPEEDER,		//something you ride on that hovers, like a speeder or swoop
	VH_ANIMAL,		//animal you ride on top of that walks, like a tauntaun
	VH_FLIER,		//animal you ride on top of that flies, like a giant mynoc?
	VH_NUM_VEHICLES
} vehicleType_t;

typedef enum
{
	WPOSE_NONE	= 0,
	WPOSE_BLASTER,
	WPOSE_SABERLEFT,
	WPOSE_SABERRIGHT,
} EWeaponPose;

extern stringID_table_t VehicleTable[VH_NUM_VEHICLES+1];

//===========================================================================================================
//START VEHICLE WEAPONS
//===========================================================================================================
typedef struct vehWeaponInfo_s {
//*** IMPORTANT!!! *** vWeapFields table correponds to this structure!
	char	*name;
	qboolean	bIsProjectile;	//traceline or entity?
	qboolean	bHasGravity;	//if a projectile, drops
	qboolean	bIonWeapon;//disables ship shields and sends them out of control
	qboolean	bSaberBlockable;//lightsabers can deflect this projectile
	int		iMuzzleFX;	//index of Muzzle Effect
	int		iModel;		//handle to the model used by this projectile
	int		iShotFX;	//index of Shot Effect
	int		iImpactFX;	//index of Impact Effect
	int		iG2MarkShaderHandle;	//index of shader to use for G2 marks made on other models when hit by this projectile
	float	fG2MarkSize;//size (diameter) of the ghoul2 mark
	int		iLoopSound;	//index of loopSound
	float	fSpeed;		//speed of projectile/range of traceline
	float	fHoming;		//0.0 = not homing, 0.5 = half vel to targ, half cur vel, 1.0 = all vel to targ
	float	fHomingFOV;		//missile will lose lock on if DotProduct of missile direction and direction to target ever drops below this (-1 to 1, -1 = never lose target, 0 = lose if ship gets behind missile, 1 = pretty much will lose it's target right away)
	int		iLockOnTime;	//0 = no lock time needed, else # of ms needed to lock on
	int		iDamage;		//damage done when traceline or projectile directly hits target
	int		iSplashDamage;//damage done to ents in splashRadius of end of traceline or projectile origin on impact
	float	fSplashRadius;//radius that ent must be in to take splashDamage (linear fall-off)
	int		iAmmoPerShot;	//how much "ammo" each shot takes
	int		iHealth;		//if non-zero, projectile can be shot, takes this much damage before being destroyed
	float	fWidth;		//width of traceline or bounding box of projecile (non-rotating!)
	float	fHeight;		//height of traceline or bounding box of projecile (non-rotating!)
	int		iLifeTime;	//removes itself after this amount of time
	qboolean	bExplodeOnExpire;	//when iLifeTime is up, explodes rather than simply removing itself
} vehWeaponInfo_t;

#define	VWFOFS(x) offsetof(vehWeaponInfo_t, x)

#define MAX_VEH_WEAPONS	16	//sigh... no more than 16 different vehicle weapons
#define VEH_WEAPON_BASE	0
#define VEH_WEAPON_NONE	-1

extern vehWeaponInfo_t g_vehWeaponInfo[MAX_VEH_WEAPONS];
extern int	numVehicleWeapons;

//===========================================================================================================
//END VEHICLE WEAPONS
//===========================================================================================================

#define		MAX_VEHICLE_MUZZLES			12
#define		MAX_VEHICLE_EXHAUSTS		12
#define		MAX_VEHICLE_WEAPONS			2
#define		MAX_VEHICLE_TURRETS			2
#define		MAX_VEHICLE_TURRET_MUZZLES	2

typedef struct turretStats_s {
	int			iWeapon;	//what vehWeaponInfo index to use
	int			iDelay;		//delay between turret muzzle shots
	int			iAmmoMax;	//how much ammo it has
	int			iAmmoRechargeMS;	//how many MS between every point of recharged ammo
	char		*yawBone;	//bone on ship that this turret uses to yaw
	char		*pitchBone;	//bone on ship that this turret uses to pitch
	int			yawAxis;	//axis on yawBone to which we should to apply the yaw angles
	int			pitchAxis;	//axis on pitchBone to which we should to apply the pitch angles
	float		yawClampLeft;	//how far the turret is allowed to turn left
	float		yawClampRight;	//how far the turret is allowed to turn right
	float		pitchClampUp;	//how far the turret is allowed to title up
	float		pitchClampDown; //how far the turret is allowed to tilt down
	int			iMuzzle[MAX_VEHICLE_TURRET_MUZZLES];//iMuzzle-1 = index of ship's muzzle to fire this turret's 1st and 2nd shots from
	char		*gunnerViewTag;//Where to put the view origin of the gunner (name)
	float		fTurnSpeed;	//how quickly the turret can turn
	qboolean	bAI;	//whether or not the turret auto-targets enemies when it's not manned
	qboolean	bAILead;//whether
	float		fAIRange;	//how far away the AI will look for enemies
	int			passengerNum;//which passenger, if any, has control of this turret (overrides AI)
} turretStats_t;

typedef struct vehWeaponStats_s {
//*** IMPORTANT!!! *** See note at top of next structure!!! ***
	// Weapon stuff.
	int			ID;//index into the weapon data
	// The delay between shots for each weapon.
	int			delay;
	// Whether or not all the muzzles for each weapon can be linked together (linked delay = weapon delay * number of muzzles linked!)
	int			linkable;
	// Whether or not to auto-aim the projectiles/tracelines at the thing under the crosshair when we fire
	qboolean	aimCorrect;
	//maximum ammo
	int			ammoMax;
	//ammo recharge rate - milliseconds per unit (minimum of 100, which is 10 ammo per second)
	int			ammoRechargeMS;
	//sound to play when out of ammo (plays default "no ammo" sound if none specified)
	int			soundNoAmmo;
} vehWeaponStats_t;

typedef struct vehicleInfo_s {
//*** IMPORTANT!!! *** vehFields table correponds to this structure!
	char		*name;	//unique name of the vehicle

	//general data
	vehicleType_t	type;	//what kind of vehicle
	int			numHands;	//if 2 hands, no weapons, if 1 hand, can use 1-handed weapons, if 0 hands, can use 2-handed weapons
	float		lookPitch;	//How far you can look up and down off the forward of the vehicle
	float		lookYaw;	//How far you can look left and right off the forward of the vehicle
	float		length;		//how long it is - used for body length traces when turning/moving?
	float		width;		//how wide it is - used for body length traces when turning/moving?
	float		height;		//how tall it is - used for body length traces when turning/moving?
	vec3_t		centerOfGravity;//offset from origin: {forward, right, up} as a modifier on that dimension (-1.0f is all the way back, 1.0f is all the way forward)

	//speed stats
	float		speedMax;		//top speed
	float		turboSpeed;		//turbo speed
	float		speedMin;		//if < 0, can go in reverse
	float		speedIdle;		//what speed it drifts to when no accel/decel input is given
	float		accelIdle;		//if speedIdle > 0, how quickly it goes up to that speed
	float		acceleration;	//when pressing on accelerator
	float		decelIdle;		//when giving no input, how quickly it drops to speedIdle
	float		throttleSticks;	//if true, speed stays at whatever you accel/decel to, unless you turbo or brake
	float		strafePerc;		//multiplier on current speed for strafing.  If 1.0f, you can strafe at the same speed as you're going forward, 0.5 is half, 0 is no strafing

	//handling stats
	float		bankingSpeed;	//how quickly it pitches and rolls (not under player control)
	float		rollLimit;		//how far it can roll to either side
	float		pitchLimit;		//how far it can roll forward or backward
	float		braking;		//when pressing on decelerator
	float		mouseYaw;		// The mouse yaw override.
	float		mousePitch;		// The mouse pitch override.
	float		turningSpeed;	//how quickly you can turn
	qboolean	turnWhenStopped;//whether or not you can turn when not moving
	float		traction;		//how much your command input affects velocity
	float		friction;		//how much velocity is cut on its own
	float		maxSlope;		//the max slope that it can go up with control
	qboolean	speedDependantTurning;//vehicle turns faster the faster it's going

	//durability stats
	int			mass;			//for momentum and impact force (player mass is 10)
	int			armor;			//total points of damage it can take
	int			shields;		//energy shield damage points
	int			shieldRechargeMS;//energy shield milliseconds per point recharged
	float		toughness;		//modifies incoming damage, 1.0 is normal, 0.5 is half, etc.  Simulates being made of tougher materials/construction
	int			malfunctionArmorLevel;//when armor drops to or below this point, start malfunctioning
	int			surfDestruction; //can parts of this thing be torn off on impact? -rww

	//individual "area" health -rww
	int			health_front;
	int			health_back;
	int			health_right;
	int			health_left;

	//visuals & sounds
	char		*model;			//what model to use - if make it an NPC's primary model, don't need this?
	char		*skin;			//what skin to use - if make it an NPC's primary model, don't need this?
	int			g2radius;		//render radius for the ghoul2 model
	int			riderAnim;		//what animation the rider uses
	int			radarIconHandle;//what icon to show on radar in MP
	int			dmgIndicFrameHandle;//what image to use for the frame of the damage indicator
	int			dmgIndicShieldHandle;//what image to use for the shield of the damage indicator
	int			dmgIndicBackgroundHandle;//what image to use for the background of the damage indicator
	int			iconFrontHandle;//what image to use for the front of the ship on the damage indicator
	int			iconBackHandle;	//what image to use for the back of the ship on the damage indicator
	int			iconRightHandle;//what image to use for the right of the ship on the damage indicator
	int			iconLeftHandle;	//what image to use for the left of the ship on the damage indicator
	int			crosshairShaderHandle;//what image to use for the left of the ship on the damage indicator
	int			shieldShaderHandle;//What shader to use when drawing the shield shell
	char		*droidNPC;		//NPC to attach to *droidunit tag (if it exists in the model)

	int			soundOn;		//sound to play when get on it
	int			soundTakeOff;	//sound to play when ship takes off
	int			soundEngineStart;//sound to play when ship's thrusters first activate
	int			soundLoop;		//sound to loop while riding it
	int			soundSpin;		//sound to loop while spiraling out of control
	int			soundTurbo;		//sound to play when turbo/afterburner kicks in
	int			soundHyper;		//sound to play when ship lands
	int			soundLand;		//sound to play when ship lands
	int			soundOff;		//sound to play when get off
	int			soundFlyBy;		//sound to play when they buzz you
	int			soundFlyBy2;	//alternate sound to play when they buzz you
	int			soundShift1;	//sound to play when accelerating
	int			soundShift2;	//sound to play when accelerating
	int			soundShift3;	//sound to play when decelerating
	int			soundShift4;	//sound to play when decelerating

	int			iExhaustFX;		//exhaust effect, played from "*exhaust" bolt(s)
	int			iTurboFX;		//turbo exhaust effect, played from "*exhaust" bolt(s) when ship is in "turbo" mode
	int			iTurboStartFX;	//turbo begin effect, played from "*exhaust" bolts when "turbo" mode begins
	int			iTrailFX;		//trail effect, played from "*trail" bolt(s)
	int			iImpactFX;		//impact effect, for when it bumps into something
	int			iExplodeFX;		//explosion effect, for when it blows up (should have the sound built into explosion effect)
	int			iWakeFX;			//effect it makes when going across water
	int			iDmgFX;			//effect to play on damage from a weapon or something
	int			iInjureFX;
	int			iNoseFX;		//effect for nose piece flying away when blown off
	int			iLWingFX;		//effect for left wing piece flying away when blown off
	int			iRWingFX;		//effect for right wing piece flying away when blown off

	//Weapon stats
	vehWeaponStats_t	weapon[MAX_VEHICLE_WEAPONS];

	// Which weapon a muzzle fires (has to match one of the weapons this vehicle has). So 1 would be weapon 1,
	// 2 would be weapon 2 and so on.
	int			weapMuzzle[MAX_VEHICLE_MUZZLES];

	//turrets (if any) on the vehicle
	turretStats_t	turret[MAX_VEHICLE_TURRETS];

	// The max height before this ship (?) starts (auto)landing.
	float		landingHeight;

	//other misc stats
	int			gravity;		//normal is 800
	float		hoverHeight;	//if 0, it's a ground vehicle
	float		hoverStrength;	//how hard it pushes off ground when less than hover height... causes "bounce", like shocks
	qboolean	waterProof;		//can drive underwater if it has to
	float		bouyancy;		//when in water, how high it floats (1 is neutral bouyancy)
	int			fuelMax;		//how much fuel it can hold (capacity)
	int			fuelRate;		//how quickly is uses up fuel
	int			turboDuration;	//how long turbo lasts
	int			turboRecharge;	//how long turbo takes to recharge
	int			visibility;		//for sight alerts
	int			loudness;		//for sound alerts
	float		explosionRadius;//range of explosion
	int			explosionDamage;//damage of explosion

	int			maxPassengers;	// The max number of passengers this vehicle may have (Default = 0).
	qboolean	hideRider;		// rider (and passengers?) should not be drawn
	qboolean	killRiderOnDeath;//if rider is on vehicle when it dies, they should die
	qboolean	flammable;		//whether or not the vehicle should catch on fire before it explodes
	int			explosionDelay;	//how long the vehicle should be on fire/dying before it explodes
	//camera stuff
	qboolean	cameraOverride;	//whether or not to use all of the following 3rd person camera override values
	float		cameraRange;	//how far back the camera should be - normal is 80
	float		cameraVertOffset;//how high over the vehicle origin the camera should be - normal is 16
	float		cameraHorzOffset;//how far to left/right (negative/positive) of of the vehicle origin the camera should be - normal is 0
	float		cameraPitchOffset;//a modifier on the camera's pitch (up/down angle) to the vehicle - normal is 0
	float		cameraFOV;		//third person camera FOV, default is 80
	float		cameraAlpha;	//fade out the vehicle to this alpha (0.1-1.0f) if it's in the way of the crosshair
	qboolean	cameraPitchDependantVertOffset;//use the hacky AT-ST pitch dependant vertical offset

	//NOTE: some info on what vehicle weapon to use?  Like ATST or TIE bomber or TIE fighter or X-Wing...?

//===VEH_PARM_MAX========================================================================
//*** IMPORTANT!!! *** vehFields table correponds to this structure!

//THE FOLLOWING FIELDS are not in the vehFields table because they are internal variables, not read in from the .veh file
	int			modelIndex;		//set internally, not until this vehicle is spawned into the level

	// NOTE: Please note that most of this stuff has been converted from C++ classes to generic C.
	// This part of the structure is used to simulate inheritance for vehicles. The basic idea is that all vehicle use
	// this vehicle interface since they declare their own functions and assign the function pointer to the
	// corresponding function. Meanwhile, the base logic can still call the appropriate functions. In C++ talk all
	// of these functions (pointers) are pure virtuals and this is an abstract base class (although it cannot be
	// inherited from, only contained and reimplemented (through an object and a setup function respectively)). -AReis

	// Makes sure that the vehicle is properly animated.
	void (*AnimateVehicle)( Vehicle_t *pVeh );

	// Makes sure that the rider's in this vehicle are properly animated.
	void (*AnimateRiders)( Vehicle_t *pVeh );

	// Determine whether this entity is able to board this vehicle or not.
	qboolean (*ValidateBoard)( Vehicle_t *pVeh, bgEntity_t *pEnt );

	// Set the parent entity of this Vehicle NPC.
	void (*SetParent)( Vehicle_t *pVeh, bgEntity_t *pParentEntity );

	// Add a pilot to the vehicle.
	void (*SetPilot)( Vehicle_t *pVeh, bgEntity_t *pPilot );

	// Add a passenger to the vehicle (false if we're full).
	qboolean (*AddPassenger)( Vehicle_t *pVeh );

	// Animate the vehicle and it's riders.
	void (*Animate)( Vehicle_t *pVeh );

	// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
	qboolean (*Board)( Vehicle_t *pVeh, bgEntity_t *pEnt );

	// Eject an entity from the vehicle.
	qboolean (*Eject)( Vehicle_t *pVeh, bgEntity_t *pEnt, qboolean forceEject );

	// Eject all the inhabitants of this vehicle.
	qboolean (*EjectAll)( Vehicle_t *pVeh );

	// Start a delay until the vehicle dies.
	void (*StartDeathDelay)( Vehicle_t *pVeh, int iDelayTime );

	// Update death sequence.
	void (*DeathUpdate)( Vehicle_t *pVeh );

	// Register all the assets used by this vehicle.
	void (*RegisterAssets)( Vehicle_t *pVeh );

	// Initialize the vehicle (should be called by Spawn?).
	qboolean (*Initialize)( Vehicle_t *pVeh );

	// Like a think or move command, this updates various vehicle properties.
	qboolean (*Update)( Vehicle_t *pVeh, const usercmd_t *pUcmd );

	// Update the properties of a Rider (that may reflect what happens to the vehicle).
	//
	//	[return]		bool			True if still in vehicle, false if otherwise.
	qboolean (*UpdateRider)( Vehicle_t *pVeh, bgEntity_t *pRider, usercmd_t *pUcmd );

	// ProcessMoveCommands the Vehicle.
	void (*ProcessMoveCommands)( Vehicle_t *pVeh );

	// ProcessOrientCommands the Vehicle.
	void (*ProcessOrientCommands)( Vehicle_t *pVeh );

	// Attachs all the riders of this vehicle to their appropriate position/tag (*driver, *pass1, *pass2, whatever...).
	void (*AttachRiders)( Vehicle_t *pVeh );

	// Make someone invisible and un-collidable.
	void (*Ghost)( Vehicle_t *pVeh, bgEntity_t *pEnt );

	// Make someone visible and collidable.
	void (*UnGhost)( Vehicle_t *pVeh, bgEntity_t *pEnt );

	// Get the pilot of this vehicle.
	const bgEntity_t *(*GetPilot)( Vehicle_t *pVeh );

	// Whether this vehicle is currently inhabited (by anyone) or not.
	qboolean (*Inhabited)( Vehicle_t *pVeh );
} vehicleInfo_t;


#define	VFOFS(x) offsetof(vehicleInfo_t, x)

#define MAX_VEHICLES	16	//sigh... no more than 64 individual vehicles
#define VEHICLE_BASE	0
#define VEHICLE_NONE	-1

extern vehicleInfo_t g_vehicleInfo[MAX_VEHICLES];
extern int	numVehicles;

#define VEH_DEFAULT_SPEED_MAX		800.0f
#define VEH_DEFAULT_ACCEL			10.0f
#define VEH_DEFAULT_DECEL			10.0f
#define VEH_DEFAULT_STRAFE_PERC		0.5f
#define VEH_DEFAULT_BANKING_SPEED	0.5f
#define VEH_DEFAULT_ROLL_LIMIT		60.0f
#define VEH_DEFAULT_PITCH_LIMIT		90.0f
#define VEH_DEFAULT_BRAKING			10.0f
#define VEH_DEFAULT_TURNING_SPEED	1.0f
#define VEH_DEFAULT_TRACTION		8.0f
#define VEH_DEFAULT_FRICTION		1.0f
#define VEH_DEFAULT_MAX_SLOPE		0.85f
#define VEH_DEFAULT_MASS			200
#define VEH_DEFAULT_MAX_ARMOR		200
#define VEH_DEFAULT_TOUGHNESS		2.5f
#define VEH_DEFAULT_GRAVITY			800
#define VEH_DEFAULT_HOVER_HEIGHT	64.0f
#define VEH_DEFAULT_HOVER_STRENGTH	10.0f
#define VEH_DEFAULT_VISIBILITY		0
#define VEH_DEFAULT_LOUDNESS		0
#define VEH_DEFAULT_EXP_RAD			400.0f
#define VEH_DEFAULT_EXP_DMG			1000
#define VEH_MAX_PASSENGERS			10

#define MAX_STRAFE_TIME				2000.0f//FIXME: extern?
#define	MIN_LANDING_SPEED			200//equal to or less than this and close to ground = auto-slow-down to land
#define	MIN_LANDING_SLOPE			0.8f//must be pretty flat to land on the surf

#define VEH_MOUNT_THROW_LEFT		-5
#define	VEH_MOUNT_THROW_RIGHT		-6


typedef enum vehEject_e
{
	VEH_EJECT_LEFT,
	VEH_EJECT_RIGHT,
	VEH_EJECT_FRONT,
	VEH_EJECT_REAR,
	VEH_EJECT_TOP,
	VEH_EJECT_BOTTOM
} vehEject_t;

// Vehicle flags.
typedef enum
{
	VEH_NONE = 0, VEH_FLYING = 0x00000001, VEH_CRASHING = 0x00000002,
	VEH_LANDING = 0x00000004, VEH_BUCKING = 0x00000010, VEH_WINGSOPEN = 0x00000020,
	VEH_GEARSOPEN = 0x00000040, VEH_SLIDEBREAKING = 0x00000080, VEH_SPINNING = 0x00000100,
	VEH_OUTOFCONTROL = 0x00000200,
	VEH_SABERINLEFTHAND = 0x00000400
} vehFlags_t;

//defines for impact damage surface stuff
#define	SHIPSURF_FRONT		0
#define	SHIPSURF_BACK		1
#define	SHIPSURF_RIGHT		2
#define	SHIPSURF_LEFT		3

#define	SHIPSURF_DAMAGE_FRONT_LIGHT		0
#define	SHIPSURF_DAMAGE_BACK_LIGHT		1
#define	SHIPSURF_DAMAGE_RIGHT_LIGHT		2
#define	SHIPSURF_DAMAGE_LEFT_LIGHT		3
#define	SHIPSURF_DAMAGE_FRONT_HEAVY		4
#define	SHIPSURF_DAMAGE_BACK_HEAVY		5
#define	SHIPSURF_DAMAGE_RIGHT_HEAVY		6
#define	SHIPSURF_DAMAGE_LEFT_HEAVY		7

//generic part bits
#define SHIPSURF_BROKEN_A	(1<<0) //gear 1
#define SHIPSURF_BROKEN_B	(1<<1) //gear 1
#define SHIPSURF_BROKEN_C	(1<<2) //wing 1
#define SHIPSURF_BROKEN_D	(1<<3) //wing 2
#define SHIPSURF_BROKEN_E	(1<<4) //wing 3
#define SHIPSURF_BROKEN_F	(1<<5) //wing 4
#define SHIPSURF_BROKEN_G	(1<<6) //front

typedef struct vehWeaponStatus_s {
	//linked firing mode
	qboolean	linked;//weapon 1's muzzles are in linked firing mode
	//current weapon ammo
	int			ammo;
	//debouncer for ammo recharge
	int			lastAmmoInc;
	//which muzzle will fire next
	int			nextMuzzle;
} vehWeaponStatus_t;

typedef struct vehTurretStatus_s {
	//current weapon ammo
	int			ammo;
	//debouncer for ammo recharge
	int			lastAmmoInc;
	//which muzzle will fire next
	int			nextMuzzle;
	//which entity they're after
	int			enemyEntNum;
	//how long to hold on to our current enemy
	int			enemyHoldTime;
} vehTurretStatus_t;
// This is the implementation of the vehicle interface and any of the other variables needed. This
// is what actually represents a vehicle. -AReis.
#ifdef __GNUC__
struct Vehicle_s
#else
typedef struct Vehicle_s
#endif
{
	// The entity who pilots/drives this vehicle.
	// NOTE: This is redundant (since m_pParentEntity->owner _should_ be the pilot). This makes things clearer though.
	bgEntity_t *m_pPilot;

	int m_iPilotTime; //if spawnflag to die without pilot and this < level.time then die.
	int m_iPilotLastIndex; //index to last pilot
	qboolean m_bHasHadPilot; //qtrue once the vehicle gets its first pilot

	// The passengers of this vehicle.
	//bgEntity_t **m_ppPassengers;
	bgEntity_t *m_ppPassengers[VEH_MAX_PASSENGERS];

	//the droid unit NPC for this vehicle, if any
	bgEntity_t *m_pDroidUnit;

	// The number of passengers currently in this vehicle.
	int m_iNumPassengers;

	// The entity from which this NPC comes from.
	bgEntity_t *m_pParentEntity;

	// If not zero, how long to wait before we can do anything with the vehicle (we're getting on still).
	// -1 = board from left, -2 = board from right, -3 = jump/quick board.  -4 & -5 = throw off existing pilot
	int		m_iBoarding;

	// Used to check if we've just started the boarding process
	qboolean	m_bWasBoarding;

	// The speed the vehicle maintains while boarding occurs (often zero)
	vec3_t	m_vBoardingVelocity;

	// Time modifier (must only be used in ProcessMoveCommands() and ProcessOrientCommands() and is updated in Update()).
	float m_fTimeModifier;

	// Ghoul2 Animation info.
	//int m_iDriverTag;
	int m_iLeftExhaustTag;
	int m_iRightExhaustTag;
	int m_iGun1Tag;
	int m_iGun1Bone;
	int m_iLeftWingBone;
	int m_iRightWingBone;

	int m_iExhaustTag[MAX_VEHICLE_EXHAUSTS];
	int m_iMuzzleTag[MAX_VEHICLE_MUZZLES];
	int m_iDroidUnitTag;
	int	m_iGunnerViewTag[MAX_VEHICLE_TURRETS];//Where to put the view origin of the gunner (index)

	//this stuff is a little bit different from SP, because I am lazy -rww
	int m_iMuzzleTime[MAX_VEHICLE_MUZZLES];
	// These are updated every frame and represent the current position and direction for the specific muzzle.
	vec3_t m_vMuzzlePos[MAX_VEHICLE_MUZZLES], m_vMuzzleDir[MAX_VEHICLE_MUZZLES];

	// This is how long to wait before being able to fire a specific muzzle again. This is based on the firing rate
	// so that a firing rate of 10 rounds/sec would make this value initially 100 miliseconds.
	int m_iMuzzleWait[MAX_VEHICLE_MUZZLES];

	// The user commands structure.
	usercmd_t m_ucmd;

	// The direction an entity will eject from the vehicle towards.
	int m_EjectDir;

	// Flags that describe the vehicles behavior.
	unsigned long m_ulFlags;

	// NOTE: Vehicle Type ID, Orientation, and Armor MUST be transmitted over the net.

	// The ID of the type of vehicle this is.
	int m_iVehicleTypeID;

	// Current angles of this vehicle.
	//vec3_t		m_vOrientation;
	float		*m_vOrientation;
	//Yeah, since we use the SP code for vehicles, I want to use this value, but I'm going
	//to make it a pointer to a vec3_t in the playerstate for prediction's sake. -rww

	// How long you have strafed left or right (increments every frame that you strafe to right, decrements every frame you strafe left)
	int			m_fStrafeTime;

	// Previous angles of this vehicle.
	vec3_t		m_vPrevOrientation;

	// Previous viewangles of the rider
	vec3_t		m_vPrevRiderViewAngles;

	// When control is lost on a speeder, current angular velocity is stored here and applied until landing
	float		m_vAngularVelocity;

	vec3_t		m_vFullAngleVelocity;

	// Current armor and shields of your vehicle (explodes if armor to 0).
	int			m_iArmor;	//hull strength - STAT_HEALTH on NPC
	int			m_iShields;	//energy shielding - STAT_ARMOR on NPC

	//mp-specific
	int			m_iHitDebounce;

	// Timer for all cgame-FX...? ex: exhaust?
	int			m_iLastFXTime;

	// When to die.
	int			m_iDieTime;

	// This pointer is to a valid VehicleInfo (which could be an animal, speeder, fighter, whatever). This
	// contains the functions actually used to do things to this specific kind of vehicle as well as shared
	// information (max speed, type, etc...).
	vehicleInfo_t *m_pVehicleInfo;

	// This trace tells us if we're within landing height.
	trace_t m_LandTrace;

	// TEMP: The wing angles (used to animate it).
	vec3_t m_vWingAngles;

	//amount of damage done last impact
	int			m_iLastImpactDmg;

	//bitflag of surfaces that have broken off
	int			m_iRemovedSurfaces;

	int			m_iDmgEffectTime;

	// the last time this vehicle fired a turbo burst
	int			m_iTurboTime;

	//how long it should drop like a rock for after freed from SUSPEND
	int			m_iDropTime;

	int			m_iSoundDebounceTimer;

	//last time we incremented the shields
	int			lastShieldInc;

	//so we don't hold it down and toggle it back and forth
	qboolean	linkWeaponToggleHeld;

	//info about our weapons (linked, ammo, etc.)
	vehWeaponStatus_t	weaponStatus[MAX_VEHICLE_WEAPONS];
	vehTurretStatus_t	turretStatus[MAX_VEHICLE_TURRETS];

	//the guy who was previously the pilot
	bgEntity_t *	m_pOldPilot;
#if defined(__GNUC__) || defined(__GCC__) || defined(MINGW32) || defined(MACOS_X)
	};
#else
	} Vehicle_t;
#endif

extern int BG_VehicleGetIndex( const char *vehicleName );
