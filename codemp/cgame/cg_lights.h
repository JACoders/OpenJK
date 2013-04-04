#pragma once
#if !defined(CG_LIGHTS_H_INC)
#define CG_LIGHTS_H_INC

typedef struct
{
	int				length;
	color4ub_t		value;
	color4ub_t		map[MAX_QPATH];
} clightstyle_t;

void	CG_ClearLightStyles (void);
void	CG_RunLightStyles (void);
void	CG_SetLightstyle (int i);

#endif // CG_LIGHTS_H_INC
