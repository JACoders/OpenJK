#ifndef __CG_CAMERA__
#define __CG_CAMERA__

//
// cg_camera.cpp

#define MAX_CAMERA_GROUP_SUBJECTS	16
#define MAX_ACCEL_PER_FRAME			10.0f
#define MAX_SHAKE_INTENSITY			16.0f
#define	CAMERA_DEFAULT_FOV			90.0f
#define CAMERA_WIDESCREEN_FOV		120.0f
#define	BAR_DURATION				1000.0f
#define BAR_RATIO					48.0f

#define CAMERA_MOVING		0x00000001
#define CAMERA_PANNING		0x00000002
#define CAMERA_ZOOMING		0x00000004
#define	CAMERA_BAR_FADING	0x00000008
#define	CAMERA_FADING		0x00000010
#define	CAMERA_FOLLOWING	0x00000020
#define	CAMERA_TRACKING		0x00000040
#define CAMERA_ROFFING		0x00000080
#define CAMERA_SMOOTHING	0x00000100
#define CAMERA_CUT			0x00000200
#define CAMERA_ACCEL		0x00000400

// NOTE!! This structure is now saved out as part of the load/save game, so tell me if you put any pointers or
//	other goofy crap in... -Ste
//
typedef struct camera_s
{
	//Position / Facing information
	vec3_t	origin;
	vec3_t	angles;
	
	vec3_t	origin2;
	vec3_t	angles2;

	//Movement information
	float	move_duration;
	float	move_time;
	int		move_type;	//CMOVE_LINEAR, CMOVE_BEZIER

	//FOV information
	float	FOV;
	float	FOV2;
	float	FOV_duration;
	float	FOV_time;
	float	FOV_vel;
	float	FOV_acc;

	//Pan information
	float	pan_time;
	float	pan_duration;

	//Following information
	char	cameraGroup[MAX_QPATH];
	float	cameraGroupZOfs;
	char	cameraGroupTag[MAX_QPATH];
	vec3_t	subjectPos;
	float	subjectSpeed;
	float	followSpeed;
	qboolean followInitLerp;
	float	distance;
	qboolean distanceInitLerp;
	//int		aimEntNum;//FIXME: remove

	//Tracking information
	int		trackEntNum;
	vec3_t	trackToOrg;
	vec3_t	moveDir;
	float	speed;
	float	initSpeed;
	float	trackInitLerp;
	int		nextTrackEntUpdateTime;

	//Cine-bar information
	float	bar_alpha;
	float	bar_alpha_source;
	float	bar_alpha_dest;
	float	bar_time;
	
	float	bar_height_source;
	float	bar_height_dest;
	float	bar_height;

	vec4_t	fade_color;
	vec4_t	fade_source;
	vec4_t	fade_dest;
	float	fade_time;
	float	fade_duration;

	//State information
	int		info_state;

	//Shake information
	float	shake_intensity;
	int		shake_duration;
	int		shake_start;

	//Smooth information
	float	smooth_intensity;
	int		smooth_duration;
	int		smooth_start;
	vec3_t	smooth_origin;
	bool	smooth_active; // means smooth_origin and angles are valid


	// ROFF information
	char	sRoff[MAX_QPATH];	// name of a cached roff 
	int		roff_frame;		// current frame in the roff data
	int		next_roff_time;	// time when it's ok to apply the next roff frame 

#ifdef _XBOX
	qboolean	widescreen;
#endif

} camera_t;

extern	bool		in_camera;
extern	camera_t	client_camera;

void CGCam_Init( void );

void CGCam_Enable( void );
void CGCam_Disable( void );

#ifdef _XBOX
void CGCam_SetWidescreen( qboolean widescreen );
#endif

void CGCam_SetPosition( vec3_t org );
void CGCam_SetAngles( vec3_t ang );
void CGCam_SetFOV( float FOV );
#ifdef _XBOX
void CGCam_SetFOV2( float FOV2 );
#endif

void CGCam_Zoom( float FOV, float duration );
//void CGCam_Pan( vec3_t	dest, float duration );
void CGCam_Pan( vec3_t dest, vec3_t panDirection, float duration );
void CGCam_Move( vec3_t dest, float duration );
void CGCam_Fade( vec4_t source, vec4_t dest, float duration );

void CGCam_UpdateFade( void );

void CGCam_Update( void );
void CGCam_RenderScene( void );
void CGCam_DrawWideScreen( void );

void CGCam_Shake( float intensity, int duration );
void CGCam_UpdateShake( vec3_t origin, vec3_t angles );

void CGCam_Follow( const char *cameraGroup, float speed, float initLerp );
void CGCam_Track( const char *trackName, float speed, float initLerp );
void CGCam_Distance( float distance, float initLerp );
void CGCam_Roll( float	dest, float duration );

void CGCam_StartRoff( char *roff );

void CGCam_Smooth( float intensity, int duration );
void CGCam_UpdateSmooth( vec3_t origin, vec3_t angles );

#endif	//__CG_CAMERA__