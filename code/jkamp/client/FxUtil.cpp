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

#include "client.h"
#include "FxScheduler.h"

vec3_t	WHITE = {1.0f, 1.0f, 1.0f};

struct SEffectList
{
	CEffect *mEffect;
	int		mKillTime;
	bool	mPortal;
};

#define PI		3.14159f

SEffectList		effectList[MAX_EFFECTS];
SEffectList		*nextValidEffect;
SFxHelper		theFxHelper;

int				activeFx = 0;
int				drawnFx;
qboolean		fxInitialized = qfalse;

//-------------------------
// FX_Free
//
// Frees all FX
//-------------------------
bool FX_Free( bool templates )
{
	for ( int i = 0; i < MAX_EFFECTS; i++ )
	{
		if ( effectList[i].mEffect )
		{
			delete effectList[i].mEffect;
		}

		effectList[i].mEffect = 0;
	}

	activeFx = 0;

	theFxScheduler.Clean( templates );
	return true;
}

//-------------------------
// FX_Stop
//
// Frees all active FX but leaves the templates
//-------------------------
void FX_Stop( void )
{
	for ( int i = 0; i < MAX_EFFECTS; i++ )
	{
		if ( effectList[i].mEffect )
		{
			delete effectList[i].mEffect;
		}

		effectList[i].mEffect = 0;
	}

	activeFx = 0;

	theFxScheduler.Clean(false);
}

//-------------------------
// FX_Init
//
// Preps system for use
//-------------------------
int	FX_Init( refdef_t* refdef )
{
//	FX_Free( true );
	if ( fxInitialized == qfalse )
	{
		fxInitialized = qtrue;

		for ( int i = 0; i < MAX_EFFECTS; i++ )
		{
			effectList[i].mEffect = 0;
		}
	}
	nextValidEffect = &effectList[0];

#ifdef _DEBUG
	fx_freeze = Cvar_Get("fx_freeze", "0", CVAR_CHEAT);
#endif
	fx_debug = Cvar_Get("fx_debug", "0", CVAR_TEMP);
	fx_countScale = Cvar_Get("fx_countScale", "1", CVAR_ARCHIVE_ND);
	fx_nearCull = Cvar_Get("fx_nearCull", "16", CVAR_ARCHIVE_ND);

	theFxHelper.ReInit(refdef);

	return true;
}

void FX_SetRefDef(refdef_t *refdef)
{
	theFxHelper.refdef = refdef;
}

//-------------------------
// FX_FreeMember
//-------------------------
static void FX_FreeMember( SEffectList *obj )
{
	obj->mEffect->Die();
	delete obj->mEffect;
	obj->mEffect = 0;

	// May as well mark this to be used next
	nextValidEffect = obj;

	activeFx--;
}


//-------------------------
// FX_GetValidEffect
//
// Finds an unused effect slot
//
// Note - in the editor, this function may return NULL, indicating that all
// effects are being stopped.
//-------------------------
static SEffectList *FX_GetValidEffect()
{
	if ( nextValidEffect->mEffect == 0 )
	{
		return nextValidEffect;
	}

	int			i;
	SEffectList	*ef;

	// Blah..plow through the list till we find something that is currently untainted
	for ( i = 0, ef = effectList; i < MAX_EFFECTS; i++, ef++ )
	{
		if ( ef->mEffect == 0 )
		{
			return ef;
		}
	}

	// report the error.
#ifndef FINAL_BUILD
	theFxHelper.Print( "FX system out of effects\n" );
#endif

	// Hmmm.. just trashing the first effect in the list is a poor approach
	FX_FreeMember( &effectList[0] );

	// Recursive call
	return nextValidEffect;
}

//-------------------------
// FX_Add
//
// Adds all fx to the view
//-------------------------
void FX_Add( bool portal )
{
	int			i;
	SEffectList	*ef;

	drawnFx = 0;

	int numFx = activeFx;	//but stop when there can't be any more left!
	for ( i = 0, ef = effectList; i < MAX_EFFECTS && numFx; i++, ef++ )
	{
		if ( ef->mEffect != 0)
		{
			--numFx;
			if (portal != ef->mPortal)
			{
				continue;	//this one does not render in this scene
			}
			// Effect is active
			if ( theFxHelper.mTime > ef->mKillTime )
			{
				// Clean up old effects, calling any death effects as needed
				// this flag just has to be cleared otherwise death effects might not happen correctly
				ef->mEffect->ClearFlags( FX_KILL_ON_IMPACT );
				FX_FreeMember( ef );
			}
			else
			{
				if ( ef->mEffect->Update() == false )
				{
					// We've been marked for death
					FX_FreeMember( ef );
					continue;
				}
			}
		}
	}


	if ( fx_debug->integer && !portal)
	{
		theFxHelper.Print( "Active    FX: %i\n", activeFx );
		theFxHelper.Print( "Drawn     FX: %i\n", drawnFx );
		theFxHelper.Print( "Scheduled FX: %i High: %i\n", theFxScheduler.NumScheduledFx(), theFxScheduler.GetHighWatermark() );
	}
}



//-------------------------
// FX_AddPrimitive
//
// Note - in the editor, this function may change *pEffect to NULL, indicating that
// all effects are being stopped.
//-------------------------
extern bool gEffectsInPortal;	//from FXScheduler.cpp so i don't have to pass it in on EVERY FX_ADD*
void FX_AddPrimitive( CEffect **pEffect, int killTime )
{
	SEffectList *item = FX_GetValidEffect();

	item->mEffect = *pEffect;
	item->mKillTime = theFxHelper.mTime + killTime;
	item->mPortal = gEffectsInPortal;	//global set in AddScheduledEffects

	activeFx++;

	// Stash these in the primitive so it has easy access to the vals
	(*pEffect)->SetTimeStart( theFxHelper.mTime );
	(*pEffect)->SetTimeEnd( theFxHelper.mTime + killTime );
}

//-------------------------
//  FX_AddParticle
//-------------------------
CParticle *FX_AddParticle( vec3_t org, vec3_t vel, vec3_t accel, float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							float rotation, float rotationDelta,
							vec3_t min, vec3_t max, float elasticity,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags = 0,
							EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
							CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding effects when the system is paused
		return 0;
	}

	CParticle *fx = new CParticle;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( org );
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( org );
		}
		fx->SetOrigin1( org );
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetVel( vel );
		fx->SetAccel( accel );

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetFlags( flags );
		fx->SetShader( shader );
		fx->SetRotation( rotation );
		fx->SetRotationDelta( rotationDelta );
		fx->SetElasticity( elasticity );
		fx->SetMin( min );
		fx->SetMax( max );
		fx->SetDeathFxID( deathID );
		fx->SetImpactFxID( impactID );

		fx->Init();

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}



//-------------------------
//  FX_AddLine
//-------------------------
CLine *FX_AddLine( vec3_t start, vec3_t end, float size1, float size2, float sizeParm,
									float alpha1, float alpha2, float alphaParm,
									vec3_t sRGB, vec3_t eRGB, float rgbParm,
									int killTime, qhandle_t shader, int flags = 0,
									EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
									CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/)
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding new effects when the system is paused
		return 0;
	}

	CLine *fx = new CLine;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( start ); //offset from bolt pos
			fx->SetVel( end );	//vel is the vector offset from bolt+orgOffset
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( start );
			fx->SetOrigin2( end );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetShader( shader );
		fx->SetFlags( flags );

		fx->SetSTScale( 1.0f, 1.0f );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}


//-------------------------
//  FX_AddElectricity
//-------------------------
CElectricity *FX_AddElectricity( vec3_t start, vec3_t end, float size1, float size2, float sizeParm,
								float alpha1, float alpha2, float alphaParm,
								vec3_t sRGB, vec3_t eRGB, float rgbParm,
								float chaos, int killTime, qhandle_t shader, int flags = 0,
								EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
								CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding new effects when the system is paused
		return 0;
	}

	CElectricity *fx = new CElectricity;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( start );//offset
			fx->SetVel( end );	//vel is the vector offset from bolt+orgOffset
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( start );
			fx->SetOrigin2( end );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetShader( shader );
		fx->SetFlags( flags );
		fx->SetChaos( chaos );

		fx->SetSTScale( 1.0f, 1.0f );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
		// in the editor, fx may now be NULL?
		if ( fx )
		{
			fx->Initialize();
		}
	}

	return fx;
}


//-------------------------
//  FX_AddTail
//-------------------------
CTail *FX_AddTail( vec3_t org, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							vec3_t min, vec3_t max, float elasticity,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags = 0,
							EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
							CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding effects when the system is paused
		return 0;
	}

	CTail *fx = new CTail;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( org );
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( org );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetVel( vel );
		fx->SetAccel( accel );

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Length----------------
		fx->SetLengthStart( length1 );
		fx->SetLengthEnd( length2 );

		if (( flags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_WAVE )
		{
			fx->SetLengthParm( lengthParm * PI * 0.001f );
		}
		else if ( flags & FX_LENGTH_PARM_MASK )
		{
			fx->SetLengthParm( lengthParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetFlags( flags );
		fx->SetShader( shader );
		fx->SetElasticity( elasticity );
		fx->SetMin( min );
		fx->SetMax( max );
		fx->SetSTScale( 1.0f, 1.0f );
		fx->SetDeathFxID( deathID );
		fx->SetImpactFxID( impactID );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}


//-------------------------
//  FX_AddCylinder
//-------------------------
CCylinder *FX_AddCylinder( vec3_t start, vec3_t normal,
							float size1s, float size1e, float size1Parm,
							float size2s, float size2e, float size2Parm,
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
							CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/,
							qboolean traceEnd)
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding new effects when the system is paused
		return 0;
	}

	CCylinder *fx = new CCylinder;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( start );//offset
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( start );
		}
		fx->SetTraceEnd(traceEnd);

		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetOrigin1( start );
		fx->SetNormal( normal );

		// RGB----------------
		fx->SetRGBStart( rgb1 );
		fx->SetRGBEnd( rgb2 );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size1----------------
		fx->SetSizeStart( size1s );
		fx->SetSizeEnd( size1e );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( size1Parm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( size1Parm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size2----------------
		fx->SetSize2Start( size2s );
		fx->SetSize2End( size2e );

		if (( flags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_WAVE )
		{
			fx->SetSize2Parm( size2Parm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE2_PARM_MASK )
		{
			fx->SetSize2Parm( size2Parm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Length1---------------
		fx->SetLengthStart( length1 );
		fx->SetLengthEnd( length2 );

		if (( flags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_WAVE )
		{
			fx->SetLengthParm( lengthParm * PI * 0.001f );
		}
		else if ( flags & FX_LENGTH_PARM_MASK )
		{
			fx->SetLengthParm( lengthParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetShader( shader );
		fx->SetFlags( flags );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}

//-------------------------
//  FX_AddEmitter
//-------------------------
CEmitter *FX_AddEmitter( vec3_t org, vec3_t vel, vec3_t accel,
								float size1, float size2, float sizeParm,
								float alpha1, float alpha2, float alphaParm,
								vec3_t rgb1, vec3_t rgb2, float rgbParm,
								vec3_t angs, vec3_t deltaAngs,
								vec3_t min, vec3_t max, float elasticity,
								int deathID, int impactID, int emitterID,
								float density, float variance,
								int killTime, qhandle_t model, int flags = 0,
								EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
								CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding effects when the system is paused
		return 0;
	}

	CEmitter *fx = new CEmitter;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			assert(0);//not done
//			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetOrigin1( org );
		fx->SetVel( vel );
		fx->SetAccel( accel );

		// RGB----------------
		fx->SetRGBStart( rgb1 );
		fx->SetRGBEnd( rgb2 );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetAngles( angs );
		fx->SetAngleDelta( deltaAngs );
		fx->SetFlags( flags );
		fx->SetModel( model );
		fx->SetElasticity( elasticity );
		fx->SetMin( min );
		fx->SetMax( max );
		fx->SetDeathFxID( deathID );
		fx->SetImpactFxID( impactID );
		fx->SetEmitterFxID( emitterID );
		fx->SetDensity( density );
		fx->SetVariance( variance );
		fx->SetOldTime( theFxHelper.mTime );

		fx->SetLastOrg( org );
		fx->SetLastVel( vel );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}

//-------------------------
//  FX_AddLight
//-------------------------
CLight *FX_AddLight( vec3_t org, float size1, float size2, float sizeParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, int flags = 0,
							EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
							CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/)
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding effects when the system is paused
		return 0;
	}

	CLight *fx = new CLight;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( org );//offset
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( org );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);

		// RGB----------------
		fx->SetRGBStart( rgb1 );
		fx->SetRGBEnd( rgb2 );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetFlags( flags );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;

}


//-------------------------
//  FX_AddOrientedParticle
//-------------------------
COrientedParticle *FX_AddOrientedParticle( vec3_t org, vec3_t norm, vec3_t vel, vec3_t accel,
						float size1, float size2, float sizeParm,
						float alpha1, float alpha2, float alphaParm,
						vec3_t rgb1, vec3_t rgb2, float rgbParm,
						float rotation, float rotationDelta,
						vec3_t min, vec3_t max, float bounce,
						int deathID, int impactID,
						int killTime, qhandle_t shader, int flags = 0,
						EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/,
						CGhoul2Info_v *ghoul2/*0*/, int entNum/*-1*/, int modelNum/*-1*/, int boltNum/*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding effects when the system is paused
		return 0;
	}

	COrientedParticle *fx = new COrientedParticle;

	if ( fx )
	{
		if (flags&FX_RELATIVE && ghoul2 != NULL)
		{
			fx->SetOrigin1( NULL );
			fx->SetOrgOffset( org );//offset
			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		else
		{
			fx->SetOrigin1( org );
		}
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetOrigin1( org );
		fx->SetNormal( norm );
		fx->SetVel( vel );
		fx->SetAccel( accel );

		// RGB----------------
		fx->SetRGBStart( rgb1 );
		fx->SetRGBEnd( rgb2 );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetFlags( flags );
		fx->SetShader( shader );
		fx->SetRotation( rotation );
		fx->SetRotationDelta( rotationDelta );
		fx->SetElasticity( bounce );
		fx->SetMin( min );
		fx->SetMax( max );
		fx->SetDeathFxID( deathID );
		fx->SetImpactFxID( impactID );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}


//-------------------------
//  FX_AddPoly
//-------------------------
CPoly *FX_AddPoly( vec3_t *verts, vec2_t *st, int numVerts,
							vec3_t vel, vec3_t accel,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t rotationDelta, float bounce, int motionDelay,
							int killTime, qhandle_t shader, int flags )
{
	if ( theFxHelper.mFrameTime < 1 || !verts )
	{ // disallow adding effects when the system is paused or the user doesn't pass in a vert array
		return 0;
	}

	CPoly *fx = new CPoly;

	if ( fx )
	{
		// Do a cheesy copy of the verts and texture coords into our own structure
		for ( int i = 0; i < numVerts; i++ )
		{
			VectorCopy( verts[i], fx->mOrg[i] );
			VectorCopy2( st[i], fx->mST[i] );
		}

		fx->SetVel( vel );
		fx->SetAccel( accel );

		// RGB----------------
		fx->SetRGBStart( rgb1 );
		fx->SetRGBEnd( rgb2 );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetFlags( flags );
		fx->SetShader( shader );
		fx->SetRot( rotationDelta );
		fx->SetElasticity( bounce );
		fx->SetMotionTimeStamp( motionDelay );
		fx->SetNumVerts( numVerts );

		// Now that we've set our data up, let's process it into a useful format
		fx->PolyInit();

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}


//-------------------------
//  FX_AddFlash
//-------------------------
CFlash *FX_AddFlash( vec3_t origin,
					float size1, float size2, float sizeParm,
					float alpha1, float alpha2, float alphaParm,
					vec3_t sRGB, vec3_t eRGB, float rgbParm,
					int killTime, qhandle_t shader, int flags,
					EMatImpactEffect matImpactFX /*MATIMPACTFX_NONE*/, int fxParm /*-1*/ )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding new effects when the system is paused
		return 0;
	}

	if (!shader)
	{ //yeah..this is bad, I guess, but SP seems to handle it by not drawing the flash, so I will too.
		assert(shader);
		return 0;
	}

	CFlash *fx = new CFlash;

	if ( fx )
	{
		fx->SetMatImpactFX(matImpactFX);
		fx->SetMatImpactParm(fxParm);
		fx->SetOrigin1( origin );

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetShader( shader );
		fx->SetFlags( flags );

//		fx->SetSTScale( 1.0f, 1.0f );

		fx->Init();

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}

//-------------------------
//  FX_AddBezier
//-------------------------
CBezier *FX_AddBezier( vec3_t start, vec3_t end,
								vec3_t control1, vec3_t control1Vel,
								vec3_t control2, vec3_t control2Vel,
								float size1, float size2, float sizeParm,
								float alpha1, float alpha2, float alphaParm,
								vec3_t sRGB, vec3_t eRGB, float rgbParm,
								int killTime, qhandle_t shader, int flags )
{
	if ( theFxHelper.mFrameTime < 1 )
	{ // disallow adding new effects when the system is paused
		return 0;
	}

	CBezier *fx = new CBezier;

	if ( fx )
	{
		fx->SetOrigin1( start );
		fx->SetOrigin2( end );

		fx->SetControlPoints( control1, control2 );
		fx->SetControlVel( control1Vel, control2Vel );

		// RGB----------------
		fx->SetRGBStart( sRGB );
		fx->SetRGBEnd( eRGB );

		if (( flags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
		{
			fx->SetRGBParm( rgbParm * PI * 0.001f );
		}
		else if ( flags & FX_RGB_PARM_MASK )
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm( rgbParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Alpha----------------
		fx->SetAlphaStart( alpha1 );
		fx->SetAlphaEnd( alpha2 );

		if (( flags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
		{
			fx->SetAlphaParm( alphaParm * PI * 0.001f );
		}
		else if ( flags & FX_ALPHA_PARM_MASK )
		{
			fx->SetAlphaParm( alphaParm * 0.01f * killTime + theFxHelper.mTime );
		}

		// Size----------------
		fx->SetSizeStart( size1 );
		fx->SetSizeEnd( size2 );

		if (( flags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
		{
			fx->SetSizeParm( sizeParm * PI * 0.001f );
		}
		else if ( flags & FX_SIZE_PARM_MASK )
		{
			fx->SetSizeParm( sizeParm * 0.01f * killTime + theFxHelper.mTime );
		}

		fx->SetShader( shader );
		fx->SetFlags( flags );

		fx->SetSTScale( 1.0f, 1.0f );

		FX_AddPrimitive( (CEffect**)&fx, killTime );
	}

	return fx;
}
