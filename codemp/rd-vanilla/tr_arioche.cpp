#include "tr_local.h"
#include "tr_WorldEffects.h"

// Patches up the loaded map to handle the parameters passed from the UI

// Remap sky to contents of the cvar ar_sky
// Grab sunlight properties from the indirected sky

void R_RMGInit(void)
{
	char			newSky[MAX_QPATH];
	char			newFog[MAX_QPATH];
	shader_t		*fog;
	fog_t			*gfog;
	mgrid_t			*grid;
	char			temp[MAX_QPATH];
	int				i;
	unsigned short	*pos;

	ri->Cvar_VariableStringBuffer("RMG_sky", newSky, MAX_QPATH);
	// Get sunlight - this should set up all the sunlight data
	R_FindShader( newSky, lightmapsNone, stylesDefault, qfalse );

	// Remap sky
	R_RemapShader("textures/tools/_sky", newSky, NULL);

	// Fill in the lightgrid with sunlight
	if(tr.world->lightGridData)
	{
		grid = tr.world->lightGridData;
		grid->ambientLight[0][0] = (byte)Com_Clampi(0, 255, tr.sunAmbient[0] * 255.0f);
		grid->ambientLight[0][1] = (byte)Com_Clampi(0, 255, tr.sunAmbient[1] * 255.0f);
		grid->ambientLight[0][2] = (byte)Com_Clampi(0, 255, tr.sunAmbient[2] * 255.0f);
		R_ColorShiftLightingBytes(grid->ambientLight[0], grid->ambientLight[0]);

		grid->directLight[0][0] = (byte)Com_Clampi(0, 255, tr.sunLight[0]);
		grid->directLight[0][1] = (byte)Com_Clampi(0, 255, tr.sunLight[1]);
		grid->directLight[0][2] = (byte)Com_Clampi(0, 255, tr.sunLight[2]);
		R_ColorShiftLightingBytes(grid->directLight[0], grid->directLight[0]);

		NormalToLatLong(tr.sunDirection, grid->latLong);

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
		ri->Cvar_VariableStringBuffer("RMG_fog", newFog, MAX_QPATH);
		fog = R_FindShader( newFog, lightmapsNone, stylesDefault, qfalse);
		if (fog != tr.defaultShader)
		{
			gfog = tr.world->fogs + tr.world->globalFog;
			gfog->parms = *fog->fogParms;
			if (gfog->parms.depthForOpaque)
			{
				gfog->tcScale = 1.0f / ( gfog->parms.depthForOpaque * 8.0f );
				tr.distanceCull = gfog->parms.depthForOpaque;
				tr.distanceCullSquared = tr.distanceCull * tr.distanceCull;
				ri->Cvar_Set("RMG_distancecull", va("%f", tr.distanceCull));
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

	ri->Cvar_VariableStringBuffer("RMG_weather", temp, MAX_QPATH);

	// Set up any weather effects
	switch(atol(temp))
	{
	case 0:
		break;
	case 1:
		RE_WorldEffectCommand("rain init 1000");
		RE_WorldEffectCommand("rain outside");
		break;
	case 2:
		RE_WorldEffectCommand("snow init 1000 outside");
		RE_WorldEffectCommand("snow outside");
		break;
	}
}

// end
