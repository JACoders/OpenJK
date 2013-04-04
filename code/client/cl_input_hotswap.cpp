/* 
	TODO: finalize item support

	1) Make ItemSelectUp() work.
	2) Change cg.itemSelect to whatever var is used to store selected item.
	3) Make sure commands in itemCommands work in both multi & single player.
*/

#include "client.h"
#include "../cgame/cg_local.h"

#include "cl_input_hotswap.h"
#include "../qcommon/xb_settings.h"

#define FORCESELECTTIME forcepowerSelectTime
#define FORCESELECT		forcepowerSelect
#define INVSELECTTIME	inventorySelectTime
#define INVSELECT		inventorySelect
#define REGISTERSOUND	cgi_S_RegisterSound
#define STARTSOUND		cgi_S_StartLocalSound
#define WEAPONBINDSTR	"weapon"

#define BIND_TIME 2000 //number of milliseconds button is held before binding


const char *itemCommands[INV_MAX] = {
	"use_electrobinoculars\n",
	"use_bacta\n",
	"use_seeker\n",
	"use_goggles\n",
	"use_sentry\n",
	NULL,						//goodie key
	NULL,						//security key
};

// Commands to issue when user presses a force-bound button
const char *forceDownCommands[MAX_SHOWPOWERS] = {
	"force_absorb\n",
	"force_heal\n",
	"force_protect\n",
	"force_distract\n",

	"force_speed\n",
	"force_throw\n",
	"force_pull\n",
	"force_sight\n",

	"+force_drain\n",
	"+force_lightning\n",
	"force_rage\n",
	"+force_grip\n",
};

// Commands to issue when user releases a force-bound button
const char *forceUpCommands[MAX_SHOWPOWERS] = {
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	"-force_drain\n",
	"-force_lightning\n",
	NULL,
	"-force_grip\n",
};


HotSwapManager::HotSwapManager(int uniqueID) :
	uniqueID(uniqueID)
{
	Reset();
}


const char *HotSwapManager::GetBinding(void)
{
	char buf[64];

	sprintf(buf, "hotswap%d", uniqueID);
	cvar_t *cvar = Cvar_Get(buf, "", CVAR_ARCHIVE);

	if(!cvar || !cvar->string[0])
		return NULL;

	if (cvar->integer < HOTSWAP_CAT_ITEM) {	// Weapon
		return va("weapon %d", cvar->integer);
	} else if (cvar->integer < HOTSWAP_CAT_FORCE) {	// Item
		return itemCommands[cvar->integer - HOTSWAP_CAT_ITEM];
	} else { // Force power
		return forceDownCommands[cvar->integer - HOTSWAP_CAT_FORCE];
	}
}

const char *HotSwapManager::GetBindingUp(void)
{
	char buf[64];

	sprintf(buf, "hotswap%d", uniqueID);
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
		HotSwapBind(uniqueID, HOTSWAP_CAT_WEAPON, cg.weaponSelect);
	} else if(ForceSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_FORCE, cg.FORCESELECT);
	} else if(ItemSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_ITEM, cg.INVSELECT);
	} else{
		assert(0);
	}

	noBind = true;
	STARTSOUND(REGISTERSOUND("sound/interface/update"), 0);
}


bool HotSwapManager::ForceSelectUp(void)
{
	return cg.FORCESELECTTIME != 0 &&
		(cg.FORCESELECTTIME + WEAPON_SELECT_TIME >= cg.time);
}


bool HotSwapManager::WeaponSelectUp(void)
{
	return cg.weaponSelectTime != 0 &&
		(cg.weaponSelectTime + WEAPON_SELECT_TIME >= cg.time);
}


bool HotSwapManager::ItemSelectUp(void)
{
	return cg.INVSELECTTIME != 0 &&
		(cg.INVSELECTTIME + WEAPON_SELECT_TIME >= cg.time);
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
		Cbuf_ExecuteText(EXEC_NOW, binding);
	}
}

void HotSwapManager::ExecuteUp(void)
{
	const char *binding = GetBindingUp();
	if(binding) {
		Cbuf_ExecuteText(EXEC_NOW, binding);
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
	if(!HUDInBindState()) {
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
	char uniqueID[64];
	sprintf(uniqueID, "hotswap%d", buttonID);

	// Add category as an offset for when we retrieve it
	Cvar_SetValue( uniqueID, value+category );
	Settings.hotswapSP[buttonID] = value+category;

	Settings.Save();
}

