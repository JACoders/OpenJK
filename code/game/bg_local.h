// bg_local.h -- local definitions for the bg (both games) files

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001F

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct {
	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	qboolean	walking;
	qboolean	groundPlane;
	trace_t		groundTrace;

	float		impactSpeed;

	vec3_t		previous_origin;
	vec3_t		previous_velocity;
	int			previous_waterlevel;
} pml_t;

extern	pmove_t		*pm;
extern	pml_t		pml;

// movement parameters
extern	const float	pm_stopspeed;
extern	const float	pm_duckScale;
extern	const float	pm_swimScale;
extern	const float	pm_wadeScale;

extern	const float	pm_accelerate;
extern	const float	pm_airaccelerate;
extern	const float	pm_wateraccelerate;
extern	const float	pm_flyaccelerate;

extern	const float	pm_friction;
extern	const float	pm_waterfriction;
extern	const float	pm_flightfriction;

extern	int		c_pmove;

void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce );
void PM_AddTouchEnt( int entityNum );
void PM_AddEvent( int newEvent );

qboolean	PM_SlideMove( float gravity );
void		PM_StepSlideMove( float gravity );



