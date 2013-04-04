#ifndef _CL_DATA_H_
#define _CL_DATA_H_

//#include "../cgame/cg_local.h"
//#include "../game/g_shared.h"
//#include "../game/g_local.h"
#include "client.h"
#include "cl_input_hotswap.h"

#include "../win32/win_input.h"

#include <vector>
#define	MAX_CMD_BUFFER	4096
#define	MAX_LOOPBACK	16

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )
#define STRING_SIZE 64


typedef struct {
	byte	data[1359];
	int		datalen;
} loopmsg_t;

typedef struct {
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

#define CM_START_LOOP()		ClientManager::StartIt(); do {
#define CM_END_LOOP()		} while (ClientManager::NextIt());


#define IN_MAX_JOYSTICKS 2

class ClientData
{
public:
	ClientData();
	~ClientData();

public:
	connstate_t	state;				// connection status
	
//	clientActive_t		mv_cl;
	clientActive_t*		m_cl;

	clientConnection_t	mv_clc;
	clientConnection_t *m_clc;

	cg_t				mv_cg;
	cg_t			   *m_cg;

	// Button Data
	kbutton_t	in_left, in_right, in_forward, in_back;
	kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
	kbutton_t	in_strafe, in_speed;
	kbutton_t	in_up, in_down;
	kbutton_t	in_buttons[16];
	qboolean	in_mlooking;

	// Hot-swap button data
	HotSwapManager swapMan1;
	HotSwapManager swapMan2;

	// Command Text Data
	int			cmd_wait;
	msg_t		cmd_text;
	byte		cmd_text_buf[MAX_CMD_BUFFER];
	char		cmd_defer_text_buf[MAX_CMD_BUFFER];

	// NET stuff
	loopback_t	*loopbacks;

	// event stuff
	sysEvent_t	eventQue[MAX_QUED_EVENTS];
	int			eventHead, eventTail;

	// key stuff	
	qboolean	key_overstrikeMode;
	qboolean	anykeydown;
	qkey_t		keys[MAX_KEYS];

	// Used for autolevel
	unsigned long lastFireTime;
	// Used for taunting
	unsigned long voiceTogglePressTime;

	// controller stuff
	int			controller;

	JoystickInfo joyInfo[IN_MAX_JOYSTICKS];

	int		serverCommandSequence;

	int			cg_thirdPersonSpecialCam;

	int			cg_thirdPerson;
	int			cg_thirdPersonRange;
	int			cg_thirdPersonAngle;
	int			cg_thirdPersonPitchOffset;
	int			cg_thirdPersonVertOffset;
	float		cg_thirdPersonCameraDamp;
	float		cg_thirdPersonTargetDamp;

	float		cg_thirdPersonAlpha;
	int			cg_thirdPersonHorzOffset;

	vec3_t	cameraFocusAngles,			cameraFocusLoc;
	vec3_t	cameraIdealTarget,			cameraIdealLoc;
	vec3_t	cameraCurTarget,			cameraCurLoc;
	int		cameraLastFrame;

	float	cameraLastYaw;
	float	cameraStiffFactor;

	int		cg_crossHairStatus;

	short	cg_autolevel;
	float	cg_pitch;
	float	cg_sensitivity;
#ifdef _XBOX
	float	cg_sensitivityY;
#endif
	short	cg_autoswitch;

	int	cvar_modifiedFlags;

	char autoName[STRING_SIZE];

	//character Model and saber info
	char model[STRING_SIZE];
	int modelIndex;
	//colors
	char char_color_red[STRING_SIZE];
	char char_color_green[STRING_SIZE];
	char char_color_blue[STRING_SIZE];
	
	char saber1[STRING_SIZE];
	char saber2[STRING_SIZE];
	char color1[STRING_SIZE];
	char color2[STRING_SIZE];
	char saber_color1[STRING_SIZE];
	char saber_color2[STRING_SIZE];
	char forcePowers[STRING_SIZE];
//	char forcePowersCustom[STRING_SIZE];
	int forceSide;
	int forceConfig;
	int myTeam;
	qboolean isSpectating;


//	char profileName[STRING_SIZE];



	// Camera shake stuff
	float		shake_intensity;
	int			shake_duration;
	int			shake_start;

	int cgRageTime;
	int cgRageFadeTime;
	float cgRageFadeVal;

	int cgRageRecTime;
	int cgRageRecFadeTime;
	float cgRageRecFadeVal;

	int cgAbsorbTime;
	int cgAbsorbFadeTime;
	float cgAbsorbFadeVal;

	int cgProtectTime;
	int cgProtectFadeTime;
	float cgProtectFadeVal;

	int cgYsalTime;
	int cgYsalFadeTime;
	float cgYsalFadeVal;
};


class ClientManager
{
public:

	__inline static void				StartIt( void ) { ActivateClient(0);  }
	__inline static bool				NextIt( void )		
	{ 
//		if ( curClient + 1 >= static_cast<int>(clientData.size()) ) {
		if ( curClient + 1 >= numClients ) {
			ActivateClient(mainClient);
			return false;
		}
		ActivateClient( curClient + 1); 
		return true;
	}

	__inline static int				ActiveController( void ) {  return activeClient->controller; }
	
	__inline static void			SetActiveController( int ctl ) { activeClient->controller = ctl; }
	__inline static void			SetMainClient( int client)	{ mainClient = client; }
	__inline static void			ActivateMainClient( void )	{ ActivateClient(mainClient); }
	__inline static ClientData&		ActiveClient( void )	{ return *activeClient; }
	__inline static ClientData&		GetClient( int i )	{ return *clientData[i] ; }
	__inline static int				ActiveClientNum( void ) { return curClient; }
	__inline static int				ActivePort ( void )		{ return Cvar_VariableIntegerValue("qport") + curClient; }

	__inline static bool			IsActiveClient (int n)	{ return (n == curClient); }
	__inline static int				NumClients( void )		{ return numClients; }

	__inline static int				Controller(int clientNum) { return clientData[clientNum]->controller; }	
	__inline static void ActivateClient( int client )
	{
		if (client >= numClients) return;

//		clientData[curClient]->m_cg = cg;
//		if(clientData[curClient]->m_cl)
//            clientData[curClient]->m_cl = cl;
//		clientData[curClient]->m_clc = clc;
		// Need to check when removing second client:
		if( curClient < numClients )
			clientData[curClient]->state = cls.state;

		curClient = client;
		activeClient = clientData[curClient];

		cg = clientData[curClient]->m_cg;
//		if(clientData[curClient]->m_cl)
            cl = clientData[curClient]->m_cl;
		clc = clientData[curClient]->m_clc;
		cls.state = clientData[curClient]->state;
	}

	static int	AddClient( void );
	static void Resize( int n );		
	static void Shutdown( void );
	static void Init( int _numClients );

	static void						ClientForceInit (int client);
	static void						ClientForceInit ( void );
	static void						ClearAllClientInputs(void);
	static void						ClientForceAllInit ( void );

	static bool						ActivateByControllerId( int id);
	static bool						IsAClientControlller ( int id );
	static void						ClientFeedDeferedScript(void );
	
	static void						ClientActiveRelocate( bool bOutside );
	
private:
	ClientManager() {}
	~ClientManager() {}
	operator =(const ClientManager&);

private:
//	static tvector(ClientData*)		clientData;
	static ClientData*				clientData[2];
	static int						curClient;
	static int						mainClient;
	static ClientData*				activeClient;
	static int						numClients;

	static bool						bClOutside;

public:
	static qboolean					splitScreenMode;
};

#endif
