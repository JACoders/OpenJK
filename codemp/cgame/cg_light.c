#include "cg_local.h"

typedef struct clightstyle_s {
	int				length;
	color4ub_t		value;
	color4ub_t		map[MAX_QPATH];
} clightstyle_t;

static	clightstyle_t	cl_lightstyle[MAX_LIGHT_STYLES];
static	int				lastofs;

/*
================
CG_ClearLightStyles
================
*/
void CG_ClearLightStyles (void)
{
	int	i;

	memset( cl_lightstyle, 0, sizeof( cl_lightstyle ) );
	lastofs = -1;

	for ( i=0; i<MAX_LIGHT_STYLES*3; i++ )
		CG_SetLightstyle( i );
}

/*
================
CG_RunLightStyles
================
*/
void CG_RunLightStyles (void)
{
	int ofs, i, j;
	clightstyle_t *ls;

	ofs = cg.time / 50;
//	if ( ofs == lastofs )
//		return;
	lastofs = ofs;

	for ( i=0, ls=cl_lightstyle; i<MAX_LIGHT_STYLES; i++, ls++ ) {
		union { byte b[4]; int32_t i; } a;

		ls->value[3] = 255;
		if ( !ls->length ) {
			ls->value[0] = ls->value[1] = ls->value[2] = 255;
		}
		else if ( ls->length == 1 ) {
			ls->value[0] = ls->map[0][0];
			ls->value[1] = ls->map[0][1];
			ls->value[2] = ls->map[0][2];
		//	ls->value[3] = ls->map[0][3];
		}
		else {
			ls->value[0] = ls->map[ofs%ls->length][0];
			ls->value[1] = ls->map[ofs%ls->length][1];
			ls->value[2] = ls->map[ofs%ls->length][2];
		//	ls->value[3] = ls->map[ofs%ls->length][3];
		}

		for ( j=0; j<4; j++ )
			a.b[j] = ls->value[j];
		trap->R_SetLightStyle( i, a.i );
	}
}

void CG_SetLightstyle (int i)
{
	const char	*s;
	int			j, k;

	s = CG_ConfigString( i+CS_LIGHT_STYLES );
	j = strlen (s);
	if (j >= MAX_QPATH)
	{
		Com_Error (ERR_DROP, "svc_lightstyle length=%i", j);
	}

	cl_lightstyle[(i/3)].length = j;
	for (k=0 ; k<j ; k++)
	{
		cl_lightstyle[(i/3)].map[k][(i%3)] = (float)(s[k]-'a')/(float)('z'-'a') * 255.0;
	}
}
