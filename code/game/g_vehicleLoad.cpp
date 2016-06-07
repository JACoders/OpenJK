//g_vehicleLoad.cpp
// leave this line at the top for all NPC_xxxx.cpp files...
#include "g_headers.h"
#include "../qcommon/q_shared.h"
#include "anims.h"
#include "g_vehicles.h"

extern qboolean G_ParseLiteral( const char **data, const char *string );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );

vehicleInfo_t g_vehicleInfo[MAX_VEHICLES];
int		numVehicles = 1;//first one is null/default

typedef enum {
	VF_INT,
	VF_FLOAT,
	VF_LSTRING,	// string on disk, pointer in memory, TAG_LEVEL
	VF_VECTOR,
	VF_BOOL,
	VF_VEHTYPE,
	VF_ANIM
} vehFieldType_t;

typedef struct
{
	char	*name;
	int		ofs;
	vehFieldType_t	type;
} vehField_t;

vehField_t vehFields[VEH_PARM_MAX] =
{
	{"name", VFOFS(name), VF_LSTRING},	//unique name of the vehicle

	//general data
	{"type", VFOFS(type), VF_VEHTYPE},	//what kind of vehicle

	{"numHands", VFOFS(numHands), VF_INT},	//if 2 hands, no weapons, if 1 hand, can use 1-handed weapons, if 0 hands, can use 2-handed weapons
	{"lookPitch", VFOFS(lookPitch), VF_FLOAT},	//How far you can look up and down off the forward of the vehicle
	{"lookYaw", VFOFS(lookYaw), VF_FLOAT},	//How far you can look left and right off the forward of the vehicle
	{"length", VFOFS(length), VF_FLOAT},		//how long it is - used for body length traces when turning/moving?
	{"width", VFOFS(width), VF_FLOAT},		//how wide it is - used for body length traces when turning/moving?
	{"height", VFOFS(height), VF_FLOAT},		//how tall it is - used for body length traces when turning/moving?
	{"centerOfGravity", VFOFS(centerOfGravity), VF_VECTOR},//offset from origin: {forward, right, up} as a modifier on that dimension (-1.0f is all the way back, 1.0f is all the way forward)

	//speed stats
	{"speedMax", VFOFS(speedMax), VF_FLOAT},		//top speed
	{"turboSpeed", VFOFS(turboSpeed), VF_FLOAT},	//turbo speed
	{"speedMin", VFOFS(speedMin), VF_FLOAT},		//if < 0, can go in reverse
	{"speedIdle", VFOFS(speedIdle), VF_FLOAT},		//what speed it drifts to when no accel/decel input is given
	{"accelIdle", VFOFS(accelIdle), VF_FLOAT},		//if speedIdle > 0, how quickly it goes up to that speed
	{"acceleration", VFOFS(acceleration), VF_FLOAT},	//when pressing on accelerator
	{"decelIdle", VFOFS(decelIdle), VF_FLOAT},		//when giving no input, how quickly it drops to speedIdle
	{"strafePerc", VFOFS(strafePerc), VF_FLOAT},		//multiplier on current speed for strafing.  If 1.0f, you can strafe at the same speed as you're going forward, 0.5 is half, 0 is no strafing

	//handling stats
	{"bankingSpeed", VFOFS(bankingSpeed), VF_FLOAT},	//how quickly it pitches and rolls (not under player control)
	{"pitchLimit", VFOFS(pitchLimit), VF_FLOAT},		//how far it can roll forward or backward
	{"rollLimit", VFOFS(rollLimit), VF_FLOAT},		//how far it can roll to either side
	{"braking", VFOFS(braking), VF_FLOAT},		//when pressing on decelerator
	{"turningSpeed", VFOFS(turningSpeed), VF_FLOAT},	//how quickly you can turn
	{"turnWhenStopped", VFOFS(turnWhenStopped), VF_BOOL},//whether or not you can turn when not moving
	{"traction", VFOFS(traction), VF_FLOAT},		//how much your command input affects velocity
	{"friction", VFOFS(friction), VF_FLOAT},		//how much velocity is cut on its own
	{"maxSlope", VFOFS(maxSlope), VF_FLOAT},		//the max slope that it can go up with control

	//durability stats
	{"mass", VFOFS(mass), VF_INT},			//for momentum and impact force (player mass is 10)
	{"armor", VFOFS(armor), VF_INT},			//total points of damage it can take
	{"toughness", VFOFS(toughness), VF_FLOAT},		//modifies incoming damage, 1.0 is normal, 0.5 is half, etc.  Simulates being made of tougher materials/construction
	{"malfunctionArmorLevel", VFOFS(malfunctionArmorLevel), VF_INT},//when armor drops to or below this point, start malfunctioning

	//visuals & sounds
	{"model", VFOFS(model), VF_LSTRING},			//what model to use - if make it an NPC's primary model, don't need this?
	{"skin", VFOFS(skin), VF_LSTRING},			//what skin to use - if make it an NPC's primary model, don't need this?
	{"riderAnim", VFOFS(riderAnim), VF_ANIM},		//what animation the rider uses
	{"gunswivelBone", VFOFS(gunswivelBone), VF_LSTRING},//gun swivel bones
	{"lFinBone", VFOFS(lFinBone), VF_LSTRING},			//left fin bone
	{"rFinBone", VFOFS(rFinBone), VF_LSTRING},			//right fin bone
	{"lExhaustTag", VFOFS(lExhaustTag), VF_LSTRING},	//left exhaust tag
	{"rExhaustTag", VFOFS(rExhaustTag), VF_LSTRING},	//right exhaust tag

	{"soundOn", VFOFS(soundOn), VF_LSTRING},		//sound to play when get on it
	{"soundLoop", VFOFS(soundLoop), VF_LSTRING},		//sound to loop while riding it
	{"soundOff", VFOFS(soundOff), VF_LSTRING},		//sound to play when get off
	{"exhaustFX", VFOFS(exhaustFX), VF_LSTRING},		//exhaust effect, played from "*exhaust" bolt(s)
	{"trailFX", VFOFS(trailFX), VF_LSTRING},		//trail effect, played from "*trail" bolt(s)
	{"impactFX", VFOFS(impactFX), VF_LSTRING},		//impact effect, for when it bumps into something
	{"explodeFX", VFOFS(explodeFX), VF_LSTRING},		//explosion effect, for when it blows up (should have the sound built into explosion effect)
	{"wakeFX", VFOFS(wakeFX), VF_LSTRING},		//effect it makes when going across water

	//other misc stats
	{"gravity", VFOFS(gravity), VF_INT},			//normal is 800
	{"hoverHeight", VFOFS(hoverHeight), VF_FLOAT},	//if 0, it's a ground vehicle
	{"hoverStrength", VFOFS(hoverStrength), VF_FLOAT},	//how hard it pushes off ground when less than hover height... causes "bounce", like shocks
	{"waterProof", VFOFS(waterProof), VF_BOOL},		//can drive underwater if it has to
	{"bouyancy", VFOFS(bouyancy), VF_FLOAT},		//when in water, how high it floats (1 is neutral bouyancy)
	{"fuelMax", VFOFS(fuelMax), VF_INT},		//how much fuel it can hold (capacity)
	{"fuelRate", VFOFS(fuelRate), VF_INT},		//how quickly is uses up fuel
	{"visibility", VFOFS(visibility), VF_INT},		//for sight alerts
	{"loudness", VFOFS(loudness), VF_INT},		//for sound alerts
	{"explosionRadius", VFOFS(explosionRadius), VF_FLOAT},//range of explosion
	{"explosionDamage", VFOFS(explosionDamage), VF_INT},//damage of explosion

	//new stuff
	{"maxPassengers", VFOFS(maxPassengers), VF_INT},	// The max number of passengers this vehicle may have (Default = 0).
	{"hideRider", VFOFS(hideRider), VF_BOOL },		// rider (and passengers?) should not be drawn
	{"killRiderOnDeath", VFOFS(killRiderOnDeath), VF_BOOL },//if rider is on vehicle when it dies, they should die
	{"flammable", VFOFS(flammable), VF_BOOL },		//whether or not the vehicle should catch on fire before it explodes
	{"explosionDelay", VFOFS(explosionDelay), VF_INT},	//how long the vehicle should be on fire/dying before it explodes
	//camera stuff
	{"cameraOverride", VFOFS(cameraOverride), VF_BOOL },	//override the third person camera with the below values - normal is 0 (off)
	{"cameraRange", VFOFS(cameraRange), VF_FLOAT},	//how far back the camera should be - normal is 80
	{"cameraVertOffset", VFOFS(cameraVertOffset), VF_FLOAT},//how high over the vehicle origin the camera should be - normal is 16
	{"cameraHorzOffset", VFOFS(cameraHorzOffset), VF_FLOAT},//how far to left/right (negative/positive) of of the vehicle origin the camera should be - normal is 0
	{"cameraPitchOffset", VFOFS(cameraPitchOffset), VF_FLOAT},//a modifier on the camera's pitch (up/down angle) to the vehicle - normal is 0
	{"cameraFOV", VFOFS(cameraFOV), VF_FLOAT},		//third person camera FOV, default is 80
	{"cameraAlpha", VFOFS(cameraAlpha), VF_BOOL },	//fade out the vehicle if it's in the way of the crosshair
};

stringID_table_t VehicleTable[VH_NUM_VEHICLES+1] =
{
	ENUM2STRING(VH_WALKER),		//something you ride inside of, it walks like you, like an AT-ST
	ENUM2STRING(VH_FIGHTER),	//something you fly inside of, like an X-Wing or TIE fighter
	ENUM2STRING(VH_SPEEDER),	//something you ride on that hovers, like a speeder or swoop
	ENUM2STRING(VH_ANIMAL),		//animal you ride on top of that walks, like a tauntaun
	ENUM2STRING(VH_FLIER),		//animal you ride on top of that flies, like a giant mynoc?
	"",	-1
};

void G_VehicleSetDefaults( vehicleInfo_t *vehicle )
{
	vehicle->name = "default";		//unique name of the vehicle
/*
	//general data
	vehicle->type = VH_SPEEDER;				//what kind of vehicle
	//FIXME: no saber or weapons if numHands = 2, should switch to speeder weapon, no attack anim on player
	vehicle->numHands = 2;					//if 2 hands, no weapons, if 1 hand, can use 1-handed weapons, if 0 hands, can use 2-handed weapons
	vehicle->lookPitch = 35;				//How far you can look up and down off the forward of the vehicle
	vehicle->lookYaw = 5;					//How far you can look left and right off the forward of the vehicle
	vehicle->length = 0;					//how long it is - used for body length traces when turning/moving?
	vehicle->width = 0;						//how wide it is - used for body length traces when turning/moving?
	vehicle->height = 0;					//how tall it is - used for body length traces when turning/moving?
	VectorClear( vehicle->centerOfGravity );//offset from origin: {forward, right, up} as a modifier on that dimension (-1.0f is all the way back, 1.0f is all the way forward)

	//speed stats - note: these are DESIRED speed, not actual current speed/velocity
	vehicle->speedMax = VEH_DEFAULT_SPEED_MAX;	//top speed
	vehicle->turboSpeed = 0;					//turboBoost
	vehicle->speedMin = 0;						//if < 0, can go in reverse
	vehicle->speedIdle = 0;						//what speed it drifts to when no accel/decel input is given
	vehicle->accelIdle = 0;						//if speedIdle > 0, how quickly it goes up to that speed
	vehicle->acceleration = VEH_DEFAULT_ACCEL;	//when pressing on accelerator (1/2 this when going in reverse)
	vehicle->decelIdle = VEH_DEFAULT_DECEL;		//when giving no input, how quickly it desired speed drops to speedIdle
	vehicle->strafePerc = VEH_DEFAULT_STRAFE_PERC;//multiplier on current speed for strafing.  If 1.0f, you can strafe at the same speed as you're going forward, 0.5 is half, 0 is no strafing

	//handling stats
	vehicle->bankingSpeed = VEH_DEFAULT_BANKING_SPEED;	//how quickly it pitches and rolls (not under player control)
	vehicle->rollLimit = VEH_DEFAULT_ROLL_LIMIT;		//how far it can roll to either side
	vehicle->pitchLimit = VEH_DEFAULT_PITCH_LIMIT;		//how far it can pitch forward or backward
	vehicle->braking = VEH_DEFAULT_BRAKING;				//when pressing on decelerator (backwards)
	vehicle->turningSpeed = VEH_DEFAULT_TURNING_SPEED;	//how quickly you can turn
	vehicle->turnWhenStopped = qfalse;					//whether or not you can turn when not moving
	vehicle->traction = VEH_DEFAULT_TRACTION;			//how much your command input affects velocity
	vehicle->friction = VEH_DEFAULT_FRICTION;			//how much velocity is cut on its own
	vehicle->maxSlope = VEH_DEFAULT_MAX_SLOPE;			//the max slope that it can go up with control

	//durability stats
	vehicle->mass = VEH_DEFAULT_MASS;			//for momentum and impact force (player mass is 10)
	vehicle->armor = VEH_DEFAULT_MAX_ARMOR;		//total points of damage it can take
	vehicle->toughness = VEH_DEFAULT_TOUGHNESS;	//modifies incoming damage, 1.0 is normal, 0.5 is half, etc.  Simulates being made of tougher materials/construction
	vehicle->malfunctionArmorLevel = 0;			//when armor drops to or below this point, start malfunctioning

	//visuals & sounds
	vehicle->model = "swoop";								//what model to use - if make it an NPC's primary model, don't need this?
	vehicle->modelIndex = 0;							//set internally, not until this vehicle is spawned into the level
	vehicle->skin = NULL;								//what skin to use - if make it an NPC's primary model, don't need this?
	vehicle->riderAnim = BOTH_GUNSIT1;					//what animation the rider uses
	vehicle->gunswivelBone = NULL;						//gun swivel bones
	vehicle->lFinBone = NULL;							//left fin bone
	vehicle->rFinBone = NULL;							//right fin bone
	vehicle->lExhaustTag = NULL;						//left exhaust tag
	vehicle->rExhaustTag = NULL;						//right exhaust tag

	vehicle->soundOn = NULL;							//sound to play when get on it
	vehicle->soundLoop = NULL;							//sound to loop while riding it
	vehicle->soundOff = NULL;							//sound to play when get off
	vehicle->exhaustFX = NULL;							//exhaust effect, played from "*exhaust" bolt(s)
	vehicle->trailFX = NULL;							//trail effect, played from "*trail" bolt(s)
	vehicle->impactFX = NULL;							//explosion effect, for when it blows up (should have the sound built into explosion effect)
	vehicle->explodeFX = NULL;							//explosion effect, for when it blows up (should have the sound built into explosion effect)
	vehicle->wakeFX = NULL;								//effect itmakes when going across water

	//other misc stats
	vehicle->gravity = VEH_DEFAULT_GRAVITY;				//normal is 800
	vehicle->hoverHeight = 0;	//if 0, it's a ground vehicle
	vehicle->hoverStrength = 0;//how hard it pushes off ground when less than hover height... causes "bounce", like shocks
	vehicle->waterProof = qtrue;						//can drive underwater if it has to
	vehicle->bouyancy = 1.0f;							//when in water, how high it floats (1 is neutral bouyancy)
	vehicle->fuelMax = 1000;							//how much fuel it can hold (capacity)
	vehicle->fuelRate = 1;								//how quickly is uses up fuel
	vehicle->visibility = VEH_DEFAULT_VISIBILITY;		//radius for sight alerts
	vehicle->loudness = VEH_DEFAULT_LOUDNESS;			//radius for sound alerts
	vehicle->explosionRadius = VEH_DEFAULT_EXP_RAD;
	vehicle->explosionDamage = VEH_DEFAULT_EXP_DMG;

	//new stuff
	vehicle->maxPassengers = 0;
	vehicle->hideRider = qfalse;						// rider (and passengers?) should not be drawn
	vehicle->killRiderOnDeath = qfalse;					//if rider is on vehicle when it dies, they should die
	vehicle->flammable = qfalse;						//whether or not the vehicle should catch on fire before it explodes
	vehicle->explosionDelay = 0;						//how long the vehicle should be on fire/dying before it explodes
	//camera stuff
	vehicle->cameraOverride = qfalse;					//whether or not to use all of the following 3rd person camera override values
	vehicle->cameraRange = 0.0f;						//how far back the camera should be - normal is 80
	vehicle->cameraVertOffset = 0.0f;					//how high over the vehicle origin the camera should be - normal is 16
	vehicle->cameraHorzOffset = 0.0f;					//how far to left/right (negative/positive) of of the vehicle origin the camera should be - normal is 0
	vehicle->cameraPitchOffset = 0.0f;					//a modifier on the camera's pitch (up/down angle) to the vehicle - normal is 0
	vehicle->cameraFOV = 0.0f;							//third person camera FOV, default is 80
	vehicle->cameraAlpha = qfalse;						//fade out the vehicle if it's in the way of the crosshair
*/
}

void G_VehicleClampData( vehicleInfo_t *vehicle )
{//sanity check and clamp the vehicle's data
	int		i;

	for ( i = 0; i < 3; i++ )
	{
		if ( vehicle->centerOfGravity[i] > 1.0f )
		{
			vehicle->centerOfGravity[i] = 1.0f;
		}
		else if ( vehicle->centerOfGravity[i] < -1.0f )
		{
			vehicle->centerOfGravity[i] = -1.0f;
		}
	}

	// Validate passenger max.
	if ( vehicle->maxPassengers > VEH_MAX_PASSENGERS )
	{
		vehicle->maxPassengers = VEH_MAX_PASSENGERS;
	}
	else if ( vehicle->maxPassengers < 0 )
	{
		vehicle->maxPassengers = 0;
	}
}


static void G_ParseVehicleParms( vehicleInfo_t *vehicle, const char **holdBuf )
{
	const char	*token;
	const char	*value;
	int		i;
	vec3_t	vec;
	byte	*b = (byte *)vehicle;
	int		_iFieldsRead = 0;
	vehicleType_t vehType;

	while ( holdBuf )
	{
		token = COM_ParseExt( holdBuf, qtrue );
		if ( !token[0] )
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing vehicles!\n" );
			return;
		}

		if ( !Q_stricmp( token, "}" ) )	// End of data for this vehicle
		{
			break;
		}

		// Loop through possible parameters
		for ( i = 0; i < VEH_PARM_MAX; i++ )
		{
			if ( vehFields[i].name && !Q_stricmp( vehFields[i].name, token ) )
			{
				// found it
				if ( COM_ParseString( holdBuf, &value ) )
				{
					continue;
				}
				switch( vehFields[i].type )
				{
				case VF_INT:
					*(int *)(b+vehFields[i].ofs) = atoi(value);
					break;
				case VF_FLOAT:
					*(float *)(b+vehFields[i].ofs) = atof(value);
					break;
				case VF_LSTRING:	// string on disk, pointer in memory, TAG_LEVEL
					*(char **)(b+vehFields[i].ofs) = G_NewString( value );
					break;
				case VF_VECTOR:
					_iFieldsRead = sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
					assert(_iFieldsRead==3 );
					if (_iFieldsRead!=3)
					{
						gi.Printf (S_COLOR_YELLOW"G_ParseVehicleParms: VEC3 sscanf() failed to read 3 floats ('angle' key bug?)\n");
					}
					((float *)(b+vehFields[i].ofs))[0] = vec[0];
					((float *)(b+vehFields[i].ofs))[1] = vec[1];
					((float *)(b+vehFields[i].ofs))[2] = vec[2];
					break;
				case VF_BOOL:
					*(qboolean *)(b+vehFields[i].ofs) = (atof(value)!=0);
					break;
				case VF_VEHTYPE:
					vehType = (vehicleType_t)GetIDForString( VehicleTable, value );
					*(vehicleType_t *)(b+vehFields[i].ofs) = vehType;
					break;
				case VF_ANIM:
					int anim = GetIDForString( animTable, value );
					*(int *)(b+vehFields[i].ofs) = anim;
					break;
				}
				break;
			}
		}
	}
}

static void G_VehicleStoreParms( const char *p )
{//load up all into a table: g_vehicleInfo
	const char	*token;
	vehicleInfo_t	*vehicle;

	////////////////// HERE //////////////////////
	// The first vehicle just contains all the base level (not 'overridden') function calls.
	G_SetSharedVehicleFunctions( &g_vehicleInfo[0] );
	numVehicles = 1;

	//try to parse data out
	COM_BeginParseSession();

	//look for an open brace
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{//barf
			return;
		}

		if ( !Q_stricmp( token, "{" ) )
		{//found one, parse out the goodies
			if ( numVehicles >= MAX_VEHICLES )
			{//sorry, no more vehicle slots!
				gi.Printf( S_COLOR_RED"Too many vehicles in *.veh (limit %d)\n", MAX_VEHICLES );
				break;
			}
			//token = token;
			vehicle = &g_vehicleInfo[numVehicles++];
			G_VehicleSetDefaults( vehicle );
			G_ParseVehicleParms( vehicle, &p );
			//sanity check and clamp the vehicle's data
			G_VehicleClampData( vehicle );
	////////////////// HERE //////////////////////
			// Setup the shared function pointers.
			G_SetSharedVehicleFunctions( vehicle );
			switch( vehicle->type )
			{
				case VH_SPEEDER:
					G_SetSpeederVehicleFunctions( vehicle );
					break;
				case VH_ANIMAL:
					G_SetAnimalVehicleFunctions( vehicle );
					break;
				case VH_FIGHTER:
					G_SetFighterVehicleFunctions( vehicle );
					break;
				case VH_WALKER:
					G_SetAnimalVehicleFunctions( vehicle );
					break;
			}
		}
	}
}

void G_VehicleLoadParms( void )
{//HMM... only do this if there's a vehicle on the level?
	int			len, totallen, vehExtFNLen, fileCnt, i;
	char		*buffer, *holdChar, *marker;
	char		vehExtensionListBuf[2048];			//	The list of file names read in

	#define MAX_VEHICLE_DATA_SIZE 0x20000
	char	VehicleParms[MAX_VEHICLE_DATA_SIZE]={0};

	//	gi.Printf( "Parsing *.veh vehicle definitions\n" );

	//set where to store the first one
	totallen = 0;
	marker = VehicleParms;

	//now load in the .veh vehicle definitions
	fileCnt = gi.FS_GetFileList("ext_data/vehicles", ".veh", vehExtensionListBuf, sizeof(vehExtensionListBuf) );

	holdChar = vehExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += vehExtFNLen + 1 )
	{
		vehExtFNLen = strlen( holdChar );

		//gi.Printf( "Parsing %s\n", holdChar );

		len = gi.FS_ReadFile( va( "ext_data/vehicles/%s", holdChar), (void **) &buffer );

		if ( len == -1 )
		{
			gi.Printf( "G_VehicleLoadParms: error reading file %s\n", holdChar );
		}
		else
		{
			if ( totallen && *(marker-1) == '}' )
			{//don't let it end on a } because that should be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++;
			}
			if ( totallen + len >= MAX_VEHICLE_DATA_SIZE ) {
				G_Error( "G_VehicleLoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)", holdChar );
			}
			strcat( marker, buffer );
			gi.FS_FreeFile( buffer );

			totallen += len;
			marker += len;
		}
	}
	G_VehicleStoreParms(VehicleParms);
}
