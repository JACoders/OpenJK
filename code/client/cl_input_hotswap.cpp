/*
	TODO: finalize item support

	1) Make ItemSelectUp() work.
	2) Change cg.itemSelect to whatever var is used to store selected item.
	3) Make sure commands in itemCommands work in both multi & single player.
*/

#ifdef _JK2MP
#include "../../codemp/client/client.h"
#include "../../codemp/cgame/cg_local.h"
#else
#include "client.h"
#include "../cgame/cg_local.h"
#endif

#include "cl_input_hotswap.h"


#ifdef _JK2MP
#define FORCESELECTTIME forceSelectTime
#define FORCESELECT		forceSelect
#define INVSELECTTIME	invenSelectTime
#define INVSELECT		itemSelect
#define REGISTERSOUND	S_RegisterSound
#define STARTSOUND		S_StartLocalSound
#define WEAPONBINDSTR	"weaponclean"
#else
#define FORCESELECTTIME forcepowerSelectTime
#define FORCESELECT		forcepowerSelect
#define INVSELECTTIME	inventorySelectTime
#define INVSELECT		inventorySelect
#define REGISTERSOUND	cgi_S_RegisterSound
#define STARTSOUND		cgi_S_StartLocalSound
#define WEAPONBINDSTR	"weapon"
#endif

#define BIND_TIME 3000 //number of milliseconds button is held before binding
#define EXEC_TIME 500  //max ms button can be held to execute in bind mode


#ifdef _JK2MP
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
#else
const char *itemCommands[INV_MAX] = {
	"use_electrobinoculars\n",
	"use_bacta\n",
	"use_seeker\n",
	"use_goggles\n",
	"use_sentry\n",
	NULL,						//goodie key
	NULL,						//security key
};
#endif



HotSwapManager::HotSwapManager(int uniqueID) :
	uniqueID(uniqueID),
	forceBound(false)
{
	Reset();
}


char *HotSwapManager::GetBinding(void)
{
	char buf[64];

	sprintf(buf, "hotswap%d", uniqueID);
	cvar_t *cvar = Cvar_Get(buf, "", CVAR_ARCHIVE);

	if(cvar && cvar->string[0] != 0) {
		return cvar->string;
	} else {
		return NULL;
	}
}


void HotSwapManager::Bind(void)
{
	forceBound = false;

	if(WeaponSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_WEAPON, cg.weaponSelect);
	} else if(ForceSelectUp()) {
		forceBound = true;
		HotSwapBind(uniqueID, HOTSWAP_CAT_FORCE,
#ifdef _JK2MP
				cg.FORCESELECT
#else
				showPowers[cg.FORCESELECT]
#endif
				);
	} else if(ItemSelectUp()) {
		HotSwapBind(uniqueID, HOTSWAP_CAT_ITEM, cg.INVSELECT);
	} else{
		assert(0);
	}

	noExec = true;
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

			//If a force power is bound, want to execute whenever the button
			//is down to handle powers which can be held.
			if(forceBound) {
				Execute();
			}
		}
		downTime += cls.frametime;
	}

	//Down long enough, bind button.
	if(!noBind && bindTime >= BIND_TIME) {
		Bind();
	}
}


void HotSwapManager::Execute(void)
{
	char *binding = GetBinding();
	if(binding && !noExec) {
		if(!forceBound) {
			noExec = true;
		}
		Cbuf_ExecuteText(EXEC_APPEND, binding);
	}
}


void HotSwapManager::SetDown(void)
{
	//Set the down flag.
	down = true;

	//Execute the bind if the HUD isn't up.
	if(!HUDInBindState()) {
		Execute();
	}
}


void HotSwapManager::SetUp(void)
{
	//Execute the bind if the button was held down for long enough.
	if(downTime <= EXEC_TIME) {
		Execute();
	}

	Reset();
}


void HotSwapManager::Reset(void)
{
	down = false;
	downTime = 0;
	bindTime = 0;
	noExec = false;
	noBind = false;
}


static void HotSwapBind(const char *uniqueID, const char *value)
{
	Cvar_Set(uniqueID, value);
}


void HotSwapBind(int buttonID, int category, int value)
{
	char uniqueID[64];
	sprintf(uniqueID, "hotswap%d", buttonID);

	switch(category) {
	case HOTSWAP_CAT_WEAPON:
		HotSwapBind(uniqueID, va("%s %d\n", WEAPONBINDSTR, value));
		break;
	case HOTSWAP_CAT_ITEM:
		assert(itemCommands[value]);
		HotSwapBind(uniqueID, itemCommands[value]);
		break;
	case HOTSWAP_CAT_FORCE:
		HotSwapBind(uniqueID, va("useGivenForce %d\n", value));
		break;
	default:
		assert(0);
	}
}

