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

#include "g_local.h"
#include "g_roff.h"
#include "Q3_Interface.h"
#include "../cgame/cg_local.h"
#include "../cgame/cg_media.h" //Archangel - needed for fixing ROFF2 sounds
#include "g_functions.h"
#include "qcommon/ojk_saved_game_helper.h"

// The list of precached ROFFs
roff_list_t	roffs[MAX_ROFFS];
int			num_roffs = 0;

qboolean g_bCollidableRoffs = qfalse;

extern stringID_table_t animTable [MAX_ANIMATIONS+1];	//Archangel - needed for misc_model_ghoul 'animSequence'
extern int G_ParseAnimFileSet(const char *skeletonName, const char *modelName = 0);	//Archangel - needed for misc_model_ghoul 'animSequence'
extern void	Q3_TaskIDComplete( gentity_t *ent, taskID_t taskType );

static void G_RoffNotetrackCallback( gentity_t *ent, const char *notetrack)
{
	int addlArgs = 0;
	int i = 0, r = 0, r2 = 0, objectID = 0, anglesGathered = 0, posoffsetGathered = 0, animSettingsGathered = 0, efxSettingsGathered = 0;
	int animFlags = 0, setFrame = 0, blendTime = 0;
	float animSpeed = 0.0f;
	char type[256];
	char argument[512];
	char addlArg[512];
	char errMsg[256];
	char t[64];
	char teststr[256];
	std::vector<std::string> addlArgBuffer;
	vec3_t parsedAngles, parsedOffset, useAngles, useOrigin, forward, right, up;

	if (!ent || !notetrack)
	{
		return;
	}

	//Supported Notetrack types:	effect, sound, loop, USE, play
	//General Notetrack format:		type argument additionalArguments
	//Each type provides further details on expected arguments.  Note: <> denote required argument, [] denote optional argument

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if (notetrack[i] != ' ')
	{ //didn't pass in a valid notetrack type, or forgot the argument for it
		return;
	}

	i++;

	while (notetrack[i] && notetrack[i] != ' ')
	{
		if (notetrack[i] != '\n' && notetrack[i] != '\r')
		{ //don't read line ends for an argument
			argument[r] = notetrack[i];
			r++;
		}
		i++;
	}
	argument[r] = '\0';

	if (!r)
	{
		return;
	}

	if (notetrack[i] == ' ')
	{ //additional arguments...
		addlArgs = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addlArg[r] = notetrack[i];
			r++;
			i++;
		}
		addlArg[r] = '\0';
	}

	if (strcmp(type, "effect") == 0)
	{ //effect notetrack format =  "effect <effects_relative_filepath.ext> [±OffsetX±OffsetY±OffsetZ] [XANGLE-YANGLE-ZANGLE]"
		//	note: '+' and '-' are delimiters! Not positive or negative
		//	effect notetrack example:  "effect effects/explosion1.efx 0+0+64 0-0-1"  

		if (!addlArgs)
		{
			VectorClear(parsedOffset);
			goto defaultoffsetposition;
		}

		i = 0;

		while (posoffsetGathered < 3)
		{
			r = 0;
			while (addlArg[i] && addlArg[i] != '+' && addlArg[i] != ' ')
			{
				t[r] = addlArg[i];
				r++;
				i++;
			}
			t[r] = '\0';
			i++;
			if (!r)
			{ //failure...
				VectorClear(parsedOffset);
				i = 0;
				goto defaultoffsetposition;
			}
			parsedOffset[posoffsetGathered] = atof(t);
			posoffsetGathered++;
		}

		if (posoffsetGathered < 3)
		{
			sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
			goto functionend;
		}

		i--;

		if (addlArg[i] != ' ')
		{
			addlArgs = 0;
		}

defaultoffsetposition:

		r = 0;
		if (argument[r] == '/')
		{
			r++;
		}
		while (argument[r] && argument[r] != '/')
		{
			teststr[r2] = argument[r];
			r2++;
			r++;
		}
		teststr[r2] = '\0';

		if (r2 && strstr(teststr, "effects"))
		{ //get rid of the leading "effects" since it's auto-inserted
			r++;
			r2 = 0;

			while (argument[r])
			{
				teststr[r2] = argument[r];
				r2++;
				r++;
			}
			teststr[r2] = '\0';

			strcpy(argument, teststr);
		}

		objectID = G_EffectIndex(argument);
		r = 0;

		if (objectID)
		{
			if (addlArgs)
			{ //if there is a second additional argument for an effect it's format is expected to be:  XANGLE-YANGLE-ZANGLE (in degrees)
				i++;
				while (anglesGathered < 3)
				{
					r = 0;
					while (addlArg[i] && addlArg[i] != '-')
					{
						t[r] = addlArg[i];
						r++;
						i++;
					}
					t[r] = '\0';
					i++;

					if (!r)
					{ //failed to get a new part of the vector
						anglesGathered = 0;
						break;
					}

					parsedAngles[anglesGathered] = atof(t);
					anglesGathered++;
				}

				if (anglesGathered)
				{
					VectorCopy(parsedAngles, useAngles);
				}
				else
				{ //failed to parse angles from the extra argument provided...
					VectorCopy(ent->s.apos.trBase, useAngles);
				}
			}
			else
			{ //if no constant angles, play in direction entity is facing
				VectorCopy(ent->s.apos.trBase, useAngles);
			}

			AngleVectors(useAngles, forward, right, up);

			VectorCopy(ent->s.pos.trBase, useOrigin);

			//forward
			useOrigin[0] += forward[0]*parsedOffset[0];
			useOrigin[1] += forward[1]*parsedOffset[0];
			useOrigin[2] += forward[2]*parsedOffset[0];

			//right
			useOrigin[0] += right[0]*parsedOffset[1];
			useOrigin[1] += right[1]*parsedOffset[1];
			useOrigin[2] += right[2]*parsedOffset[1];

			//up
			useOrigin[0] += up[0]*parsedOffset[2];
			useOrigin[1] += up[1]*parsedOffset[2];
			useOrigin[2] += up[2]*parsedOffset[2];

#if !NDEBUG
			Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
			G_PlayEffect(objectID, useOrigin, useAngles);
		}
	}
	else if (strcmp(type, "sound") == 0)
	{
		//sound notetrack format =	"sound <relative_soundpath.ext>" 
		//sound notetrack example:	"sound sound/vehicles/tie/flyby2.mp3"
		
#if !NDEBUG
		Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
		//check for eType and play the sound
		if (ent->s.eType == ET_MOVER)
		{
			objectID = cgi_S_RegisterSound(argument);
			cgi_S_StartSound(ent->s.pos.trBase, ent->s.number, CHAN_BODY, objectID);
		}
		else
		{
			G_SoundOnEnt(ent, CHAN_BODY, argument);
		}
	}
	else if (strcmp(type, "loop") == 0)
    {
		//loop notetrack format =	"loop <rof|sfx> <relative|absolute>|<sound_relativepath.ext>|<kill>" 
		//loop notetrack examples:	"loop rof relative" or "loop rof absolute" or...
		//							"loop sfx sound/vehicles/tie/loop.wav" or "loop sfx kill"
									
		if (strcmp(argument, "rof") == 0)
		{
			if (strcmp(addlArg, "relative") == 0) //reset rof to original delta position/rotation at current location before looping
			{
				VectorCopy(ent->s.origin2, ent->s.pos.trBase);
				VectorCopy(ent->s.origin2, ent->currentOrigin);
				VectorCopy(ent->s.angles2, ent->s.apos.trBase);
				VectorCopy(ent->s.angles2, ent->currentAngles);
			}
			else if (strcmp(addlArg, "absolute") == 0) //reset rof to original delta position/rotation world location before looping
			{
				VectorSubtract(ent->pos1, ent->pos1, ent->pos1);
				VectorSubtract(ent->pos2, ent->pos2, ent->pos2);

				VectorSubtract(ent->s.pos.trBase, ent->s.pos.trBase, ent->s.pos.trBase);
				VectorSubtract(ent->currentOrigin, ent->currentOrigin, ent->currentOrigin);
				VectorSubtract(ent->s.apos.trBase, ent->s.apos.trBase, ent->s.apos.trBase);
				VectorSubtract(ent->currentAngles, ent->currentAngles, ent->currentAngles);

				VectorCopy(ent->currentOrigin, ent->s.origin2);
				VectorCopy(ent->currentAngles, ent->s.angles2);
			}
			else
			{
				sprintf(errMsg, "Invalid additional argument <%s> for type 'loop rof'", addlArg);
				goto functionend;
			}
			
			// Start the ROFF from the beginning
			ent->roff_ctr = 0;

			// Let the ROFF playing start.
			ent->next_roff_time = level.time;

			//Re-link entity
			gi.linkentity(ent);

#if !NDEBUG
			Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
			// Re-apply the ROFF
			G_Roff(ent);			
		}
		else if (strcmp(argument, "sfx") == 0)
		{
			//check additional argument for relative sound path
			r = 0;
			r2 = 0;
			
			if (addlArg[r] == '/')
			{
				r++;
			}
			while (addlArg[r] && addlArg[r] != '/')
			{
				teststr[r2] = addlArg[r];
				r2++;
				r++;
			}
			teststr[r2] = '\0';

			if (r2 && strstr(teststr, "kill"))
			{ // kill the looping sound
				ent->s.loopSound = 0;
#if !NDEBUG
				Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
			}
			else if (r2 && strstr(teststr, "sound"))
			{
				//OK... we should have a relative sound path

#if !NDEBUG
				Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
				//try to register the sound and add it to the entitystate loopSound parameter
				if (ent->s.eType == ET_MOVER)
				{
					objectID = cgi_S_RegisterSound(addlArg);
					if (objectID)
						ent->s.loopSound = objectID;
					else
					{
						ent->s.loopSound = 0;
						sprintf(errMsg, "cgi_S_RegisterSound(%s) failed to return a valid sfxHandle_t for additional argument. Setting 'loopSound' to 0.", addlArg);
						goto functionend;
					}
				}
				else
				{
					ent->s.loopSound = G_SoundIndex(addlArg);
				}
			}
			else
			{
				sprintf(errMsg, "Invalid additional argument <%s> for type 'loop sfx'", addlArg);
				goto functionend;
			}
		}
		else if (strcmp(argument, "efx") == 0)
		{
			qboolean bKillEFX = qfalse;

			//looping efx only supported by Ghould models... check class name
			if (ent->classname == "misc_model_ghoul" || ent->ghoul2.IsValid())
			{
				//G_PlayEffect( int fxID, 
				//				const int modelIndex, 
				//				const int boltIndex, 
				//				const int entNum, 
				//				const vec3_t origin, 
				//				int iLoopTime,			//iLoopTime 0 = not looping, 1 for infinite, else duration
				//				qboolean isRelative ) 

				//parse additional argument for G_PlayEffect parameters
				if (addlArgs)
				{
					if (addlArg[0] != '\0')
					{
						while (efxSettingsGathered < 1)
						{	//get the fxID
							i = 0;
							if (addlArg[i] == '/')
							{
								i++;
							}
							r = 0;
							while (addlArg[i] && addlArg[i] != '/')
							{
								teststr[r] = addlArg[i];
								r++;
								i++;
							}
							teststr[r] = '\0';

							if (r && strstr(teststr, "kill-effects"))
							{ // kill the looping efx
#if !NDEBUG
								Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
								//get rid of the leading "kill-effects"
								i++;
								r = 0;

								while (addlArg[i] && addlArg[i] != ' ')
								{
									teststr[r] = addlArg[i];
									r++;
									i++;
								}
								teststr[r] = '\0';

								//set to kill
								bKillEFX = qtrue;
							}
							else if (r && strstr(teststr, "effects"))
							{ //get rid of the leading "effects" since it's auto-inserted
#if !NDEBUG
								Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
								i++;
								r = 0;

								while (addlArg[i] && addlArg[i] != ' ')
								{
									teststr[r] = addlArg[i];
									r++;
									i++;
								}
								teststr[r] = '\0';
							}

							if (!r)
							{ //failure...
								sprintf(errMsg, "Invalid additional argument [%s]", addlArg);
								i = 0;
								goto functionend;
							}
							addlArgBuffer.push_back(teststr);
							efxSettingsGathered++;
						}

						objectID = G_EffectIndex(addlArgBuffer[0].c_str());

						if (objectID) //fxID is valid... press on
						{
							if (addlArg[i] == ' ')  //we have custom settings! Or do we?
							{
								i++; //move to first char in second additional argument

								if (bKillEFX)
								{
									while (efxSettingsGathered < 2) //we still count the fxID just gathered
									{ //keep parsing second part of addlArg
										r = 0;
										while (addlArg[i] && addlArg[i] != ':')
										{
											t[r] = addlArg[i];
											r++;
											i++;
										}
										t[r] = '\0';
										i++;
										if (!r)
										{ //failure...
											sprintf(errMsg, "Invalid additional argument [%s]", addlArg);
											i = 0;
											goto functionend;
										}
										addlArgBuffer.push_back(t);
										efxSettingsGathered++;
									}

									if (efxSettingsGathered != 2)
									{ //failure...
										sprintf(errMsg, "Invalid additional argument count [%s]. Expected 2, got %i", addlArg, efxSettingsGathered);
										i = 0;
										goto functionend;
									}
									else
									{
										//copy addlArgBuffer values to G_StopEffect parameters
										int boltID = gi.G2API_AddBolt(&ent->ghoul2[0], addlArgBuffer[1].c_str());

										//stop the efx
										G_StopEffect(objectID, ent->playerModel, boltID, ent->s.number);

										//remove the eFlags2
										ent->s.eFlags2 &= ~(EF2_ROFF2_LOOP_EFX);
									}
								}
								else
								{
									while (efxSettingsGathered < 6) //we still count the fxID just gathered
									{ //keep parsing second part of addlArg
										r = 0;
										while (addlArg[i] && addlArg[i] != ':')
										{
											t[r] = addlArg[i];
											r++;
											i++;
										}
										t[r] = '\0';
										i++;
										if (!r)
										{ //failure...
											sprintf(errMsg, "Invalid additional argument [%s]", addlArg);
											i = 0;
											goto functionend;
										}
										addlArgBuffer.push_back(t);
										efxSettingsGathered++;
									}

									//copy addlArgBuffer values to G_PlayEffect parameters
									int boltID = gi.G2API_AddBolt(&ent->ghoul2[0], addlArgBuffer[1].c_str());

									// parse origin offset
									char posBuffer[64];
									strncpy(posBuffer, addlArgBuffer[2].c_str(), addlArgBuffer[2].length());
									posBuffer[addlArgBuffer[2].length()] = '\0';
									i = 0;
									while (posoffsetGathered < 3)
									{
										r = 0;
										while (posBuffer[i] && posBuffer[i] != '+')
										{
											t[r] = posBuffer[i];
											r++;
											i++;
										}
										t[r] = '\0';
										i++;
										if (!r)
										{ //failure... 
											VectorClear(parsedOffset); //just zero-out offset and play efx at origin
											i = 0;
											goto zerooffsetposition;
										}
										parsedOffset[posoffsetGathered] = atof(t);
										posoffsetGathered++;
									}

									if (posoffsetGathered < 3)
									{
										sprintf(errMsg, "Offset position argument for 'loop efx' type is invalid.");
										goto functionend;
									}

zerooffsetposition:
									int iLoopTime = atoi(addlArgBuffer[3].c_str());
									qboolean isRelative;
									if (strcmp(addlArgBuffer[4].c_str(), "true") == 0)
										isRelative = qtrue;
									else
										isRelative = qfalse;
									
									//parse the angles
									// angles argument format is expected to be:  XANGLE-YANGLE-ZANGLE (in degrees)
									// note: the '-' is a delimiter, not numerical sign... blame Raven

									// parse origin offset
									char pos2Buffer[64];
									strncpy(pos2Buffer, addlArgBuffer[5].c_str(), addlArgBuffer[5].length());
									pos2Buffer[addlArgBuffer[5].length()] = '\0';
									i = 0;
									while (anglesGathered < 3)
									{
										r = 0;
										while (pos2Buffer[i] && pos2Buffer[i] != '-')
										{
											t[r] = pos2Buffer[i];
											r++;
											i++;
										}
										t[r] = '\0';
										i++;

										if (!r)
										{ //failed to get a new part of the vector
											anglesGathered = 0;
											break;
										}

										parsedAngles[anglesGathered] = atof(t);
										anglesGathered++;
									}

									if (anglesGathered)
									{
										VectorCopy(parsedAngles, useAngles);
									}
									else
									{ //failed to parse angles from the extra argument provided... default to zero
										VectorCopy(vec3_origin, useAngles);
									}

									//set the eFlags2
									ent->s.eFlags2 |= EF2_ROFF2_LOOP_EFX;
							
									//play the efx
									G_PlayEffect(objectID, ent->playerModel, boltID, ent->s.number, parsedOffset, iLoopTime, isRelative, useAngles);
								}
							}
							else
							{ //missing efx additional arguments
								sprintf(errMsg, "Missing additional arguments for type 'loop efx' notetrack.  Playing basic efx at origin.");
								G_PlayEffect(objectID, ent->s.origin);
								goto functionend;
							}
						}
						else
						{
							sprintf(errMsg, "fxID for not found for 'loop efx' notetrack: %s", notetrack);
							goto functionend;
						}
					}
					else
					{
						sprintf(errMsg, "Missing additional argument for type 'loop efx' notetrack.");
						goto functionend;
					}
				}
				else
				{
					sprintf(errMsg, "Invalid additional argument <%s> for type 'loop efx'", addlArg);
					goto functionend;
				}
			}
			else
			{
				sprintf(errMsg, "Invalid entity class <%s> for type 'loop efx' -- only Ghoul2 supported.", ent->classname);
				goto functionend;
			}
		}
		else
		{
			sprintf(errMsg, "Invalid argument <%s> for type 'loop' notetrack.", argument);
			goto functionend;
		}
    }
    else if (strcmp(type, "USE") == 0)
    {
		//USE notetrack format =	"USE <IBI_ScriptName_noExt>" 
		//USE notetrack example:	"USE airborne"
		
        //try to cache the script
        Quake3Game()->PrecacheScript(argument);

#if !NDEBUG
		Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
        //run the IBI script
        Quake3Game()->RunScript(ent, argument);
    }
	else if (strcmp(type, "play") == 0)
	{
		//play notetrack format =	"play <GLAanim|MD3anim> <animSequence> [<rootBoneName>:<animFlags>:<animSpeed>:<setFrame>:<blendTime>]"
		//play notetrack example 1:	"play GLAanim BOTH_ATTACK1"
		//play notetrack example 2:	"play GLAanim BOTH_ATTACK1 model_root:72:1.0:0:350"
		//play notetrack example 3:	"play MD3anim <animSequence> (Note: currently unsupported... WIP)

		//first up, clear the addlArgBuffer
		addlArgBuffer.clear();
		addlArgBuffer.shrink_to_fit();
		
		if (strcmp(argument, "GLAanim") == 0)
		{
			//check additional argument for an animation sequence and settings	
			if (addlArgs)
			{					
				if ( addlArg[0] != '\0' )  
				{

					i = 0;

					while (animSettingsGathered < 1)
					{	//get the animSequence
						r = 0;
						while (addlArg[i] && addlArg[i] != ' ')
						{
							t[r] = addlArg[i];
							r++;
							i++;
						}
						t[r] = '\0';
						i++;
						if (!r)
						{ //failure...
							sprintf(errMsg, "Invalid additional argument [%s]", addlArg);
							i = 0;
							goto functionend;
						}
						addlArgBuffer.push_back(t);
						animSettingsGathered++;
					}

					i--; //back up one char to check for a space

					if (addlArg[i] == ' ') //we have custom settings! Or do we?
					{
						i++; //move to first char in second additional argument
						while (animSettingsGathered < 6) //we still count the animSequence just gathered
						{ //keep parsing second part of addlArg
							r = 0;
							while (addlArg[i] && addlArg[i] != ':')
							{
								t[r] = addlArg[i];
								r++;
								i++;
							}
							t[r] = '\0';
							i++;
							if (!r)
							{ //failure...
								sprintf(errMsg, "Invalid additional argument [%s]", addlArg);
								i = 0;
								goto functionend;
							}
							addlArgBuffer.push_back(t);
							animSettingsGathered++;
						}

						//copy addlArgBuffer values to parameters
						//	Note:	assigning the addlArgBuffer[1].c_str() to ent->rootBoneName causes the animation to fail to play and jitter.
						//			So just pass it SetBoneAnim function directly
						strcpy(ent->animSequence, addlArgBuffer[0].c_str());
						animFlags = atoi(addlArgBuffer[2].c_str());
						animSpeed = atof(addlArgBuffer[3].c_str());
						setFrame = atoi(addlArgBuffer[4].c_str());
						blendTime = atoi(addlArgBuffer[5].c_str());

						if (setFrame < 0 && setFrame != -1)
							setFrame = -1;

						//find animation index
						int anim = GetIDForString(animTable, ent->animSequence);
						char* skeletonName;
						gi.G2API_GetAnimFileName(&ent->ghoul2[0], &skeletonName);
						char* strippedSkelName = COM_SkipPath(skeletonName);
						int animFileIndex = G_ParseAnimFileSet(strippedSkelName, ent->model);

						if (animFileIndex)
						{
							if (anim >= 0)
							{
								animation_t *animations = level.knownAnimFileSets[animFileIndex].animations;

								if (setFrame >= 0)
									setFrame += animations[anim].firstFrame;

								int endFrame = ((animations[anim].numFrames - 1) + animations[anim].firstFrame);

								if (setFrame >= endFrame)
									setFrame = endFrame - 1;

								//assign custom settings to entity
								ent->setFrame = setFrame;
								ent->startFrame = animations[anim].firstFrame;
								ent->endFrame = endFrame;
								ent->s.eG2AnimFlags = animFlags;
								ent->blendTime = blendTime;

#if !NDEBUG
								Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
								gi.G2API_SetBoneAnim(&ent->ghoul2[0], addlArgBuffer[1].c_str(), animations[anim].firstFrame, endFrame,
													ent->s.eG2AnimFlags, animSpeed, (cg.time ? cg.time : level.time), ent->setFrame, ent->blendTime);
							}
							else
							{
								sprintf(errMsg, "Can't find animSequence <%s> in GLA (%s)", ent->animSequence, skeletonName);
								goto functionend;
							}
						}
						else
						{ // no animation.cfg file or one was not specified, try to play specified frames
#if !NDEBUG
							Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
							gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", ent->startFrame, ent->endFrame,
												BONE_ANIM_OVERRIDE_FREEZE, 1.0f, (cg.time ? cg.time : level.time), -1, -1);

							ent->endFrame = 0; // don't allow it to do anything with the animation function in G_main					
						}
					}
					else
					{ //use default settings

						strcpy(ent->animSequence, addlArgBuffer[0].c_str());

						//find animation index
						int anim = GetIDForString(animTable, ent->animSequence);
						char* skeletonName;
						gi.G2API_GetAnimFileName(&ent->ghoul2[0], &skeletonName);
						char* strippedSkelName = COM_SkipPath(skeletonName);
						int animFileIndex = G_ParseAnimFileSet(strippedSkelName, ent->model);

						if (animFileIndex)
						{
							if (anim >= 0)
							{
								animation_t *animations = level.knownAnimFileSets[animFileIndex].animations;
								float animSpeed = 50.0f / animations[anim].frameLerp;
								int blendTime = 350;
#if !NDEBUG
								Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
								gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", animations[anim].firstFrame, ((animations[anim].numFrames - 1) + animations[anim].firstFrame),
													BONE_ANIM_OVERRIDE_FREEZE, animSpeed, (cg.time ? cg.time : level.time), animations[anim].firstFrame, blendTime);
							}
							else
							{
								sprintf(errMsg, "Can't find animSequence <%s> in GLA (%s)", ent->animSequence, skeletonName);
								goto functionend;
							}
						}
						else
						{ // no animation.cfg file or one was not specified, try to play specified frames
#if !NDEBUG
							Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
							gi.G2API_SetBoneAnim(&ent->ghoul2[0], "model_root", ent->startFrame, ent->endFrame,
												BONE_ANIM_OVERRIDE_FREEZE, 1.0f, (cg.time ? cg.time : level.time), -1, -1);

							ent->endFrame = 0; // don't allow it to do anything with the animation function in G_main					
						}
					}
				}
				else
				{
					sprintf(errMsg, "Missing additional argument for type 'play GLAanim' notetrack.");
					goto functionend;
				}
			}
			else
			{
				sprintf(errMsg, "Missing additional argument for type 'play GLAanim' notetrack.");
				goto functionend;
			}
		}
		else if (strcmp(argument, "MD3anim") == 0)
		{
			//TODO:  Write code to support misc_model_breakable ( md3 )
#if !NDEBUG
			Com_Printf(S_COLOR_GREEN"NoteTrack:  \"%s\"\n", notetrack); //DEBUGGING
#endif
			//TODO:  play MD3 animation
		}
		else
		{
			sprintf(errMsg, "Invalid argument <%s> for type 'play' notetrack.", argument);
			goto functionend;
		}
	}
	//else if ...
	else
	{
		if (type[0])
		{
			Com_Printf("Warning: \"%s\" is an invalid ROFF NoteTrack function\n", type);
		}
		else
		{
			Com_Printf("Warning: NoteTrack is missing function and/or arguments\n");
		}
	}

	return;

functionend:
	Com_Printf("Type-specific NoteTrack error: %s\n", errMsg);
	return;
}


//-------------------------------------------------------
// G_CacheRoffNoteTracks
//
// PreCaches the ROFF2 NoteTrack data
//-------------------------------------------------------

static void G_CacheRoffNoteTracks(const char *notetrack)
{
    int i = 0, r = 0, r2 = 0, objectID = 0;
    char type[256]		= {0};
    char argument[512]	= {0};
	char addlArg[512]	= {0};	
	char errMsg[256]	= {0};
	char teststr[256]	= {0};
	int addlArgs = 0;

    if (!notetrack)
    {
        return;
    }

    while (notetrack[i] && notetrack[i] != ' ')
    {
        type[i] = notetrack[i];
        i++;
    }

    type[i] = '\0';

    if (notetrack[i] != ' ')
    { //didn't pass in a valid notetrack type, or forgot the argument for it
        return;
    }

    i++;

    while (notetrack[i] && notetrack[i] != ' ')
    {
        if (notetrack[i] != '\n' && notetrack[i] != '\r')
        { //don't read line ends for an argument
            argument[r] = notetrack[i];
            r++;
        }
        i++;
    }
    argument[r] = '\0';

    if (!r)
    {
        return;
    }
	
	if (notetrack[i] == ' ')
	{ //additional arguments...
		addlArgs = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addlArg[r] = notetrack[i];
			r++;
			i++;
		}
		addlArg[r] = '\0';
	}

    if (strcmp(type, "effect") == 0)
    {
        //try to register the EFX
        objectID = G_EffectIndex(argument);
    }
    else if (strcmp(type, "sound") == 0)
    {
        //try to register the sound
		objectID = cgi_S_RegisterSound(argument);
    }
    else if (strcmp(type, "loop") == 0)
    {
        //only thing we do here is register a sound if 'loop' argument is "sfx"
		//everything else is handled by G_RoffNotetrackCallback function.
		
		if (strcmp(argument, "sfx") == 0)
		{
			//check additional argument for a relative sound path
			r = 0;
			r2 = 0;
			
			if (addlArg[r] == '/')
			{
				r++;
			}
			while (addlArg[r] && addlArg[r] != '/')
			{
				teststr[r2] = addlArg[r];
				r2++;
				r++;
			}
			teststr[r2] = '\0';

			if (r2 && strstr(teststr, "sound"))
			{ 
				//OK... we should have a relative sound path
				//try to register the sound
				objectID = cgi_S_RegisterSound(argument);
			}
			else
			{
				sprintf(errMsg, "Additional argument invalid sound path for type 'loop sfx <addlArg>'");
				goto functionend;
			}
		}
    }
    else if (strcmp(type, "USE") == 0)
    {	
        //try to cache the script
        Quake3Game()->PrecacheScript(argument);
    }
	else if (strcmp(type, "play") == 0)
	{
		//Handled by the ROFF Callback because no access to entity at this time
	}
    //else if ...
    else
    {
        if (type[0])
        {
            Com_Printf("Warning: \"%s\" is an invalid ROFF NoteTrack function\n", type);
        }
        else
        {
            Com_Printf("Warning: NoteTrack is missing function and/or arguments\n");
        }
    }

    return;
	
functionend:
	Com_Printf("Type-specific NoteTrack error: %s\n", errMsg);
	return;
}


//-------------------------------------------------------
// G_ValidRoff
//
// Checks header to verify we have a valid .ROF file
//-------------------------------------------------------

static qboolean G_ValidRoff( roff_hdr2_t *header )
{
	if ( !strncmp( header->mHeader, "ROFF", 4 ))
	{
		if ( LittleLong(header->mVersion) == ROFF_VERSION2 && LittleLong(header->mCount) > 0 )
		{
			return qtrue;
		}
		else if ( LittleLong(header->mVersion) == ROFF_VERSION && LittleFloat(((roff_hdr_t*)header)->mCount) > 0.0f )
		{ // version 1 defines the count as a float, so we best do the count check as a float or we'll get bogus results
			return qtrue;
		}
	}

	return qfalse;
}


//-------------------------------------------------------
// G_FreeRoff
//
// Deletes all .ROF files from memory
//-------------------------------------------------------

static void G_FreeRoff(int index)
{
	if(roffs[index].mNumNoteTracks) {
		delete [] roffs[index].mNoteTrackIndexes[0];
		delete [] roffs[index].mNoteTrackIndexes;
	}
}


//-------------------------------------------------------
// G_InitRoff
//
// Initializes the .ROF file
//-------------------------------------------------------

static qboolean G_InitRoff( char *file, unsigned char *data )
{
	roff_hdr_t *header = (roff_hdr_t *)data;
	int	count;
	int i;

	roffs[num_roffs].fileName = G_NewString( file );

	if ( LittleLong(header->mVersion) == ROFF_VERSION )
	{
		count = (int)LittleFloat(header->mCount);

		// We are Old School(tm)
		roffs[num_roffs].type = 1;

		roffs[num_roffs].data = (void *) G_Alloc( count * sizeof( move_rotate_t ) );
		move_rotate_t *mem	= (move_rotate_t *)roffs[num_roffs].data;

		roffs[num_roffs].mFrameTime = 100; // old school ones have a hard-coded frame time
		roffs[num_roffs].mLerp = 10;
		roffs[num_roffs].mNumNoteTracks = 0;
		roffs[num_roffs].mNoteTrackIndexes = NULL;

		if ( mem )
		{
			// The allocation worked, so stash this stuff off so we can reference the data later if needed
			roffs[num_roffs].frames		= count;

			// Step past the header to get to the goods
			move_rotate_t *roff_data = ( move_rotate_t *)&header[1];

			// Copy all of the goods into our ROFF cache
			for ( i = 0; i < count; i++, roff_data++, mem++ )
			{
				// Copy just the delta position and orientation which can be applied to anything at a later point
#ifdef Q3_BIG_ENDIAN
				mem->origin_delta[0] = LittleFloat(roff_data->origin_delta[0]);
				mem->origin_delta[1] = LittleFloat(roff_data->origin_delta[1]);
				mem->origin_delta[2] = LittleFloat(roff_data->origin_delta[2]);
				mem->rotate_delta[0] = LittleFloat(roff_data->rotate_delta[0]);
				mem->rotate_delta[1] = LittleFloat(roff_data->rotate_delta[1]);
				mem->rotate_delta[2] = LittleFloat(roff_data->rotate_delta[2]);
#else
				VectorCopy( roff_data->origin_delta, mem->origin_delta );
				VectorCopy( roff_data->rotate_delta, mem->rotate_delta );
#endif
			}
			return qtrue;
		}
	}
	else if ( LittleLong(header->mVersion) == ROFF_VERSION2 )
	{
		// Version 2.0, heck yeah!
		roff_hdr2_t *hdr = (roff_hdr2_t *)data;
		count = LittleLong(hdr->mCount);

		roffs[num_roffs].frames	= count;
		roffs[num_roffs].data	= (void *) G_Alloc( count * sizeof( move_rotate2_t ));
		move_rotate2_t *mem		= (move_rotate2_t *)roffs[num_roffs].data;

		if ( mem )
		{
			roffs[num_roffs].mFrameTime			= LittleLong(hdr->mFrameRate);
			roffs[num_roffs].mLerp				= 1000 / LittleLong(hdr->mFrameRate);
			roffs[num_roffs].mNumNoteTracks		= LittleLong(hdr->mNumNotes);

			if (roffs[num_roffs].mFrameTime < 50)
			{
				Com_Printf(S_COLOR_RED"Error: \"%s\" has an invalid ROFF framerate (%d < 50)\n", file, roffs[num_roffs].mFrameTime);
			}
			assert( roffs[num_roffs].mFrameTime >= 50 );//HAS to be at least 50 to be reliable

			 // Step past the header to get to the goods
			move_rotate2_t *roff_data = ( move_rotate2_t *)&hdr[1];

			roffs[num_roffs].type = 2; //rww - any reason this wasn't being set already?

			// Copy all of the goods into our ROFF cache
			for ( i = 0; i < count; i++ )
			{
#ifdef Q3_BIG_ENDIAN
				mem[i].origin_delta[0] = LittleFloat(roff_data[i].origin_delta[0]);
				mem[i].origin_delta[1] = LittleFloat(roff_data[i].origin_delta[1]);
				mem[i].origin_delta[2] = LittleFloat(roff_data[i].origin_delta[2]);
				mem[i].rotate_delta[0] = LittleFloat(roff_data[i].rotate_delta[0]);
				mem[i].rotate_delta[1] = LittleFloat(roff_data[i].rotate_delta[1]);
				mem[i].rotate_delta[2] = LittleFloat(roff_data[i].rotate_delta[2]);
#else
				VectorCopy( roff_data[i].origin_delta, mem[i].origin_delta );
				VectorCopy( roff_data[i].rotate_delta, mem[i].rotate_delta );
#endif

				mem[i].mStartNote = LittleLong(roff_data[i].mStartNote);
				mem[i].mNumNotes = LittleLong(roff_data[i].mNumNotes);
			}

			if ( LittleLong(hdr->mNumNotes) )
			{
				int		size;
				char	*ptr, *start;

				ptr = start = (char *)&roff_data[i];
				size = 0;

				for( i = 0; i < LittleLong(hdr->mNumNotes); i++ )
				{
					size += strlen(ptr) + 1;
					ptr += strlen(ptr) + 1;
				}

				// ? Get rid of dynamic memory ?
				roffs[num_roffs].mNoteTrackIndexes = new char *[LittleLong(hdr->mNumNotes)];
				ptr = roffs[num_roffs].mNoteTrackIndexes[0] = new char[size];
				memcpy(roffs[num_roffs].mNoteTrackIndexes[0], start, size);

				for( i = 1; i < LittleLong(hdr->mNumNotes); i++ )
				{
					ptr += strlen(ptr) + 1;
					roffs[num_roffs].mNoteTrackIndexes[i] = ptr;
				}
				
                //preCache NoteTracks
                for (i = 0; i < LittleLong(hdr->mNumNotes); i++)
                {
                    G_CacheRoffNoteTracks(roffs[num_roffs].mNoteTrackIndexes[i]);
                }
			}
			return qtrue;
		}
	}

	return qfalse;
}


//-------------------------------------------------------
// G_LoadRoff
//
// Does the fun work of loading and caching a roff file
//	If the file is already cached, it just returns an
//	ID to the cached file.
//-------------------------------------------------------

int G_LoadRoff( const char *fileName )
{
	char	file[MAX_QPATH];
	byte	*data;
	int		len, i, roff_id = 0;

	// Before even bothering with all of this, make sure we have a place to store it.
	if ( num_roffs >= MAX_ROFFS )
	{
		Com_Printf( S_COLOR_RED"MAX_ROFFS count exceeded.  Skipping load of .ROF '%s'\n", fileName );
		return roff_id;
	}

	// The actual path
	sprintf( file, "%s/%s.rof", Q3_SCRIPT_DIR, fileName );

	// See if I'm already precached
	for ( i = 0; i < num_roffs; i++ )
	{
		if ( Q_stricmp( file, roffs[i].fileName ) == 0 )
		{
			// Good, just return me...avoid zero index
			return i + 1;
		}
	}

#ifdef _DEBUG
//	Com_Printf( S_COLOR_GREEN"Caching ROF: '%s'\n", file );
#endif

	// Read the file in one fell swoop
	len = gi.FS_ReadFile( file, (void**) &data);

	if ( len <= 0 )
	{
		Com_Printf( S_COLOR_RED"Could not open .ROF file '%s'\n", fileName );
		return roff_id;
	}

	// Now let's check the header info...
	roff_hdr2_t *header = (roff_hdr2_t *)data;

	// ..and make sure it's reasonably valid
	if ( !G_ValidRoff( header ))
	{
		Com_Printf( S_COLOR_RED"Invalid .ROF format '%s'\n", fileName );
	}
	else
	{
		G_InitRoff( file, data );

		// Done loading this roff, so save off an id to it..increment first to avoid zero index
		roff_id = ++num_roffs;
	}

	gi.FS_FreeFile( data );

	return roff_id;
}


void G_FreeRoffs(void)
{
	while(num_roffs) {
		G_FreeRoff(num_roffs - 1);
		num_roffs--;
	}
}


//-------------------------------------------------------
// G_Roff
//
// Handles applying the roff data to the specified ent
//-------------------------------------------------------

void G_Roff( gentity_t *ent )
{
	if ( !ent->next_roff_time )
	{
		return;
	}

	if ( ent->next_roff_time > level.time )
	{// either I don't think or it's just not time to have me think yet
		return;
	}

	const int roff_id = G_LoadRoff( ent->roff );

	if ( !roff_id )
	{	// Couldn't cache this rof
		return;
	}

	// The ID is one higher than the array index
	const roff_list_t *  roff	= &roffs[ roff_id - 1 ];
	vec3_t	org, ang;

	if ( roff->type == 2 )
	{
		move_rotate2_t	*data	= &((move_rotate2_t *)roff->data)[ ent->roff_ctr ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );
		/*if (data->mStartNote != -1 || data->mNumNotes)
		{
			G_RoffNotetrackCallback(ent, roffs[roff_id - 1].mNoteTrackIndexes[data->mStartNote]);
		}*/
		if ( data->mStartNote != -1 )
		{
			for ( int n = 0; n < data->mNumNotes; n++ )
			{
				G_RoffNotetrackCallback(ent, roffs[roff_id - 1].mNoteTrackIndexes[data->mStartNote + n]);
			}
		}
	}
	else
	{
		move_rotate_t	*data	= &((move_rotate_t *)roff->data)[ ent->roff_ctr ];
		VectorCopy( data->origin_delta, org );
		VectorCopy( data->rotate_delta, ang );
	}

#ifdef _DEBUG
	if ( g_developer->integer )
	{
		Com_Printf( S_COLOR_GREEN"ROFF dat: num: %d o:<%.2f %.2f %.2f> a:<%.2f %.2f %.2f>\n",
					ent->roff_ctr,
					org[0], org[1], org[2],
					ang[0], ang[1], ang[2] );
	}
#endif

	if ( ent->client )
	{
		// Set up the angle interpolation
		//-------------------------------------
		VectorAdd( ent->s.apos.trBase, ang, ent->s.apos.trBase );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_INTERPOLATE;

		// Store what the next apos->trBase should be
		VectorCopy( ent->s.apos.trBase, ent->client->ps.viewangles );
		VectorCopy( ent->s.apos.trBase, ent->currentAngles );
		VectorCopy( ent->s.apos.trBase, ent->s.angles );
		if ( ent->NPC )
		{
			//ent->NPC->desiredPitch = ent->s.apos.trBase[PITCH];
			ent->NPC->desiredYaw = ent->s.apos.trBase[YAW];
		}

		// Set up the origin interpolation
		//-------------------------------------
		VectorAdd( ent->s.pos.trBase, org, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trType = TR_INTERPOLATE;

		// Store what the next pos->trBase should be
		VectorCopy( ent->s.pos.trBase, ent->client->ps.origin );
		VectorCopy( ent->s.pos.trBase, ent->currentOrigin );
		//VectorCopy( ent->s.pos.trBase, ent->s.origin );
	}
	else
	{
		// Set up the angle interpolation
		//-------------------------------------
		VectorScale( ang, roff->mLerp, ent->s.apos.trDelta );
		VectorCopy( ent->pos2, ent->s.apos.trBase );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_LINEAR;

		// Store what the next apos->trBase should be
		VectorAdd( ent->pos2, ang, ent->pos2 );

		// Set up the origin interpolation
		//-------------------------------------
		VectorScale( org, roff->mLerp, ent->s.pos.trDelta );
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trType = TR_LINEAR;

		// Store what the next apos->trBase should be
		VectorAdd( ent->pos1, org, ent->pos1 );

		//make it true linear... FIXME: sticks around after ROFF is done, but do we really care?
		ent->alt_fire = qtrue;

		if ( ent->e_ThinkFunc == thinkF_TieFighterThink || ent->e_ThinkFunc == thinkF_TieBomberThink ||
			( !ent->e_ThinkFunc
			&& ent->s.eType != ET_MISSILE
			&& ent->s.eType != ET_ITEM
			&& ent->s.eType != ET_MOVER ) )
		{//will never set currentAngles & currentOrigin itself ( why do we limit which one's get set?, just set all the time? )
			EvaluateTrajectory( &ent->s.apos, level.time, ent->currentAngles );
			EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
		}
	}

	// Link just in case.
	gi.linkentity( ent );

	// See if the ROFF playback is done
	//-------------------------------------
	if ( ++ent->roff_ctr >= roff->frames )
	{
		// We are done, so let me think no more, then tell the task that we're done.
		ent->next_roff_time = 0;

		// Stop any rotation or movement.
		VectorClear( ent->s.pos.trDelta );
		VectorClear( ent->s.apos.trDelta );

		Q3_TaskIDComplete( ent, TID_MOVE_NAV );

		return;
	}

	ent->next_roff_time = level.time + roff->mFrameTime;
}


//-------------------------------------------------------
// G_SaveCachedRoffs
//
// Really fun savegame stuff
//-------------------------------------------------------

void G_SaveCachedRoffs()
{
	int i, len = 0;

	ojk::SavedGameHelper saved_game(
		::gi.saved_game);

	// Write out the number of cached ROFFs
	saved_game.write_chunk<int32_t>(
		INT_ID('R', 'O', 'F', 'F'),
		::num_roffs);

	// Now dump out the cached ROFF file names in order so they can be loaded on the other end
	for ( i = 0; i < num_roffs; i++ )
	{
		// Dump out the string length to make things a bit easier on the other end...heh heh.
		len = strlen( roffs[i].fileName ) + 1;

		saved_game.write_chunk<int32_t>(
			INT_ID('S', 'L', 'E', 'N'),
			len);

		saved_game.write_chunk(
			INT_ID('R', 'S', 'T', 'R'),
			roffs[i].fileName,
			len);
	}
}


//-------------------------------------------------------
// G_LoadCachedRoffs
//
// Really fun loadgame stuff
//-------------------------------------------------------

void G_LoadCachedRoffs()
{
	int		i, count = 0, len = 0;
	char	buffer[MAX_QPATH];

	ojk::SavedGameHelper saved_game(
		::gi.saved_game);

	// Get the count of goodies we need to revive
	saved_game.read_chunk<int32_t>(
		INT_ID('R', 'O', 'F', 'F'),
		count);

	// Now bring 'em back to life
	for ( i = 0; i < count; i++ )
	{
		saved_game.read_chunk<int32_t>(
			INT_ID('S', 'L', 'E', 'N'),
			len);

		if (len < 0 || static_cast<size_t>(len) >= sizeof(buffer))
		{
			::G_Error("invalid length for RSTR string in save game: %d bytes\n", len);
		}

		saved_game.read_chunk(
			INT_ID('R', 'S', 'T', 'R'),
			buffer,
			len);

		G_LoadRoff( buffer );
	}
}
