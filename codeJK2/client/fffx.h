// Filename:-	fffx.h		(Force Feedback FX)

#ifndef FFFX_H
#define FFFX_H

// this part can be seen by the CGAME as well...

// These enums match the generic ones built into the effects ROM in the MS SideWinder FF Joystick, 
//	so blame MS for anything you don't like (like that aircraft carrier one - jeez!)...
//
// (Judging from the names of most of these, the MS FF guys appear to be rather fond of ID-type games...)
//
typedef enum 
	{
	fffx_RandomNoise=0,
	fffx_AircraftCarrierTakeOff,	// this one is pointless / dumb
	fffx_BasketballDribble,
	fffx_CarEngineIdle,
	fffx_ChainsawIdle,
	fffx_ChainsawInAction,
	fffx_DieselEngineIdle,
	fffx_Jump,
	fffx_Land,
	fffx_MachineGun,
	fffx_Punched,
	fffx_RocketLaunch,
	fffx_SecretDoor,
	fffx_SwitchClick,
	fffx_WindGust,
	fffx_WindShear,		// also pretty crap
	fffx_Pistol,
	fffx_Shotgun,
	fffx_Laser1,
	fffx_Laser2,
	fffx_Laser3,
	fffx_Laser4,
	fffx_Laser5,
	fffx_Laser6,
	fffx_OutOfAmmo,
	fffx_LightningGun,
	fffx_Missile,
	fffx_GatlingGun,
	fffx_ShortPlasma,
	fffx_PlasmaCannon1,
	fffx_PlasmaCannon2,
	fffx_Cannon,
	//
	fffx_NUMBEROF,
	fffx_NULL		// special use, ignore during array mallocs etc, use fffx_NUMBEROF instead
	} ffFX_e;


#ifndef CGAME_ONLY

/////////////////////////// START of functions to call /////////////////////////////////
//
//
//
// Once the game is running you should *only* access FF functions through these 4 macros. Savvy?
//
// Usage note: In practice, you can have about 4 FF FX running concurrently, though I defy anyone to make sense
//	of that amount of white-noise vibration. MS guidelines say you shouldn't leave vibration on for long periods
//	of time (eg engine rumble) because of various nerve-damage/lawsuit issues, so for this reason there is no API
//	support for setting durations etc. All FF FX stuff here is designed for things like firing, hit damage, driving
//	over bumps, etc that can be played as one-off events, though you *can* do things like FFFX_ENSURE(fffx_ChainsawIdle)
//	if you really want to.
//
// Combining small numbers of effects such as having a laser firing (MS photons have higher mass apparently <g>), 
//	and then firing a machine gun in bursts as well are no problem, and easily felt, hence the ability to stop playing
//	individual FF FX.
//
#define FFFX_START(f)	FF_Play(f)
#define FFFX_ENSURE(f)	FF_EnsurePlaying(f)
#define FFFX_STOP(f)	FF_Stop(f)			// some effects (eg. gatling, chainsaw), need this, or they play too long after trigger-off.
#define FFFX_STOPALL	FF_StopAll()


//
// These 2 are called at app start/stop, but you can call FF_Init to change FF devices anytime (takes a couple of seconds)
//
void		FF_Init(qboolean bTryMouseFirst=true);
void		FF_Shutdown(void);
//
// other stuff you may want to call but don't have to...
//
qboolean	FF_IsAvailable(void);	
qboolean	FF_IsMouse(void);		
qboolean	FF_SetTension(int iTension);	// tension setting 0..3 (0=none)
qboolean	FF_SetSpring(long lSpring);		// precision version of above, 0..n..10000
											// (only provided for command line fiddling with 
											//	weird hardware. FF_SetTension(1) = default
											// = FF_SetSpring(2000) (internal lookup table))

//
//
//
/////////////////////////// END of functions to call /////////////////////////////////




// do *not* call this functions directly (or else!), use the macros above
//
void FF_Play			(ffFX_e fffx);
void FF_EnsurePlaying	(ffFX_e fffx);
void FF_Stop			(ffFX_e fffx);
void FF_StopAll			(void);

#define MAX_CONCURRENT_FFFXs 4	// only for my code to use/read, do NOT alter!



#endif	// #ifndef CGAME_ONLY
#endif	// #ifndef FFFX_H

//////////////////////// eof //////////////////////

