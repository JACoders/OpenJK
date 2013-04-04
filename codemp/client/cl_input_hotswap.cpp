/* 
	TODO: finalize item support

	1) Make ItemSelectUp() work.
	2) Change cg->itemSelect to whatever var is used to store selected item.
	3) Make sure commands in itemCommands work in both multi & single player.
*/

#include "client.h"
#include "../cgame/cg_local.h"

#include "cl_input_hotswap.h"
#include "cl_data.h"
#include "../qcommon/xb_settings.h"

#define FORCESELECTTIME forceSelectTime
#define FORCESELECT		forceSelect
#define INVSELECTTIME	invenSelectTime
#define INVSELECT		itemSelect
#define REGISTERSOUND	S_RegisterSound
#define STARTSOUND		S_StartLocalSound
#define WEAPONBINDSTR	"weaponclean"

#define BIND_TIME 2000 //number of milliseconds button is held before binding
#define EXEC_TIME 500  //max ms button can be held to execute in bind mode


const char *itemCommands[HI_NUM_HOLDABLE] = {
	NULL,						//HI_NONE
	"use_seeker\n",
	"use_field\n",
	"use_bacta\n",
	"use_bactabig\n",
	"use_electrobinoculars\n",
	"use_sentry\n",
	"use_jetpack\n",
	NULL,						//ammo dispenser
	NULL,						//health dispenser
	"use_eweb\n",
	"use_cloak\n",
};

// Commands to issue when user presses a force-bound button
const char *forceDownCommands[NUM_FORCE_POWERS] = {
	"force_heal\n",			// FP_HEAL
	NULL,					// FP_LEVITATION
	"force_speed\n",		// FP_SPEED
	"force_throw\n",		// FP_PUSH
	"force_pull\n",			// FP_PULL
	"force_distract\n",		// FP_TELEPATHY
	"+force_grip\n",		// FP_GRIP
	"+force_lightning\n",	// FP_LIGHTNING
	"force_rage\n",			// FP_RAGE
	"force_protect\n",		// FP_PROTECT
	"force_absorb\n",		// FP_ABSORB
	"force_healother\n",	// FP_TEAM_HEAL
	"force_forcepowerother\n",	// FP_TEAM_FORCE
	"+force_drain\n",		// FP_DRAIN
	"force_seeing\n",		// FP_SEE
	NULL,					// FP_SABER_OFFENSE
	NULL,					// FP_SABER_DEFENSE
	NULL,					// FP_SABERTHROW
};

// Commands to issue when user releases a force-bound button
const char *forceUpCommands[NUM_FORCE_POWERS] = {
	NULL,				// FP_HEAL
	NULL,				// FP_LEVITATION
	NULL,				// FP_SPEED
	NULL,				// FP_PUSH
	NULL,				// FP_PULL
	NULL,				// FP_TELEPATHY
	"-force_grip\n",	// FP_GRIP
	"-force_lightning\n",	// FP_LIGHTNING
	NULL,				// FP_RAGE
	NULL,				// FP_PROTECT
	NULL,				// FP_ABSORB
	NULL,				// FP_TEAM_HEAL
	NULL,				// FP_TEAM_FORCE
	"-force_drain\n",	// FP_DRAIN
	NULL,				// FP_SEE
	NULL,				// FP_SABER_OFFENSE
	NULL,				// FP_SABER_DEFENSE
	NULL,				// FP_SABERTHROW
};


HotSwapManager::HotSwapManager(int uniqueID) :
	uniqueID(uniqueID)
{
	Reset();
}


const char *HotSwapManager::GetBinding(void)
{
	char buf[64];

	// Need to use unique variables for each client in split screen:
	sprintf(buf, "hotswap%d", uniqueID+(ClientManager::ActiveClientNum()*4));
	cvar_t *cvar = Cvar_Get(buf, "", CVAR_ARCHIVE);

	if(!cvar || !cvar->string[0])
		return NULL;

	if (cvar->integer < HOTSWAP_CAT_ITEM) {	// Weapon
		return va("weaponclean %d", cvar->integer);
	} else if (cvar->integer < HOTSWAP_CAT_FORCE) {	// Item
		return itemCommands[cvar->integer - HOTSWAP_CAT_ITEM];
	} else { // Force power
		return forceDownCommands[cvar->integer - HOTSWAP_CAT_FORCE];
	}
}

const char *HotSwapManager::GetBindingUp(void)
{
	char buf[64];

	sprintf(buf, "hotswap%d", uniqueID+(ClientManager::ActiveClientNum()*4));
	cvar_t *cvar = Cvar_Get(buf, "", CVAR_ARCHIVE);

	if(!cvar || !cvar->string[0])
		return NULL;

	// Only force powers have release-commands
	if (cvar->integer < HOTSWAP_CAT_FORCE) {
		return NULL;
	} else {
		return forceUpCommands[cvar->integer - HOTSWAP_CAT_FORCE];
	}
}


void HotSwapManager::Bind(void)
{
	if(WeaponSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_WEAPON, cg->weaponSelect);
	} else if(ForceSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_FORCE, cg->FORCESELECT);
	} else if(ItemSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_ITEM, cg->INVSELECT);
	} else{
		assert(0);
	}

	noBind = true;
	STARTSOUND(REGISTERSOUND("sound/interface/update"), 0);
}


bool HotSwapManager::ForceSelectUp(void)
{
	return cg->FORCESELECTTIME != 0 &&
		(cg->FORCESELECTTIME + WEAPON_SELECT_TIME >= cg->time);
}


bool HotSwapManager::WeaponSelectUp(void)
{
	return cg->weaponSelectTime != 0 &&
		(cg->weaponSelectTime + WEAPON_SELECT_TIME >= cg->time);
}


bool HotSwapManager::ItemSelectUp(void)
{
	return cg->INVSELECTTIME != 0 &&
		(cg->INVSELECTTIME + WEAPON_SELECT_TIME >= cg->time);
}


bool HotSwapManager::HUDInBindState(void)
{
	return ForceSelectUp() || WeaponSelectUp() || ItemSelectUp();
}


void HotSwapManager::Update(void)
{
	if(down) {
		//Increment bindTime only if HUD is in select mode.
		if(HUDInBindState()) {
			bindTime += cls.frametime;
		} else {

			//Clear bind time.
			bindTime = 0;

			}
		}

	//Down long enough, bind button.
	if(!noBind && bindTime >= BIND_TIME) {
		Bind();
	}
}


void HotSwapManager::Execute(void)
{
	const char *binding = GetBinding();
	if(binding) {
		Cbuf_ExecuteText(EXEC_APPEND, binding);
	}
}

void HotSwapManager::ExecuteUp(void)
{
	const char *binding = GetBindingUp();
	if(binding) {
		Cbuf_ExecuteText(EXEC_APPEND, binding);
	}
}


void HotSwapManager::SetDown(void)
{
	//Set the down flag.
	down = true;

	//Execute the bind if the HUD isn't up. Also, prevent re-binding!
	if(!HUDInBindState()) {
		Execute();
		noBind = true;
	}
}


void HotSwapManager::SetUp(void)
{
	// Execute the tail of the command if the HUD isn't up.
	if(!HUDInBindState() || noBind) {
		ExecuteUp();
	}

	Reset();
}


void HotSwapManager::Reset(void)
{
	down = false;
	bindTime = 0;
	noBind = false;
}

void HotSwapBind(int buttonID, int category, int value)
{
	char buf[64];
	sprintf(buf, "hotswap%d", buttonID+(ClientManager::ActiveClientNum()*4));

	// Add category as an offset for when we retrieve it
	Cvar_SetValue( buf, value+category );
	Settings.hotswapMP[buttonID+(ClientManager::ActiveClientNum()*2)] = value+category;

	Settings.Save();
}

