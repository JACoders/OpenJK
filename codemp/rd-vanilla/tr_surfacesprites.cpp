/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// tr_shade.c

#include "tr_local.h"

#include "tr_quicksprite.h"
#include "tr_WorldEffects.h"


/////===== Part of the VERTIGON system =====/////
// The surfacesprites are a simple system.  When a polygon with this shader stage on it is drawn,
// there are randomly distributed images (defined by the shader stage) placed on the surface.
// these are capable of doing effects, grass, or simple oriented sprites.
// They usually stick vertically off the surface, hence the term vertigons.

// The vertigons are applied as part of the renderer backend.  That is, they access OpenGL calls directly.


unsigned char randomindex, randominterval;
const float randomchart[256] = {
	0.6554f, 0.6909f, 0.4806f, 0.6218f, 0.5717f, 0.3896f, 0.0677f, 0.7356f,
	0.8333f, 0.1105f, 0.4445f, 0.8161f, 0.4689f, 0.0433f, 0.7152f, 0.0336f,
	0.0186f, 0.9140f, 0.1626f, 0.6553f, 0.8340f, 0.7094f, 0.2020f, 0.8087f,
	0.9119f, 0.8009f, 0.1339f, 0.8492f, 0.9173f, 0.5003f, 0.6012f, 0.6117f,
	0.5525f, 0.5787f, 0.1586f, 0.3293f, 0.9273f, 0.7791f, 0.8589f, 0.4985f,
	0.0883f, 0.8545f, 0.2634f, 0.4727f, 0.3624f, 0.1631f, 0.7825f, 0.0662f,
	0.6704f, 0.3510f, 0.7525f, 0.9486f, 0.4685f, 0.1535f, 0.1545f, 0.1121f,
	0.4724f, 0.8483f, 0.3833f, 0.1917f, 0.8207f, 0.3885f, 0.9702f, 0.9200f,
	0.8348f, 0.7501f, 0.6675f, 0.4994f, 0.0301f, 0.5225f, 0.8011f, 0.1696f,
	0.5351f, 0.2752f, 0.2962f, 0.7550f, 0.5762f, 0.7303f, 0.2835f, 0.4717f,
	0.1818f, 0.2739f, 0.6914f, 0.7748f, 0.7640f, 0.8355f, 0.7314f, 0.5288f,
	0.7340f, 0.6692f, 0.6813f, 0.2810f, 0.8057f, 0.0648f, 0.8749f, 0.9199f,
	0.1462f, 0.5237f, 0.3014f, 0.4994f, 0.0278f, 0.4268f, 0.7238f, 0.5107f,
	0.1378f, 0.7303f, 0.7200f, 0.3819f, 0.2034f, 0.7157f, 0.5552f, 0.4887f,
	0.0871f, 0.3293f, 0.2892f, 0.4545f, 0.0088f, 0.1404f, 0.0275f, 0.0238f,
	0.0515f, 0.4494f, 0.7206f, 0.2893f, 0.6060f, 0.5785f, 0.4182f, 0.5528f,
	0.9118f, 0.8742f, 0.3859f, 0.6030f, 0.3495f, 0.4550f, 0.9875f, 0.6900f,
	0.6416f, 0.2337f, 0.7431f, 0.9788f, 0.6181f, 0.2464f, 0.4661f, 0.7621f,
	0.7020f, 0.8203f, 0.8869f, 0.2145f, 0.7724f, 0.6093f, 0.6692f, 0.9686f,
	0.5609f, 0.0310f, 0.2248f, 0.2950f, 0.2365f, 0.1347f, 0.2342f, 0.1668f,
	0.3378f, 0.4330f, 0.2775f, 0.9901f, 0.7053f, 0.7266f, 0.4840f, 0.2820f,
	0.5733f, 0.4555f, 0.6049f, 0.0770f, 0.4760f, 0.6060f, 0.4159f, 0.3427f,
	0.1234f, 0.7062f, 0.8569f, 0.1878f, 0.9057f, 0.9399f, 0.8139f, 0.1407f,
	0.1794f, 0.9123f, 0.9493f, 0.2827f, 0.9934f, 0.0952f, 0.4879f, 0.5160f,
	0.4118f, 0.4873f, 0.3642f, 0.7470f, 0.0866f, 0.5172f, 0.6365f, 0.2676f,
	0.2407f, 0.7223f, 0.5761f, 0.1143f, 0.7137f, 0.2342f, 0.3353f, 0.6880f,
	0.2296f, 0.6023f, 0.6027f, 0.4138f, 0.5408f, 0.9859f, 0.1503f, 0.7238f,
	0.6054f, 0.2477f, 0.6804f, 0.1432f, 0.4540f, 0.9776f, 0.8762f, 0.7607f,
	0.9025f, 0.9807f, 0.0652f, 0.8661f, 0.7663f, 0.2586f, 0.3994f, 0.0335f,
	0.7328f, 0.0166f, 0.9589f, 0.4348f, 0.5493f, 0.7269f, 0.6867f, 0.6614f,
	0.6800f, 0.7804f, 0.5591f, 0.8381f, 0.0910f, 0.7573f, 0.8985f, 0.3083f,
	0.3188f, 0.8481f, 0.2356f, 0.6736f, 0.4770f, 0.4560f, 0.6266f, 0.4677f
};

#define WIND_DAMP_INTERVAL 50
#define WIND_GUST_TIME 2500.0
#define WIND_GUST_DECAY (1.0 / WIND_GUST_TIME)

int		lastSSUpdateTime = 0;
float	curWindSpeed=0;
float	curWindGust=5;
float	curWeatherAmount=1;
vec3_t	curWindBlowVect={0,0,0}, targetWindBlowVect={0,0,0};
vec3_t	curWindGrassDir={0,0,0}, targetWindGrassDir={0,0,0};
int		totalsurfsprites=0, sssurfaces=0;

qboolean curWindPointActive=qfalse;
float curWindPointForce = 0;
vec3_t curWindPoint;
int nextGustTime=0;
float gustLeft=0;

qboolean standardfovinitialized=qfalse;
float	standardfovx = 90, standardscalex = 1.0;
float	rangescalefactor=1.0;

vec3_t  ssrightvectors[4];
vec3_t  ssfwdvector;
int		rightvectorcount;

trRefEntity_t *ssLastEntityDrawn=NULL;
vec3_t	ssViewOrigin, ssViewRight, ssViewUp;


static void R_SurfaceSpriteFrameUpdate(void)
{
	float dtime, dampfactor;	// Time since last update and damping time for wind changes
	float ratio;
	vec3_t ang, diff, retwindvec;
	float targetspeed;
	vec3_t up={0,0,1};

	if (backEnd.refdef.time == lastSSUpdateTime)
		return;

	if (backEnd.refdef.time < lastSSUpdateTime)
	{	// Time is BEFORE the last update time, so reset everything.
		curWindGust = 5;
		curWindSpeed = r_windSpeed->value;
		nextGustTime = 0;
		gustLeft = 0;
	}

	// Reset the last entity drawn, since this is a new frame.
	ssLastEntityDrawn = NULL;

	// Adjust for an FOV.  If things look twice as wide on the screen, pretend the shaders have twice the range.
	// ASSUMPTION HERE IS THAT "standard" fov is the first one rendered.

	if (!standardfovinitialized)
	{	// This isn't initialized yet.
		if (backEnd.refdef.fov_x > 50 && backEnd.refdef.fov_x < 135)		// I don't consider anything below 50 or above 135 to be "normal".
		{
			standardfovx = backEnd.refdef.fov_x;
			standardscalex = tan(standardfovx * 0.5 * (M_PI/180.0f));
			standardfovinitialized = qtrue;
		}
		else
		{
			standardfovx = 90;
			standardscalex = tan(standardfovx * 0.5 * (M_PI/180.0f));
		}
		rangescalefactor = 1.0;		// Don't multiply the shader range by anything.
	}
	else if (standardfovx == backEnd.refdef.fov_x)
	{	// This is the standard FOV (or higher), don't multiply the shader range.
		rangescalefactor = 1.0;
	}
	else
	{	// We are using a non-standard FOV.  We need to multiply the range of the shader by a scale factor.
		if (backEnd.refdef.fov_x > 135)
		{
			rangescalefactor = standardscalex / tan(135.0f * 0.5f * (M_PI/180.0f));
		}
		else
		{
			rangescalefactor = standardscalex / tan(backEnd.refdef.fov_x * 0.5 * (M_PI/180.0f));
		}
	}

	// Create a set of four right vectors so that vertical sprites aren't always facing the same way.
	// First generate a HORIZONTAL forward vector (important).
	CrossProduct(ssViewRight, up, ssfwdvector);

	// Right Zero has a nudge forward (10 degrees).
	VectorScale(ssViewRight, 0.985f, ssrightvectors[0]);
	VectorMA(ssrightvectors[0], 0.174f, ssfwdvector, ssrightvectors[0]);

	// Right One has a big nudge back (30 degrees).
	VectorScale(ssViewRight, 0.866f, ssrightvectors[1]);
	VectorMA(ssrightvectors[1], -0.5f, ssfwdvector, ssrightvectors[1]);


	// Right two has a big nudge forward (30 degrees).
	VectorScale(ssViewRight, 0.866f, ssrightvectors[2]);
	VectorMA(ssrightvectors[2], 0.5f, ssfwdvector, ssrightvectors[2]);


	// Right three has a nudge back (10 degrees).
	VectorScale(ssViewRight, 0.985f, ssrightvectors[3]);
	VectorMA(ssrightvectors[3], -0.174f, ssfwdvector, ssrightvectors[3]);


	// Update the wind.
	// If it is raining, get the windspeed from the rain system rather than the cvar
	if (R_IsRaining() || R_IsPuffing())
	{
		curWeatherAmount = 1.0;
	}
	else
	{
		curWeatherAmount = r_surfaceWeather->value;
	}

	if (R_GetWindSpeed(targetspeed))
	{	// We successfully got a speed from the rain system.
		// Set the windgust to 5, since that looks pretty good.
		targetspeed *= 0.3f;
		if (targetspeed >= 1.0)
		{
			curWindGust = 300/targetspeed;
		}
		else
		{
			curWindGust = 0;
		}
	}
	else
	{	// Use the cvar.
		targetspeed = r_windSpeed->value;	// Minimum gust delay, in seconds.
		curWindGust = r_windGust->value;
	}

	if (targetspeed > 0 && curWindGust)
	{
		if (gustLeft > 0)
		{	// We are gusting
			// Add an amount to the target wind speed
			targetspeed *= 1.0 + gustLeft;

			gustLeft -= (float)(backEnd.refdef.time - lastSSUpdateTime)*WIND_GUST_DECAY;
			if (gustLeft <= 0)
			{
				nextGustTime = backEnd.refdef.time + (curWindGust*1000)*flrand(1.0f,4.0f);
			}
		}
		else if (backEnd.refdef.time >= nextGustTime)
		{	// See if there is another right now
			// Gust next time, mano
			gustLeft = flrand(0.75f,1.5f);
		}
	}

	// See if there is a weather system that will tell us a windspeed.
	if (R_GetWindVector(retwindvec))
	{
		retwindvec[2]=0;
		VectorScale(retwindvec, -1.0f, retwindvec);
		vectoangles(retwindvec, ang);
	}
	else
	{	// Calculate the target wind vector based off cvars
		ang[YAW] = r_windAngle->value;
	}

	ang[PITCH] = -90.0 + targetspeed;
	if (ang[PITCH]>-45.0)
	{
		ang[PITCH] = -45.0;
	}
	ang[ROLL] = 0;

	if (targetspeed>0)
	{
//		ang[YAW] += cos(tr.refdef.time*0.01+flrand(-1.0,1.0))*targetspeed*0.5;
//		ang[PITCH] += sin(tr.refdef.time*0.01+flrand(-1.0,1.0))*targetspeed*0.5;
	}

	// Get the grass wind vector first
	AngleVectors(ang, targetWindGrassDir, NULL, NULL);
	targetWindGrassDir[2]-=1.0;
//		VectorScale(targetWindGrassDir, targetspeed, targetWindGrassDir);

	// Now get the general wind vector (no pitch)
	ang[PITCH]=0;
	AngleVectors(ang, targetWindBlowVect, NULL, NULL);

	// Start calculating a smoothing factor so wind doesn't change abruptly between speeds.
	dampfactor = 1.0-r_windDampFactor->value;	// We must exponent the amount LEFT rather than the amount bled off
	dtime = (float)(backEnd.refdef.time - lastSSUpdateTime) * (1.0/(float)WIND_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

	// Note that since there are a finite number of "practical" delta millisecond values possible,
	// the ratio should be initialized into a chart ultimately.
	ratio = pow(dampfactor, dtime);

	// Apply this ratio to the windspeed...
	curWindSpeed = targetspeed - (ratio * (targetspeed-curWindSpeed));

	// Use the curWindSpeed to calculate the final target wind vector (with speed)
	VectorScale(targetWindBlowVect, curWindSpeed, targetWindBlowVect);
	VectorSubtract(targetWindBlowVect, curWindBlowVect, diff);
	VectorMA(targetWindBlowVect, -ratio, diff, curWindBlowVect);

	// Update the grass vector now
	VectorSubtract(targetWindGrassDir, curWindGrassDir, diff);
	VectorMA(targetWindGrassDir, -ratio, diff, curWindGrassDir);

	lastSSUpdateTime = backEnd.refdef.time;

	curWindPointForce = r_windPointForce->value - (ratio * (r_windPointForce->value - curWindPointForce));
	if (curWindPointForce < 0.01)
	{
		curWindPointActive = qfalse;
	}
	else
	{
		curWindPointActive = qtrue;
		curWindPoint[0] = r_windPointX->value;
		curWindPoint[1] = r_windPointY->value;
		curWindPoint[2] = 0;
	}

	if (r_surfaceSprites->integer >= 2)
	{
		ri.Printf( PRINT_ALL, "Surfacesprites Drawn: %d, on %d surfaces\n", totalsurfsprites, sssurfaces);
	}

	totalsurfsprites=0;
	sssurfaces=0;
}



/////////////////////////////////////////////
// Surface sprite calculation and drawing.
/////////////////////////////////////////////

#define FADE_RANGE			250.0
#define WINDPOINT_RADIUS	750.0

float SSVertAlpha[SHADER_MAX_VERTEXES];
float SSVertWindForce[SHADER_MAX_VERTEXES];
vec2_t SSVertWindDir[SHADER_MAX_VERTEXES];

qboolean SSAdditiveTransparency=qfalse;
qboolean SSUsingFog=qfalse;


/////////////////////////////////////////////
// Vertical surface sprites

static void RB_VerticalSurfaceSprite(vec3_t loc, float width, float height, byte light,
										byte alpha, float wind, float windidle, vec2_t fog, int hangdown, vec2_t skew, bool flattened)
{
	vec3_t loc2, right;
	float angle;
	float windsway;
	float points[16];
	color4ub_t color;

	angle = ((loc[0]+loc[1])*0.02+(tr.refdef.time*0.0015));

	if (windidle>0.0)
	{
		windsway = (height*windidle*0.075);
		loc2[0] = loc[0]+skew[0]+cos(angle)*windsway;
		loc2[1] = loc[1]+skew[1]+sin(angle)*windsway;

		if (hangdown)
		{
			loc2[2] = loc[2]-height;
		}
		else
		{
			loc2[2] = loc[2]+height;
		}
	}
	else
	{
		loc2[0] = loc[0]+skew[0];
		loc2[1] = loc[1]+skew[1];
		if (hangdown)
		{
			loc2[2] = loc[2]-height;
		}
		else
		{
			loc2[2] = loc[2]+height;
		}
	}

	if (wind>0.0 && curWindSpeed > 0.001)
	{
		windsway = (height*wind*0.075);

		// Add the angle
		VectorMA(loc2, height*wind, curWindGrassDir, loc2);
		// Bob up and down
		if (curWindSpeed < 40.0)
		{
			windsway *= curWindSpeed*(1.0/100.0);
		}
		else
		{
			windsway *= 0.4f;
		}
		loc2[2] += sin(angle*2.5)*windsway;
	}

	if ( flattened )
	{
		right[0] = sin( DEG2RAD( loc[0] ) ) * width;
		right[1] = cos( DEG2RAD( loc[0] ) ) * height;
		right[2] = 0.0f;
	}
	else
	{
		VectorScale(ssrightvectors[rightvectorcount], width*0.5, right);
	}

	color[0]=light;
	color[1]=light;
	color[2]=light;
	color[3]=alpha;

	// Bottom right
//	VectorAdd(loc, right, point);
	points[0] = loc[0] + right[0];
	points[1] = loc[1] + right[1];
	points[2] = loc[2] + right[2];
	points[3] = 0;

	// Top right
//	VectorAdd(loc2, right, point);
	points[4] = loc2[0] + right[0];
	points[5] = loc2[1] + right[1];
	points[6] = loc2[2] + right[2];
	points[7] = 0;

	// Top left
//	VectorSubtract(loc2, right, point);
	points[8] = loc2[0] - right[0] + ssfwdvector[0] * width * 0.2;
	points[9] = loc2[1] - right[1] + ssfwdvector[1] * width * 0.2;
	points[10] = loc2[2] - right[2];
	points[11] = 0;

	// Bottom left
//	VectorSubtract(loc, right, point);
	points[12] = loc[0] - right[0];
	points[13] = loc[1] - right[1];
	points[14] = loc[2] - right[2];
	points[15] = 0;

	// Add the sprite to the render list.
	SQuickSprite.Add(points, color, fog);
}

static void RB_VerticalSurfaceSpriteWindPoint(vec3_t loc, float width, float height, byte light,
												byte alpha, float wind, float windidle, vec2_t fog,
												int hangdown, vec2_t skew, vec2_t winddiff, float windforce, bool flattened)
{
	vec3_t loc2, right;
	float angle;
	float windsway;
	float points[16];
	color4ub_t color;

	if (windforce > 1)
		windforce = 1;

//	wind += 1.0-windforce;

	angle = (loc[0]+loc[1])*0.02+(tr.refdef.time*0.0015);

	if (curWindSpeed <80.0)
	{
		windsway = (height*windidle*0.1)*(1.0+windforce);
		loc2[0] = loc[0]+skew[0]+cos(angle)*windsway;
		loc2[1] = loc[1]+skew[1]+sin(angle)*windsway;
	}
	else
	{
		loc2[0] = loc[0]+skew[0];
		loc2[1] = loc[1]+skew[1];
	}
	if (hangdown)
	{
		loc2[2] = loc[2]-height;
	}
	else
	{
		loc2[2] = loc[2]+height;
	}

	if (curWindSpeed > 0.001)
	{
		// Add the angle
		VectorMA(loc2, height*wind, curWindGrassDir, loc2);
	}

	loc2[0] += height*winddiff[0]*windforce;
	loc2[1] += height*winddiff[1]*windforce;
	loc2[2] -= height*windforce*(0.75 + 0.15*sin((tr.refdef.time + 500*windforce)*0.01));

	if ( flattened )
	{
		right[0] = sin( DEG2RAD( loc[0] ) ) * width;
		right[1] = cos( DEG2RAD( loc[0] ) ) * height;
		right[2] = 0.0f;
	}
	else
	{
		VectorScale(ssrightvectors[rightvectorcount], width*0.5, right);
	}


	color[0]=light;
	color[1]=light;
	color[2]=light;
	color[3]=alpha;

	// Bottom right
//	VectorAdd(loc, right, point);
	points[0] = loc[0] + right[0];
	points[1] = loc[1] + right[1];
	points[2] = loc[2] + right[2];
	points[3] = 0;

	// Top right
//	VectorAdd(loc2, right, point);
	points[4] = loc2[0] + right[0];
	points[5] = loc2[1] + right[1];
	points[6] = loc2[2] + right[2];
	points[7] = 0;

	// Top left
//	VectorSubtract(loc2, right, point);
	points[8] = loc2[0] - right[0] + ssfwdvector[0] * width * 0.15;
	points[9] = loc2[1] - right[1] + ssfwdvector[1] * width * 0.15;
	points[10] = loc2[2] - right[2];
	points[11] = 0;

	// Bottom left
//	VectorSubtract(loc, right, point);
	points[12] = loc[0] - right[0];
	points[13] = loc[1] - right[1];
	points[14] = loc[2] - right[2];
	points[15] = 0;

	// Add the sprite to the render list.
	SQuickSprite.Add(points, color, fog);
}

static void RB_DrawVerticalSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input)
{
	int curindex, curvert;
 	vec3_t dist;
	float triarea;
	vec2_t vec1to2, vec1to3;

	vec3_t v1,v2,v3;
	float a1,a2,a3;
	float l1,l2,l3;
	vec2_t fog1, fog2, fog3;
	vec2_t winddiff1, winddiff2, winddiff3;
	float  windforce1, windforce2, windforce3;

	float posi, posj;
	float step;
	float fa,fb,fc;

	vec3_t curpoint;
	float width, height;
	float alpha, alphapos, thisspritesfadestart, light;

	byte randomindex2;

	vec2_t skew={0,0};
	vec2_t fogv;
	vec2_t winddiffv;
	float windforce=0;
	qboolean usewindpoint = (qboolean) !! (curWindPointActive && stage->ss->wind > 0);

	float cutdist=stage->ss->fadeMax*rangescalefactor, cutdist2=cutdist*cutdist;
	float fadedist=stage->ss->fadeDist*rangescalefactor, fadedist2=fadedist*fadedist;

	assert(cutdist2 != fadedist2);
	float inv_fadediff = 1.0/(cutdist2-fadedist2);

	// The faderange is the fraction amount it takes for these sprites to fade out, assuming an ideal fade range of 250
	float faderange = FADE_RANGE/(cutdist-fadedist);

	if (faderange > 1.0)
	{	// Don't want to force a new fade_rand
		faderange = 1.0;
	}

	// Quickly calc all the alphas and windstuff for each vertex
	for (curvert=0; curvert<input->numVertexes; curvert++)
	{
		VectorSubtract(ssViewOrigin, input->xyz[curvert], dist);
		SSVertAlpha[curvert] = 1.0 - (VectorLengthSquared(dist) - fadedist2) * inv_fadediff;
	}

	// Wind only needs initialization once per tess.
	if (usewindpoint && !tess.SSInitializedWind)
	{
		for (curvert=0; curvert<input->numVertexes;curvert++)
		{	// Calc wind at each point
			dist[0]=input->xyz[curvert][0] - curWindPoint[0];
			dist[1]=input->xyz[curvert][1] - curWindPoint[1];
			step = (dist[0]*dist[0] + dist[1]*dist[1]);	// dist squared

			if (step >= (float)(WINDPOINT_RADIUS*WINDPOINT_RADIUS))
			{	// No wind
				SSVertWindDir[curvert][0] = 0;
				SSVertWindDir[curvert][1] = 0;
				SSVertWindForce[curvert]=0;		// Should be < 1
			}
			else
			{
				if (step<1)
				{	// Don't want to divide by zero
					SSVertWindDir[curvert][0] = 0;
					SSVertWindDir[curvert][1] = 0;
					SSVertWindForce[curvert] = curWindPointForce * stage->ss->wind;
				}
				else
				{
					step = Q_rsqrt(step);		// Equals 1 over the distance.
					SSVertWindDir[curvert][0] = dist[0] * step;
					SSVertWindDir[curvert][1] = dist[1] * step;
					step = 1.0 - (1.0 / (step * WINDPOINT_RADIUS));	// 1- (dist/maxradius) = a scale from 0 to 1 linearly dropping off
					SSVertWindForce[curvert] = curWindPointForce * stage->ss->wind * step;	// *step means divide by the distance.
				}
			}
		}
		tess.SSInitializedWind = qtrue;
	}

	for (curindex=0; curindex<input->numIndexes-2; curindex+=3)
	{
		curvert = input->indexes[curindex];
		VectorCopy(input->xyz[curvert], v1);
		if (stage->ss->facing)
		{	// Hang down
			if (input->normal[curvert][2] > -0.5)
			{
				continue;
			}
		}
		else
		{	// Point up
			if (input->normal[curvert][2] < 0.5)
			{
				continue;
			}
		}
		l1 = input->vertexColors[curvert][2];
		a1 = SSVertAlpha[curvert];
		fog1[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog1[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);
		winddiff1[0] = SSVertWindDir[curvert][0];
		winddiff1[1] = SSVertWindDir[curvert][1];
		windforce1 = SSVertWindForce[curvert];

		curvert = input->indexes[curindex+1];
		VectorCopy(input->xyz[curvert], v2);
		if (stage->ss->facing)
		{	// Hang down
			if (input->normal[curvert][2] > -0.5)
			{
				continue;
			}
		}
		else
		{	// Point up
			if (input->normal[curvert][2] < 0.5)
			{
				continue;
			}
		}
		l2 = input->vertexColors[curvert][2];
		a2 = SSVertAlpha[curvert];
		fog2[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog2[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);
		winddiff2[0] = SSVertWindDir[curvert][0];
		winddiff2[1] = SSVertWindDir[curvert][1];
		windforce2 = SSVertWindForce[curvert];

		curvert = input->indexes[curindex+2];
		VectorCopy(input->xyz[curvert], v3);
		if (stage->ss->facing)
		{	// Hang down
			if (input->normal[curvert][2] > -0.5)
			{
				continue;
			}
		}
		else
		{	// Point up
			if (input->normal[curvert][2] < 0.5)
			{
				continue;
			}
		}
		l3 = input->vertexColors[curvert][2];
		a3 = SSVertAlpha[curvert];
		fog3[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog3[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);
		winddiff3[0] = SSVertWindDir[curvert][0];
		winddiff3[1] = SSVertWindDir[curvert][1];
		windforce3 = SSVertWindForce[curvert];

		if (a1 <= 0.0 && a2 <= 0.0 && a3 <= 0.0)
		{
			continue;
		}

		// Find the area in order to calculate the stepsize
		vec1to2[0] = v2[0] - v1[0];
		vec1to2[1] = v2[1] - v1[1];
		vec1to3[0] = v3[0] - v1[0];
		vec1to3[1] = v3[1] - v1[1];

		// Now get the cross product of this sum.
		triarea = vec1to3[0]*vec1to2[1] - vec1to3[1]*vec1to2[0];
		triarea=fabs(triarea);
		if (triarea <= 1.0)
		{	// Insanely small abhorrent triangle.
			continue;
		}
		step = stage->ss->density * Q_rsqrt(triarea);

		randomindex = (byte)(v1[0]+v1[1]+v2[0]+v2[1]+v3[0]+v3[1]);
		randominterval = (byte)(v1[0]+v2[1]+v3[2])|0x03;	// Make sure the interval is at least 3, and always odd
		rightvectorcount = 0;

		for (posi=0; posi<1.0; posi+=step)
		{
			for (posj=0; posj<(1.0-posi); posj+=step)
			{
				fa=posi+randomchart[randomindex]*step;
				randomindex += randominterval;

				fb=posj+randomchart[randomindex]*step;
				randomindex += randominterval;

				rightvectorcount=(rightvectorcount+1)&3;

				if (fa>1.0)
					continue;

				if (fb>(1.0-fa))
					continue;

				fc = 1.0-fa-fb;

				// total alpha, minus random factor so some things fade out sooner.
				alphapos = a1*fa + a2*fb + a3*fc;

				// Note that the alpha at this point is a value from 1.0 to 0.0, but represents when to START fading
				thisspritesfadestart = faderange + (1.0-faderange) * randomchart[randomindex];
				randomindex += randominterval;

				// Find where the alpha is relative to the fadestart, and calc the real alpha to draw at.
				alpha = 1.0 - ((thisspritesfadestart-alphapos)/faderange);
				if (alpha > 0.0)
				{
					if (alpha > 1.0)
						alpha=1.0;

					if (SSUsingFog)
					{
						fogv[0] = fog1[0]*fa + fog2[0]*fb + fog3[0]*fc;
						fogv[1] = fog1[1]*fa + fog2[1]*fb + fog3[1]*fc;
					}

					if (usewindpoint)
					{
						winddiffv[0] = winddiff1[0]*fa + winddiff2[0]*fb + winddiff3[0]*fc;
						winddiffv[1] = winddiff1[1]*fa + winddiff2[1]*fb + winddiff3[1]*fc;
						windforce = windforce1*fa + windforce2*fb + windforce3*fc;
					}

					VectorScale(v1, fa, curpoint);
					VectorMA(curpoint, fb, v2, curpoint);
					VectorMA(curpoint, fc, v3, curpoint);

					light = l1*fa + l2*fb + l3*fc;
					if (SSAdditiveTransparency)
					{	// Additive transparency, scale light value
//						light *= alpha;
						light = (128 + (light*0.5))*alpha;
						alpha = 1.0;
					}

					randomindex2 = randomindex;
					width = stage->ss->width*(1.0 + (stage->ss->variance[0]*randomchart[randomindex2]));
					height = stage->ss->height*(1.0 + (stage->ss->variance[1]*randomchart[randomindex2++]));
					if (randomchart[randomindex2++]>0.5)
					{
						width = -width;
					}
					if (stage->ss->fadeScale!=0 && alphapos < 1.0)
					{
						width *= 1.0 + (stage->ss->fadeScale*(1.0-alphapos));
					}

					if (stage->ss->vertSkew != 0)
					{	// flrand(-vertskew, vertskew)
						skew[0] = height * ((stage->ss->vertSkew*2.0f*randomchart[randomindex2++])-stage->ss->vertSkew);
						skew[1] = height * ((stage->ss->vertSkew*2.0f*randomchart[randomindex2++])-stage->ss->vertSkew);
					}

					if (usewindpoint && windforce > 0 && stage->ss->wind > 0.0)
					{
						if (SSUsingFog)
						{
							RB_VerticalSurfaceSpriteWindPoint(curpoint, width, height, (byte)light, (byte)(alpha*255.0),
										stage->ss->wind, stage->ss->windIdle, fogv, stage->ss->facing, skew,
										winddiffv, windforce, SURFSPRITE_FLATTENED == stage->ss->surfaceSpriteType);
						}
						else
						{
							RB_VerticalSurfaceSpriteWindPoint(curpoint, width, height, (byte)light, (byte)(alpha*255.0),
										stage->ss->wind, stage->ss->windIdle, NULL, stage->ss->facing, skew,
										winddiffv, windforce, SURFSPRITE_FLATTENED == stage->ss->surfaceSpriteType);
						}
					}
					else
					{
						if (SSUsingFog)
						{
							RB_VerticalSurfaceSprite(curpoint, width, height, (byte)light, (byte)(alpha*255.0),
										stage->ss->wind, stage->ss->windIdle, fogv, stage->ss->facing, skew, SURFSPRITE_FLATTENED == stage->ss->surfaceSpriteType);
						}
						else
						{
							RB_VerticalSurfaceSprite(curpoint, width, height, (byte)light, (byte)(alpha*255.0),
										stage->ss->wind, stage->ss->windIdle, NULL, stage->ss->facing, skew, SURFSPRITE_FLATTENED == stage->ss->surfaceSpriteType);
						}
					}

					totalsurfsprites++;
				}
			}
		}
	}
}


/////////////////////////////////////////////
// Oriented surface sprites

static void RB_OrientedSurfaceSprite(vec3_t loc, float width, float height, byte light, byte alpha, vec2_t fog, int faceup)
{
	vec3_t loc2, right;
	float points[16];
	color4ub_t color;

	color[0]=light;
	color[1]=light;
	color[2]=light;
	color[3]=alpha;

	if (faceup)
	{
		width *= 0.5;
		height *= 0.5;

		// Bottom right
	//	VectorAdd(loc, right, point);
		points[0] = loc[0] + width;
		points[1] = loc[1] - width;
		points[2] = loc[2] + 1.0;
		points[3] = 0;

		// Top right
	//	VectorAdd(loc, right, point);
		points[4] = loc[0] + width;
		points[5] = loc[1] + width;
		points[6] = loc[2] + 1.0;
		points[7] = 0;

		// Top left
	//	VectorSubtract(loc, right, point);
		points[8] = loc[0] - width;
		points[9] = loc[1] + width;
		points[10] = loc[2] + 1.0;
		points[11] = 0;

		// Bottom left
	//	VectorSubtract(loc, right, point);
		points[12] = loc[0] - width;
		points[13] = loc[1] - width;
		points[14] = loc[2] + 1.0;
		points[15] = 0;
	}
	else
	{
		VectorMA(loc, height, ssViewUp, loc2);
		VectorScale(ssViewRight, width*0.5, right);

		// Bottom right
	//	VectorAdd(loc, right, point);
		points[0] = loc[0] + right[0];
		points[1] = loc[1] + right[1];
		points[2] = loc[2] + right[2];
		points[3] = 0;

		// Top right
	//	VectorAdd(loc2, right, point);
		points[4] = loc2[0] + right[0];
		points[5] = loc2[1] + right[1];
		points[6] = loc2[2] + right[2];
		points[7] = 0;

		// Top left
	//	VectorSubtract(loc2, right, point);
		points[8] = loc2[0] - right[0];
		points[9] = loc2[1] - right[1];
		points[10] = loc2[2] - right[2];
		points[11] = 0;

		// Bottom left
	//	VectorSubtract(loc, right, point);
		points[12] = loc[0] - right[0];
		points[13] = loc[1] - right[1];
		points[14] = loc[2] - right[2];
		points[15] = 0;
	}

	// Add the sprite to the render list.
	SQuickSprite.Add(points, color, fog);
}

static void RB_DrawOrientedSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input)
{
	int curindex, curvert;
 	vec3_t dist;
	float triarea, minnormal;
	vec2_t vec1to2, vec1to3;

	vec3_t v1,v2,v3;
	float a1,a2,a3;
	float l1,l2,l3;
	vec2_t fog1, fog2, fog3;

	float posi, posj;
	float step;
	float fa,fb,fc;

	vec3_t curpoint;
	float width, height;
	float alpha, alphapos, thisspritesfadestart, light;
	byte randomindex2;
	vec2_t fogv;

	float cutdist=stage->ss->fadeMax*rangescalefactor, cutdist2=cutdist*cutdist;
	float fadedist=stage->ss->fadeDist*rangescalefactor, fadedist2=fadedist*fadedist;

	assert(cutdist2 != fadedist2);
	float inv_fadediff = 1.0/(cutdist2-fadedist2);

	// The faderange is the fraction amount it takes for these sprites to fade out, assuming an ideal fade range of 250
	float faderange = FADE_RANGE/(cutdist-fadedist);

	if (faderange > 1.0)
	{	// Don't want to force a new fade_rand
		faderange = 1.0;
	}

	if (stage->ss->facing)
	{	// Faceup sprite.
		minnormal = 0.99f;
	}
	else
	{	// Normal oriented sprite
		minnormal = 0.5f;
	}

	// Quickly calc all the alphas for each vertex
	for (curvert=0; curvert<input->numVertexes; curvert++)
	{
		// Calc alpha at each point
		VectorSubtract(ssViewOrigin, input->xyz[curvert], dist);
		SSVertAlpha[curvert] = 1.0 - (VectorLengthSquared(dist) - fadedist2) * inv_fadediff;
	}

	for (curindex=0; curindex<input->numIndexes-2; curindex+=3)
	{
		curvert = input->indexes[curindex];
		VectorCopy(input->xyz[curvert], v1);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l1 = input->vertexColors[curvert][2];
		a1 = SSVertAlpha[curvert];
		fog1[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog1[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);

		curvert = input->indexes[curindex+1];
		VectorCopy(input->xyz[curvert], v2);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l2 = input->vertexColors[curvert][2];
		a2 = SSVertAlpha[curvert];
		fog2[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog2[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);

		curvert = input->indexes[curindex+2];
		VectorCopy(input->xyz[curvert], v3);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l3 = input->vertexColors[curvert][2];
		a3 = SSVertAlpha[curvert];
		fog3[0] = *((float *)(tess.svars.texcoords[0])+(curvert<<1));
		fog3[1] = *((float *)(tess.svars.texcoords[0])+(curvert<<1)+1);

		if (a1 <= 0.0 && a2 <= 0.0 && a3 <= 0.0)
		{
			continue;
		}

		// Find the area in order to calculate the stepsize
		vec1to2[0] = v2[0] - v1[0];
		vec1to2[1] = v2[1] - v1[1];
		vec1to3[0] = v3[0] - v1[0];
		vec1to3[1] = v3[1] - v1[1];

		// Now get the cross product of this sum.
		triarea = vec1to3[0]*vec1to2[1] - vec1to3[1]*vec1to2[0];
		triarea=fabs(triarea);
		if (triarea <= 1.0)
		{	// Insanely small abhorrent triangle.
			continue;
		}
		step = stage->ss->density * Q_rsqrt(triarea);

		randomindex = (byte)(v1[0]+v1[1]+v2[0]+v2[1]+v3[0]+v3[1]);
		randominterval = (byte)(v1[0]+v2[1]+v3[2])|0x03;	// Make sure the interval is at least 3, and always odd

		for (posi=0; posi<1.0; posi+=step)
		{
			for (posj=0; posj<(1.0-posi); posj+=step)
			{
				fa=posi+randomchart[randomindex]*step;
				randomindex += randominterval;
				if (fa>1.0)
					continue;

				fb=posj+randomchart[randomindex]*step;
				randomindex += randominterval;
				if (fb>(1.0-fa))
					continue;

				fc = 1.0-fa-fb;

				// total alpha, minus random factor so some things fade out sooner.
				alphapos = a1*fa + a2*fb + a3*fc;

				// Note that the alpha at this point is a value from 1.0 to 0.0, but represents when to START fading
				thisspritesfadestart = faderange + (1.0-faderange) * randomchart[randomindex];
				randomindex += randominterval;

				// Find where the alpha is relative to the fadestart, and calc the real alpha to draw at.
				alpha = 1.0 - ((thisspritesfadestart-alphapos)/faderange);

				randomindex += randominterval;
				if (alpha > 0.0)
				{
					if (alpha > 1.0)
						alpha=1.0;

					if (SSUsingFog)
					{
						fogv[0] = fog1[0]*fa + fog2[0]*fb + fog3[0]*fc;
						fogv[1] = fog1[1]*fa + fog2[1]*fb + fog3[1]*fc;
					}

					VectorScale(v1, fa, curpoint);
					VectorMA(curpoint, fb, v2, curpoint);
					VectorMA(curpoint, fc, v3, curpoint);

					light = l1*fa + l2*fb + l3*fc;
					if (SSAdditiveTransparency)
					{	// Additive transparency, scale light value
//						light *= alpha;
						light = (128 + (light*0.5))*alpha;
						alpha = 1.0;
					}

					randomindex2 = randomindex;
					width = stage->ss->width*(1.0 + (stage->ss->variance[0]*randomchart[randomindex2]));
					height = stage->ss->height*(1.0 + (stage->ss->variance[1]*randomchart[randomindex2++]));
					if (randomchart[randomindex2++]>0.5)
					{
						width = -width;
					}
					if (stage->ss->fadeScale!=0 && alphapos < 1.0)
					{
						width *= 1.0 + (stage->ss->fadeScale*(1.0-alphapos));
					}

					if (SSUsingFog)
					{
						RB_OrientedSurfaceSprite(curpoint, width, height, (byte)light, (byte)(alpha*255.0), fogv, stage->ss->facing);
					}
					else
					{
						RB_OrientedSurfaceSprite(curpoint, width, height, (byte)light, (byte)(alpha*255.0), NULL, stage->ss->facing);
					}

					totalsurfsprites++;
				}
			}
		}
	}
}


/////////////////////////////////////////////
// Effect surface sprites

static void RB_EffectSurfaceSprite(vec3_t loc, float width, float height, byte light, byte alpha, float life, int faceup)
{
	vec3_t loc2, right;
	float points[16];
	color4ub_t color;

	color[0]=light;	//light;
	color[1]=light;	//light;
	color[2]=light;	//light;
	color[3]=alpha;	//alpha;

	if (faceup)
	{
		width *= 0.5;
		height *= 0.5;

		// Bottom right
	//	VectorAdd(loc, right, point);
		points[0] = loc[0] + width;
		points[1] = loc[1] - width;
		points[2] = loc[2] + 1.0;
		points[3] = 0;

		// Top right
	//	VectorAdd(loc, right, point);
		points[4] = loc[0] + width;
		points[5] = loc[1] + width;
		points[6] = loc[2] + 1.0;
		points[7] = 0;

		// Top left
	//	VectorSubtract(loc, right, point);
		points[8] = loc[0] - width;
		points[9] = loc[1] + width;
		points[10] = loc[2] + 1.0;
		points[11] = 0;

		// Bottom left
	//	VectorSubtract(loc, right, point);
		points[12] = loc[0] - width;
		points[13] = loc[1] - width;
		points[14] = loc[2] + 1.0;
		points[15] = 0;
	}
	else
	{
		VectorMA(loc, height, ssViewUp, loc2);
		VectorScale(ssViewRight, width*0.5, right);

		// Bottom right
	//	VectorAdd(loc, right, point);
		points[0] = loc[0] + right[0];
		points[1] = loc[1] + right[1];
		points[2] = loc[2] + right[2];
		points[3] = 0;

		// Top right
	//	VectorAdd(loc2, right, point);
		points[4] = loc2[0] + right[0];
		points[5] = loc2[1] + right[1];
		points[6] = loc2[2] + right[2];
		points[7] = 0;

		// Top left
	//	VectorSubtract(loc2, right, point);
		points[8] = loc2[0] - right[0];
		points[9] = loc2[1] - right[1];
		points[10] = loc2[2] - right[2];
		points[11] = 0;

		// Bottom left
	//	VectorSubtract(loc, right, point);
		points[12] = loc[0] - right[0];
		points[13] = loc[1] - right[1];
		points[14] = loc[2] - right[2];
		points[15] = 0;
	}

	// Add the sprite to the render list.
	SQuickSprite.Add(points, color, NULL);
}

static void RB_DrawEffectSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input)
{
	int curindex, curvert;
 	vec3_t dist;
	float triarea, minnormal;
	vec2_t vec1to2, vec1to3;

	vec3_t v1,v2,v3;
	float a1,a2,a3;
	float l1,l2,l3;

	float posi, posj;
	float step;
	float fa,fb,fc;
	float effecttime, effectpos;
	float density;

	vec3_t curpoint;
	float width, height;
	float alpha, alphapos, thisspritesfadestart, light;
	byte randomindex2;

	float cutdist=stage->ss->fadeMax*rangescalefactor, cutdist2=cutdist*cutdist;
	float fadedist=stage->ss->fadeDist*rangescalefactor, fadedist2=fadedist*fadedist;

	float fxalpha = stage->ss->fxAlphaEnd - stage->ss->fxAlphaStart;
	qboolean fadeinout=qfalse;

	assert(cutdist2 != fadedist2);
	float inv_fadediff = 1.0/(cutdist2-fadedist2);

	// The faderange is the fraction amount it takes for these sprites to fade out, assuming an ideal fade range of 250
	float faderange = FADE_RANGE/(cutdist-fadedist);
	if (faderange > 1.0f)
	{	// Don't want to force a new fade_rand
		faderange = 1.0f;
	}

	if (stage->ss->facing)
	{	// Faceup sprite.
		minnormal = 0.99f;
	}
	else
	{	// Normal oriented sprite
		minnormal = 0.5f;
	}

	// Make the object fade in.
	if (stage->ss->fxAlphaEnd < 0.05 && stage->ss->height >= 0.1 && stage->ss->width >= 0.1)
	{	// The sprite fades out, and it doesn't start at a pinpoint.  Let's fade it in.
		fadeinout=qtrue;
	}

	if (stage->ss->surfaceSpriteType == SURFSPRITE_WEATHERFX)
	{	// This effect is affected by weather settings.
		if (curWeatherAmount < 0.01)
		{	// Don't show these effects
			return;
		}
		else
		{
			density = stage->ss->density / curWeatherAmount;
		}
	}
	else
	{
		density = stage->ss->density;
	}

	// Quickly calc all the alphas for each vertex
	for (curvert=0; curvert<input->numVertexes; curvert++)
	{
		// Calc alpha at each point
		VectorSubtract(ssViewOrigin, input->xyz[curvert], dist);
		SSVertAlpha[curvert] = 1.0f - (VectorLengthSquared(dist) - fadedist2) * inv_fadediff;

	// Note this is the proper equation, but isn't used right now because it would be just a tad slower.
		// Formula for alpha is 1.0f - ((len-fade)/(cut-fade))
		// Which is equal to (1.0+fade/(cut-fade)) - (len/(cut-fade))
		// So mult=1/(cut-fade), and base=(1+fade*mult).
	//	SSVertAlpha[curvert] = fadebase - (VectorLength(dist) * fademult);

	}

	for (curindex=0; curindex<input->numIndexes-2; curindex+=3)
	{
		curvert = input->indexes[curindex];
		VectorCopy(input->xyz[curvert], v1);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l1 = input->vertexColors[curvert][2];
		a1 = SSVertAlpha[curvert];

		curvert = input->indexes[curindex+1];
		VectorCopy(input->xyz[curvert], v2);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l2 = input->vertexColors[curvert][2];
		a2 = SSVertAlpha[curvert];

		curvert = input->indexes[curindex+2];
		VectorCopy(input->xyz[curvert], v3);
		if (input->normal[curvert][2] < minnormal)
		{
			continue;
		}
		l3 = input->vertexColors[curvert][2];
		a3 = SSVertAlpha[curvert];

		if (a1 <= 0.0f && a2 <= 0.0f && a3 <= 0.0f)
		{
			continue;
		}

		// Find the area in order to calculate the stepsize
		vec1to2[0] = v2[0] - v1[0];
		vec1to2[1] = v2[1] - v1[1];
		vec1to3[0] = v3[0] - v1[0];
		vec1to3[1] = v3[1] - v1[1];

		// Now get the cross product of this sum.
		triarea = vec1to3[0]*vec1to2[1] - vec1to3[1]*vec1to2[0];
		triarea=fabs(triarea);
		if (triarea <= 1.0f)
		{	// Insanely small abhorrent triangle.
			continue;
		}
		step = density * Q_rsqrt(triarea);

		randomindex = (byte)(v1[0]+v1[1]+v2[0]+v2[1]+v3[0]+v3[1]);
		randominterval = (byte)(v1[0]+v2[1]+v3[2])|0x03;	// Make sure the interval is at least 3, and always odd

		for (posi=0; posi<1.0f; posi+=step)
		{
			for (posj=0; posj<(1.0-posi); posj+=step)
			{
				effecttime = (tr.refdef.time+10000.0*randomchart[randomindex])/stage->ss->fxDuration;
				effectpos = (float)effecttime - (int)effecttime;

				randomindex2 = randomindex+effecttime;
				randomindex += randominterval;
				fa=posi+randomchart[randomindex2++]*step;
				if (fa>1.0f)
					continue;

				fb=posj+randomchart[randomindex2++]*step;
				if (fb>(1.0-fa))
					continue;

				fc = 1.0-fa-fb;

				// total alpha, minus random factor so some things fade out sooner.
				alphapos = a1*fa + a2*fb + a3*fc;

				// Note that the alpha at this point is a value from 1.0f to 0.0, but represents when to START fading
				thisspritesfadestart = faderange + (1.0-faderange) * randomchart[randomindex2];
				randomindex2 += randominterval;

				// Find where the alpha is relative to the fadestart, and calc the real alpha to draw at.
				alpha = 1.0f - ((thisspritesfadestart-alphapos)/faderange);
				if (alpha > 0.0f)
				{
					if (alpha > 1.0f)
						alpha=1.0f;

					VectorScale(v1, fa, curpoint);
					VectorMA(curpoint, fb, v2, curpoint);
					VectorMA(curpoint, fc, v3, curpoint);

					light = l1*fa + l2*fb + l3*fc;
					randomindex2 = randomindex;
					width = stage->ss->width*(1.0f + (stage->ss->variance[0]*randomchart[randomindex2]));
					height = stage->ss->height*(1.0f + (stage->ss->variance[1]*randomchart[randomindex2++]));

					width = width + (effectpos*stage->ss->fxGrow[0]*width);
					height = height + (effectpos*stage->ss->fxGrow[1]*height);

					// If we want to fade in and out, that's different than a straight fade.
					if (fadeinout)
					{
						if (effectpos > 0.5)
						{	// Fade out
							alpha = alpha*(stage->ss->fxAlphaStart+(fxalpha*(effectpos-0.5)*2.0));
						}
						else
						{	// Fade in
							alpha = alpha*(stage->ss->fxAlphaStart+(fxalpha*(0.5-effectpos)*2.0));
						}
					}
					else
					{	// Normal fade
						alpha = alpha*(stage->ss->fxAlphaStart+(fxalpha*effectpos));
					}

					if (SSAdditiveTransparency)
					{	// Additive transparency, scale light value
//						light *= alpha;
						light = (128 + (light*0.5))*alpha;
						alpha = 1.0;
					}

					if (randomchart[randomindex2]>0.5f)
					{
						width = -width;
					}
					if (stage->ss->fadeScale!=0 && alphapos < 1.0f)
					{
						width *= 1.0f + (stage->ss->fadeScale*(1.0-alphapos));
					}

					if (stage->ss->wind>0.0f && curWindSpeed > 0.001)
					{
						vec3_t drawpoint;

						VectorMA(curpoint, effectpos*stage->ss->wind, curWindBlowVect, drawpoint);
						RB_EffectSurfaceSprite(drawpoint, width, height, (byte)light, (byte)(alpha*255.0f), stage->ss->fxDuration, stage->ss->facing);
					}
					else
					{
						RB_EffectSurfaceSprite(curpoint, width, height, (byte)light, (byte)(alpha*255.0f), stage->ss->fxDuration, stage->ss->facing);
					}

					totalsurfsprites++;
				}
			}
		}
	}
}

extern void R_WorldToLocal (vec3_t world, vec3_t localVec) ;
extern float preTransEntMatrix[16], invEntMatrix[16];
extern void R_InvertMatrix(float *sourcemat, float *destmat);

void RB_DrawSurfaceSprites( shaderStage_t *stage, shaderCommands_t *input)
{
	uint32_t	glbits=stage->stateBits;

	R_SurfaceSpriteFrameUpdate();

	//
	// Check fog
	//
	if ( tess.fogNum && tess.shader->fogPass && r_drawfog->value)
	{
		SSUsingFog = qtrue;
		SQuickSprite.StartGroup(&stage->bundle[0], glbits, tess.fogNum);
	}
	else
	{
		SSUsingFog = qfalse;
		SQuickSprite.StartGroup(&stage->bundle[0], glbits);
	}

	// Special provision in case the transparency is additive.
	if ((glbits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE))
	{	// Additive transparency, scale light value
		SSAdditiveTransparency=qtrue;
	}
	else
	{
		SSAdditiveTransparency=qfalse;
	}


	//Check if this is a new entity transformation (incl. world entity), and update the appropriate vectors if so.
	if (backEnd.currentEntity != ssLastEntityDrawn)
	{
		if (backEnd.currentEntity == &tr.worldEntity)
		{	// Drawing the world, so our job is dead-easy, in the viewparms
			VectorCopy(backEnd.viewParms.ori.origin, ssViewOrigin);
			VectorCopy(backEnd.viewParms.ori.axis[1], ssViewRight);
			VectorCopy(backEnd.viewParms.ori.axis[2], ssViewUp);
		}
		else
		{	// Drawing an entity, so we need to transform the viewparms to the model's coordinate system
//			R_WorldPointToEntity (backEnd.viewParms.ori.origin, ssViewOrigin);
			R_WorldNormalToEntity (backEnd.viewParms.ori.axis[1], ssViewRight);
			R_WorldNormalToEntity (backEnd.viewParms.ori.axis[2], ssViewUp);
			VectorCopy(backEnd.ori.viewOrigin, ssViewOrigin);
//			R_WorldToLocal(backEnd.viewParms.ori.axis[1], ssViewRight);
//			R_WorldToLocal(backEnd.viewParms.ori.axis[2], ssViewUp);
		}
		ssLastEntityDrawn = backEnd.currentEntity;
	}

	switch(stage->ss->surfaceSpriteType)
	{
	case SURFSPRITE_FLATTENED:
	case SURFSPRITE_VERTICAL:
		RB_DrawVerticalSurfaceSprites(stage, input);
		break;
	case SURFSPRITE_ORIENTED:
		RB_DrawOrientedSurfaceSprites(stage, input);
		break;
	case SURFSPRITE_EFFECT:
	case SURFSPRITE_WEATHERFX:
		RB_DrawEffectSurfaceSprites(stage, input);
		break;
	}

	SQuickSprite.EndGroup();

	sssurfaces++;
}

