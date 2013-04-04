#include "../cgame/cg_local.h"
#include "cl_data.h"


//tvector(ClientData*)			ClientManager::clientData;
ClientData*						ClientManager::clientData[2] = { NULL };
int								ClientManager::curClient = 0;
ClientData*						ClientManager::activeClient = 0;
int								ClientManager::mainClient = 0;
qboolean						ClientManager::splitScreenMode = qfalse;
int								ClientManager::numClients = 0;
bool							ClientManager::bClOutside = false;

ClientData::ClientData()
:	in_mlooking(qfalse), cmd_wait(0), loopbacks(0), eventHead(0), eventTail(0), serverCommandSequence(0),
//    cg_thirdPersonSpecialCam(0), cg_thirdPerson(0), cg_thirdPersonRange(80), cg_thirdPersonAngle(0),	// Pulled back for Xbox - also in cg_main
    cg_thirdPersonSpecialCam(0), cg_thirdPerson(0), cg_thirdPersonRange(90), cg_thirdPersonAngle(0),
	cg_thirdPersonPitchOffset(0), cg_thirdPersonVertOffset(16), cg_thirdPersonCameraDamp(0.3),
	cg_thirdPersonTargetDamp(0.5), cg_thirdPersonAlpha(1.0), cg_thirdPersonHorzOffset(0),
	key_overstrikeMode(qfalse), anykeydown(qfalse), cg_crossHairStatus(0), cg_autolevel(0), cg_pitch(-0.022),
	cg_sensitivity(2), cg_autoswitch(1), controller(-1), shake_intensity(0),
	shake_duration(0), shake_start(0), cg_sensitivityY(2), lastFireTime(0), cvar_modifiedFlags(0),forceConfig(0),
	modelIndex(0), forceSide(1), voiceTogglePressTime(0), swapMan1(HOTSWAP_ID_WHITE), swapMan2(HOTSWAP_ID_BLACK), myTeam(0), isSpectating(qfalse)

{
	state = CA_DISCONNECTED;
//	memset( &state, 0, sizeof(connstate_t));

	memset( &mv_clc, 0, sizeof( mv_clc ) );
	m_clc = &mv_clc;

	memset( &mv_cg , 0, sizeof( mv_cg  ) );
	m_cg = &mv_cg;

//	memset( &mv_cl, 0, sizeof( mv_cl ) );
//	m_cl = &mv_cl;

	memset(&in_left, 0 , sizeof(kbutton_t));
	memset(&in_right, 0 , sizeof(kbutton_t));
	memset(&in_forward, 0 , sizeof(kbutton_t));
	memset(&in_back, 0 , sizeof(kbutton_t));
	
	memset(&in_lookup, 0 , sizeof(kbutton_t));
	memset(&in_lookdown, 0 , sizeof(kbutton_t));
	memset(&in_moveleft, 0 , sizeof(kbutton_t));
	memset(&in_moveright, 0 , sizeof(kbutton_t));
	
	memset(&in_strafe, 0 , sizeof(kbutton_t));
	memset(&in_speed, 0 , sizeof(kbutton_t));
	memset(&in_up, 0 , sizeof(kbutton_t));
	memset(&in_down, 0 , sizeof(kbutton_t));
	
	memset(in_buttons, 0, sizeof(kbutton_t) * 16);

	// Command Stuff
	memset(&cmd_text, 0, sizeof(msg_t));
	memset(cmd_text_buf, 0, sizeof(byte) *MAX_CMD_BUFFER);
	memset(cmd_defer_text_buf, 0, sizeof(char) * MAX_CMD_BUFFER);
	MSG_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));

	memset( keys, 0 , sizeof(qkey_t) *MAX_KEYS);

	memset(eventQue, 0, sizeof(sysEvent_t) * MAX_QUED_EVENTS);

	memset( joyInfo, 0, sizeof(JoystickInfo) * IN_MAX_JOYSTICKS);

	strcpy(autoName, "Padawan");


	model[0] = 0;
	
	strcpy(saber_color1, "blue");
	strcpy(saber_color2, "blue");

	forcePowers[0] = 0;
	//forcePowersCustom[0] = 0;

	strcpy(forcePowers, "7-1-032330000000001333");
	//strcpy(forcePowersCustom, "7-1-032330000000001333");

	strcpy(model,"kyle/default");
	strcpy(char_color_red,"255");
	strcpy(char_color_green,"255");
	strcpy(char_color_blue,"255");
	strcpy(saber1,"single_1");
	strcpy(saber2,"none");
	strcpy(color1,"4");
	strcpy(color2,"4");
}

ClientData::~ClientData()
{
	// Really nasty hack to prevent HeapFree() being called here
	Z_PushNewDeleteTag( TAG_NEWDEL );

	// If we got any bindings via S_Malloc, make sure to kill them here
	for( int i = 0; i < MAX_KEYS; ++i )
		if( keys[i].binding )
			Z_Free( keys[i].binding );

	Z_PopNewDeleteTag();
}

bool ClientManager::ActivateByControllerId(int id)
{
	for (int i = 0; i < ClientManager::NumClients(); i++) {
		if (clientData[i]->controller == id) {
			ActivateClient(i);
//			SetMainClient(i);
			return true;
		}
	}
	return false;
}


void ClientManager::ClientForceInit ( int clientNum )
{
	int oldClientNum = curClient;
	ActivateClient(clientNum);
	SetMainClient(clientNum);
	
	ClientForceInit();

	ActivateClient(oldClientNum);
	SetMainClient(oldClientNum);
}

void ClientManager::ClientForceInit( void )
{
	MSG_Init (&activeClient->cmd_text, activeClient->cmd_text_buf, sizeof(activeClient->cmd_text_buf));
	//exec control01.cfg
	if (!activeClient->loopbacks)
	{
		Z_PushNewDeleteTag( TAG_CLIENT_MANAGER );

		activeClient->loopbacks = new loopback_t[2];
		memset(activeClient->loopbacks, 0, sizeof(loopback_t) * 2);
	
		Z_PopNewDeleteTag( );
	}
//	if (!activeClient->m_cl) activeClient->m_cl = new clientActive_t;
	
//	memset( activeClient->m_cl, 0, sizeof( clientActive_t ) );

	activeClient->state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED	

	Cbuf_ExecuteText( EXEC_APPEND, "exec control01.cfg\n" );

}

extern void IN_KeyUp( kbutton_t *b );
void ClientManager::ClearAllClientInputs(void)
{
	int i,j;
	
	for ( i = 0 ; i < numClients; i++)
	{
		for ( j = 0 ; j < 16; j++)
		{
			if (clientData[i]->in_buttons[j].active)
			{
				//	IN_KeyUp(&clientData[i]->in_buttons[j]);
				clientData[i]->in_buttons[j].down[0] = clientData[i]->in_buttons[j].down[1] = 0;
				clientData[i]->in_buttons[j].active = qfalse;
			}
		}
		if ( clientData[i]->in_down.active)
		{
		//	IN_KeyUp(&clientData[i]->in_down);
			clientData[i]->in_down.down[0] = clientData[i]->in_down.down[1] = 0;
			clientData[i]->in_down.active = qfalse;
		}
	}
}

void ClientManager::ClientForceAllInit( void ) 
{
	for (int i = 0; i < numClients; i++) {
		ClientForceInit(i);
	}
}

void ClientManager::Init(int _numClients)
{
	Shutdown();

	Z_PushNewDeleteTag( TAG_CLIENT_MANAGER );

	numClients = _numClients;
	for (int i = 0; i < _numClients; i++)
	{
		clientData[i] = new ClientData();
		clientData[i]->m_cl = new clientActive_t;
		memset( clientData[i]->m_cl, 0, sizeof(clientActive_t) );
	}

	ActivateClient(0);

	mainClient = 0;

	Z_PopNewDeleteTag();
}

void ClientManager::Shutdown( void )
{
//	tvector(ClientData*)::iterator begin = clientData.begin(), end = clientData.end();
	for ( int i = 0; i < numClients; ++i )
		delete clientData[i];
//	clientData.clear();
	numClients = 0;
	curClient = 0;
	activeClient = 0;
}

void ClientManager::Resize( int n )
{
	Z_PushNewDeleteTag( TAG_CLIENT_MANAGER );

	if (n < numClients )
	{
		for (int i = n; i < numClients; i++)
		{
			if (clientData[i]->loopbacks)
				delete clientData[i]->loopbacks;
//			if (clientData[i]->m_cl) delete clientData[i]->m_cl;
			
			Z_PushNewDeleteTag( TAG_CLIENT_MANAGER_SPECIAL );
			delete clientData[n]->m_cl;
			delete clientData[n];
			Z_PopNewDeleteTag( );
		}
//		clientData.resize(n);
		numClients = n;
	}
	else if (n > numClients )
	{
		for( int i = numClients; i < n; ++i )
		{
			clientData[i] = new ClientData();
			clientData[i]->m_cl = new clientActive_t;
			memset( clientData[i]->m_cl, 0, sizeof(clientActive_t) );
		}
		numClients = n;
//		int diff = n - numClients;
//		clientData.reserve(diff);
//		for ( ; diff > 0; diff--)
//			clientData.push_back(new ClientData());
	}

	Z_PopNewDeleteTag( );
}

int ClientManager::AddClient( void )
{
	// Special tag that tells Z_Malloc to use HeapAlloc() instead - and thus
	// use the same memory as ModelMem.
	Z_PushNewDeleteTag( TAG_CLIENT_MANAGER_SPECIAL );	//TAG_CLIENT_MANAGER );

//	clientData.push_back(new ClientData()); 
//	return static_cast<int>(clientData.size()) - 1; 
	clientData[numClients] = new ClientData();
	clientData[numClients]->m_cl = new clientActive_t;
	memset( clientData[numClients]->m_cl, 0, sizeof(clientActive_t) );
	clientData[numClients]->m_cg->widescreen = cg->widescreen;
	numClients++;

	Z_PopNewDeleteTag( );

	return numClients - 1;
}

bool ClientManager::IsAClientControlller ( int id )
{
	for (int i = 0; i < ClientManager::NumClients(); i++) {
		if (clientData[i]->controller == id)
			return true;
	}
	return false;
}

void ClientManager::ClientFeedDeferedScript(void )
{
	if ( *(activeClient->cmd_defer_text_buf) != 0)
	{
		Cbuf_AddText( activeClient->cmd_defer_text_buf );
		*(activeClient->cmd_defer_text_buf) = 0;
	}
}

void ClientManager::ClientActiveRelocate( bool bOutside )
{
	if( bOutside == bClOutside )
		return;

	Z_PushNewDeleteTag( bOutside ? TAG_CLIENT_MANAGER_SPECIAL : TAG_CLIENT_MANAGER );
	clientActive_t *newCl = new clientActive_t;
	Z_PopNewDeleteTag();

	memcpy( newCl, clientData[0]->m_cl, sizeof(clientActive_t) );
	memset( clientData[0]->m_cl, 0, sizeof(clientActive_t) );

	Z_PushNewDeleteTag( !bOutside ? TAG_CLIENT_MANAGER_SPECIAL : TAG_CLIENT_MANAGER );
	delete clientData[0]->m_cl;
	Z_PopNewDeleteTag();

	clientData[0]->m_cl = newCl;
	bClOutside = bOutside;
	if( curClient == 0 )
		cl = newCl;
}

// Hacky function for server code that can't seem to include cl_data properly:
bool SplitScreenModeActive( void )
{
	return ClientManager::splitScreenMode;
}
