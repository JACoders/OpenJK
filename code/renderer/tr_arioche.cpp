/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#include "../server/exe_headers.h"

#include "tr_local.h"
#include "tr_WorldEffects.h"

// Patches up the loaded map to handle the parameters passed from the UI

// Remap sky to contents of the cvar ar_sky
// Grab sunlight properties from the indirected sky

//void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset);
void R_ColorShiftLightingBytes( byte in[4], byte out[4] );

void NormalToLatLong( const vec3_t normal, byte bytes[2] )
{
	// check for singularities
	if (!normal[0] && !normal[1])
	{
		if ( normal[2] > 0.0f )
		{
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		}
		else
		{
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	}
	else
	{
		int	a, b;

		a = (int)(RAD2DEG( (vec_t)atan2( normal[1], normal[0] ) ) * (255.0f / 360.0f ));
		a &= 0xff;

		b = (int)(RAD2DEG( (vec_t)acos( normal[2] ) ) * ( 255.0f / 360.0f ));
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

void R_RMGInit(void)
{
	char			newSky[MAX_QPATH];
	char			newFog[MAX_QPATH];
	shader_t		*sky;
	shader_t		*fog;
	fog_t			*gfog;
	mgrid_t			*grid;
	char			temp[MAX_QPATH];
	int				i;
	unsigned short	*pos;

	Cvar_VariableStringBuffer("RMG_sky", newSky, MAX_QPATH);
	// Get sunlight - this should set up all the sunlight data
	sky = R_FindShader( newSky, lightmapsNone, stylesDefault, qfalse );

	// Remap sky
//	R_RemapShader("textures/tools/_sky", newSky, NULL);

	// Fill in the lightgrid with sunlight
	if(tr.world->lightGridData)
	{
#ifdef _XBOX
		byte *memory = (byte *)tr.world->lightGridData;

		byte *array;
		array = memory;
		memory += 3;

		array[0] = (byte)Com_Clamp(0, 255, tr.sunAmbient[0] * 255.0f);
		array[1] = (byte)Com_Clamp(0, 255, tr.sunAmbient[1] * 255.0f);
		array[2] = (byte)Com_Clamp(0, 255, tr.sunAmbient[2] * 255.0f);
		
		array[3] = (byte)Com_Clamp(0, 255, tr.sunLight[0]);
		array[4] = (byte)Com_Clamp(0, 255, tr.sunLight[1]);
		array[5] = (byte)Com_Clamp(0, 255, tr.sunLight[2]);
		
		NormalToLatLong(tr.sunDirection, grid->latLong);
#else // _XBOX
		grid = tr.world->lightGridData;
		grid->ambientLight[0][0] = (byte)Com_Clamp(0, 255, tr.sunAmbient[0] * 255.0f);
		grid->ambientLight[0][1] = (byte)Com_Clamp(0, 255, tr.sunAmbient[1] * 255.0f);
		grid->ambientLight[0][2] = (byte)Com_Clamp(0, 255, tr.sunAmbient[2] * 255.0f);
		R_ColorShiftLightingBytes(grid->ambientLight[0], grid->ambientLight[0]);

		grid->directLight[0][0] = (byte)Com_Clamp(0, 255, tr.sunLight[0]);
		grid->directLight[0][1] = (byte)Com_Clamp(0, 255, tr.sunLight[1]);
		grid->directLight[0][2] = (byte)Com_Clamp(0, 255, tr.sunLight[2]);
		R_ColorShiftLightingBytes(grid->directLight[0], grid->directLight[0]);

		NormalToLatLong(tr.sunDirection, grid->latLong);
#endif // _XBOX

		pos = tr.world->lightGridArray;
		for(i=0;i<tr.world->numGridArrayElements;i++)
		{
			*pos = 0;
			pos++;
		}
	}

	// Override the global fog with the defined one
	if(tr.world->globalFog != -1)
	{
		Cvar_VariableStringBuffer("RMG_fog", newFog, MAX_QPATH);
		fog = R_FindShader( newFog, lightmapsNone, stylesDefault, qfalse);
		if (fog != tr.defaultShader)
		{
			gfog = tr.world->fogs + tr.world->globalFog;
			gfog->parms = *fog->fogParms;
			if (gfog->parms.depthForOpaque)
			{
				gfog->tcScale = 1.0f / ( gfog->parms.depthForOpaque * 8.0f );
				tr.distanceCull = gfog->parms.depthForOpaque;
				Cvar_Set("RMG_distancecull", va("%f", tr.distanceCull));
			}
			else
			{
				gfog->tcScale = 1.0f;
			}
			gfog->colorInt = ColorBytes4 ( gfog->parms.color[0], 
										  gfog->parms.color[1], 
										  gfog->parms.color[2], 1.0f );
		}
	}

	Cvar_VariableStringBuffer("RMG_weather", temp, MAX_QPATH);

	// Set up any weather effects
	switch(atol(temp))
	{
	case 0:
		break;
	case 1:
		R_WorldEffectCommand("rain init 1000");
		R_WorldEffectCommand("rain outside");
		break;
	case 2:
		R_WorldEffectCommand("snow init 1000 outside");
		R_WorldEffectCommand("snow outside");
		break;
	}
}

// end
