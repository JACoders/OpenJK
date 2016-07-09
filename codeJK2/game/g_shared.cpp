#include "g_local.h"
#include "g_shared.h"
#include "qcommon/ojk_sg_wrappers.h"


void clientInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(infoValid, dst.infoValid);
    ::sg_export(name, dst.name);
    ::sg_export(team, dst.team);
    ::sg_export(score, dst.score);
    ::sg_export(handicap, dst.handicap);
    ::sg_export(legsModel, dst.legsModel);
    ::sg_export(legsSkin, dst.legsSkin);
    ::sg_export(torsoModel, dst.torsoModel);
    ::sg_export(torsoSkin, dst.torsoSkin);
    ::sg_export(headModel, dst.headModel);
    ::sg_export(headSkin, dst.headSkin);
    ::sg_export(extensions, dst.extensions);
    ::sg_export(animFileIndex, dst.animFileIndex);
    ::sg_export(sounds, dst.sounds);
    ::sg_export(customBasicSoundDir, dst.customBasicSoundDir);
    ::sg_export(customCombatSoundDir, dst.customCombatSoundDir);
    ::sg_export(customExtraSoundDir, dst.customExtraSoundDir);
    ::sg_export(customJediSoundDir, dst.customJediSoundDir);
}

void clientInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.infoValid, infoValid);
    ::sg_import(src.name, name);
    ::sg_import(src.team, team);
    ::sg_import(src.score, score);
    ::sg_import(src.handicap, handicap);
    ::sg_import(src.legsModel, legsModel);
    ::sg_import(src.legsSkin, legsSkin);
    ::sg_import(src.torsoModel, torsoModel);
    ::sg_import(src.torsoSkin, torsoSkin);
    ::sg_import(src.headModel, headModel);
    ::sg_import(src.headSkin, headSkin);
    ::sg_import(src.extensions, extensions);
    ::sg_import(src.animFileIndex, animFileIndex);
    ::sg_import(src.sounds, sounds);
    ::sg_import(src.customBasicSoundDir, customBasicSoundDir);
    ::sg_import(src.customCombatSoundDir, customCombatSoundDir);
    ::sg_import(src.customExtraSoundDir, customExtraSoundDir);
    ::sg_import(src.customJediSoundDir, customJediSoundDir);
}


void modelInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(modelIndex, dst.modelIndex);
    ::sg_export(customRGB, dst.customRGB);
    ::sg_export(customAlpha, dst.customAlpha);
}

void modelInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.modelIndex, modelIndex);
    ::sg_import(src.customRGB, customRGB);
    ::sg_import(src.customAlpha, customAlpha);
}


void renderInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(legsModel, dst.legsModel);
    ::sg_export(legsModelName, dst.legsModelName);
    ::sg_export(torsoModel, dst.torsoModel);
    ::sg_export(headModel, dst.headModel);
    ::sg_export(torsoModelName, dst.torsoModelName);
    ::sg_export(headModelName, dst.headModelName);
    ::sg_export(headYawRangeLeft, dst.headYawRangeLeft);
    ::sg_export(headYawRangeRight, dst.headYawRangeRight);
    ::sg_export(headPitchRangeUp, dst.headPitchRangeUp);
    ::sg_export(headPitchRangeDown, dst.headPitchRangeDown);
    ::sg_export(torsoYawRangeLeft, dst.torsoYawRangeLeft);
    ::sg_export(torsoYawRangeRight, dst.torsoYawRangeRight);
    ::sg_export(torsoPitchRangeUp, dst.torsoPitchRangeUp);
    ::sg_export(torsoPitchRangeDown, dst.torsoPitchRangeDown);
    ::sg_export(legsFrame, dst.legsFrame);
    ::sg_export(torsoFrame, dst.torsoFrame);
    ::sg_export(legsFpsMod, dst.legsFpsMod);
    ::sg_export(torsoFpsMod, dst.torsoFpsMod);
    ::sg_export(customRGB, dst.customRGB);
    ::sg_export(customAlpha, dst.customAlpha);
    ::sg_export(renderFlags, dst.renderFlags);
    ::sg_export(muzzlePoint, dst.muzzlePoint);
    ::sg_export(muzzleDir, dst.muzzleDir);
    ::sg_export(muzzlePointOld, dst.muzzlePointOld);
    ::sg_export(muzzleDirOld, dst.muzzleDirOld);
    ::sg_export(mPCalcTime, dst.mPCalcTime);
    ::sg_export(lockYaw, dst.lockYaw);
    ::sg_export(headPoint, dst.headPoint);
    ::sg_export(headAngles, dst.headAngles);
    ::sg_export(handRPoint, dst.handRPoint);
    ::sg_export(handLPoint, dst.handLPoint);
    ::sg_export(crotchPoint, dst.crotchPoint);
    ::sg_export(footRPoint, dst.footRPoint);
    ::sg_export(footLPoint, dst.footLPoint);
    ::sg_export(torsoPoint, dst.torsoPoint);
    ::sg_export(torsoAngles, dst.torsoAngles);
    ::sg_export(eyePoint, dst.eyePoint);
    ::sg_export(eyeAngles, dst.eyeAngles);
    ::sg_export(lookTarget, dst.lookTarget);
    ::sg_export(lookMode, dst.lookMode);
    ::sg_export(lookTargetClearTime, dst.lookTargetClearTime);
    ::sg_export(lastVoiceVolume, dst.lastVoiceVolume);
    ::sg_export(lastHeadAngles, dst.lastHeadAngles);
    ::sg_export(headBobAngles, dst.headBobAngles);
    ::sg_export(targetHeadBobAngles, dst.targetHeadBobAngles);
    ::sg_export(lookingDebounceTime, dst.lookingDebounceTime);
    ::sg_export(legsYaw, dst.legsYaw);
}

void renderInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.legsModel, legsModel);
    ::sg_import(src.legsModelName, legsModelName);
    ::sg_import(src.torsoModel, torsoModel);
    ::sg_import(src.headModel, headModel);
    ::sg_import(src.torsoModelName, torsoModelName);
    ::sg_import(src.headModelName, headModelName);
    ::sg_import(src.headYawRangeLeft, headYawRangeLeft);
    ::sg_import(src.headYawRangeRight, headYawRangeRight);
    ::sg_import(src.headPitchRangeUp, headPitchRangeUp);
    ::sg_import(src.headPitchRangeDown, headPitchRangeDown);
    ::sg_import(src.torsoYawRangeLeft, torsoYawRangeLeft);
    ::sg_import(src.torsoYawRangeRight, torsoYawRangeRight);
    ::sg_import(src.torsoPitchRangeUp, torsoPitchRangeUp);
    ::sg_import(src.torsoPitchRangeDown, torsoPitchRangeDown);
    ::sg_import(src.legsFrame, legsFrame);
    ::sg_import(src.torsoFrame, torsoFrame);
    ::sg_import(src.legsFpsMod, legsFpsMod);
    ::sg_import(src.torsoFpsMod, torsoFpsMod);
    ::sg_import(src.customRGB, customRGB);
    ::sg_import(src.customAlpha, customAlpha);
    ::sg_import(src.renderFlags, renderFlags);
    ::sg_import(src.muzzlePoint, muzzlePoint);
    ::sg_import(src.muzzleDir, muzzleDir);
    ::sg_import(src.muzzlePointOld, muzzlePointOld);
    ::sg_import(src.muzzleDirOld, muzzleDirOld);
    ::sg_import(src.mPCalcTime, mPCalcTime);
    ::sg_import(src.lockYaw, lockYaw);
    ::sg_import(src.headPoint, headPoint);
    ::sg_import(src.headAngles, headAngles);
    ::sg_import(src.handRPoint, handRPoint);
    ::sg_import(src.handLPoint, handLPoint);
    ::sg_import(src.crotchPoint, crotchPoint);
    ::sg_import(src.footRPoint, footRPoint);
    ::sg_import(src.footLPoint, footLPoint);
    ::sg_import(src.torsoPoint, torsoPoint);
    ::sg_import(src.torsoAngles, torsoAngles);
    ::sg_import(src.eyePoint, eyePoint);
    ::sg_import(src.eyeAngles, eyeAngles);
    ::sg_import(src.lookTarget, lookTarget);
    ::sg_import(src.lookMode, lookMode);
    ::sg_import(src.lookTargetClearTime, lookTargetClearTime);
    ::sg_import(src.lastVoiceVolume, lastVoiceVolume);
    ::sg_import(src.lastHeadAngles, lastHeadAngles);
    ::sg_import(src.headBobAngles, headBobAngles);
    ::sg_import(src.targetHeadBobAngles, targetHeadBobAngles);
    ::sg_import(src.lookingDebounceTime, lookingDebounceTime);
    ::sg_import(src.legsYaw, legsYaw);
}


void playerTeamState_t::sg_export(
    SgType& dst) const
{
    ::sg_export(state, dst.state);
    ::sg_export(captures, dst.captures);
    ::sg_export(basedefense, dst.basedefense);
    ::sg_export(carrierdefense, dst.carrierdefense);
    ::sg_export(flagrecovery, dst.flagrecovery);
    ::sg_export(fragcarrier, dst.fragcarrier);
    ::sg_export(assists, dst.assists);
    ::sg_export(lasthurtcarrier, dst.lasthurtcarrier);
    ::sg_export(lastreturnedflag, dst.lastreturnedflag);
    ::sg_export(flagsince, dst.flagsince);
    ::sg_export(lastfraggedcarrier, dst.lastfraggedcarrier);
}

void playerTeamState_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.state, state);
    ::sg_import(src.captures, captures);
    ::sg_import(src.basedefense, basedefense);
    ::sg_import(src.carrierdefense, carrierdefense);
    ::sg_import(src.flagrecovery, flagrecovery);
    ::sg_import(src.fragcarrier, fragcarrier);
    ::sg_import(src.assists, assists);
    ::sg_import(src.lasthurtcarrier, lasthurtcarrier);
    ::sg_import(src.lastreturnedflag, lastreturnedflag);
    ::sg_import(src.flagsince, flagsince);
    ::sg_import(src.lastfraggedcarrier, lastfraggedcarrier);
}


void objectives_t::sg_export(
    SgType& dst) const
{
    ::sg_export(display, dst.display);
    ::sg_export(status, dst.status);
}

void objectives_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.display, display);
    ::sg_import(src.status, status);
}


void missionStats_t::sg_export(
    SgType& dst) const
{
    ::sg_export(secretsFound, dst.secretsFound);
    ::sg_export(totalSecrets, dst.totalSecrets);
    ::sg_export(shotsFired, dst.shotsFired);
    ::sg_export(hits, dst.hits);
    ::sg_export(enemiesSpawned, dst.enemiesSpawned);
    ::sg_export(enemiesKilled, dst.enemiesKilled);
    ::sg_export(saberThrownCnt, dst.saberThrownCnt);
    ::sg_export(saberBlocksCnt, dst.saberBlocksCnt);
    ::sg_export(legAttacksCnt, dst.legAttacksCnt);
    ::sg_export(armAttacksCnt, dst.armAttacksCnt);
    ::sg_export(torsoAttacksCnt, dst.torsoAttacksCnt);
    ::sg_export(otherAttacksCnt, dst.otherAttacksCnt);
    ::sg_export(forceUsed, dst.forceUsed);
    ::sg_export(weaponUsed, dst.weaponUsed);
}

void missionStats_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.secretsFound, secretsFound);
    ::sg_import(src.totalSecrets, totalSecrets);
    ::sg_import(src.shotsFired, shotsFired);
    ::sg_import(src.hits, hits);
    ::sg_import(src.enemiesSpawned, enemiesSpawned);
    ::sg_import(src.enemiesKilled, enemiesKilled);
    ::sg_import(src.saberThrownCnt, saberThrownCnt);
    ::sg_import(src.saberBlocksCnt, saberBlocksCnt);
    ::sg_import(src.legAttacksCnt, legAttacksCnt);
    ::sg_import(src.armAttacksCnt, armAttacksCnt);
    ::sg_import(src.torsoAttacksCnt, torsoAttacksCnt);
    ::sg_import(src.otherAttacksCnt, otherAttacksCnt);
    ::sg_import(src.forceUsed, forceUsed);
    ::sg_import(src.weaponUsed, weaponUsed);
}


void clientSession_t::sg_export(
    SgType& dst) const
{
    ::sg_export(missionObjectivesShown, dst.missionObjectivesShown);
    ::sg_export(sessionTeam, dst.sessionTeam);
    ::sg_export(mission_objectives, dst.mission_objectives);
    ::sg_export(missionStats, dst.missionStats);
}

void clientSession_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.missionObjectivesShown, missionObjectivesShown);
    ::sg_import(src.sessionTeam, sessionTeam);
    ::sg_import(src.mission_objectives, mission_objectives);
    ::sg_import(src.missionStats, missionStats);
}


void clientPersistant_t::sg_export(
    SgType& dst) const
{
    ::sg_export(connected, dst.connected);
    ::sg_export(lastCommand, dst.lastCommand);
    ::sg_export(localClient, dst.localClient);
    ::sg_export(netname, dst.netname);
    ::sg_export(maxHealth, dst.maxHealth);
    ::sg_export(enterTime, dst.enterTime);
    ::sg_export(cmd_angles, dst.cmd_angles);
    ::sg_export(teamState, dst.teamState);
}

void clientPersistant_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.connected, connected);
    ::sg_import(src.lastCommand, lastCommand);
    ::sg_import(src.localClient, localClient);
    ::sg_import(src.netname, netname);
    ::sg_import(src.maxHealth, maxHealth);
    ::sg_import(src.enterTime, enterTime);
    ::sg_import(src.cmd_angles, cmd_angles);
    ::sg_import(src.teamState, teamState);
}


void gclient_t::sg_export(
    SgType& dst) const
{
    ::sg_export(ps, dst.ps);
    ::sg_export(pers, dst.pers);
    ::sg_export(sess, dst.sess);
    ::sg_export(noclip, dst.noclip);
    ::sg_export(lastCmdTime, dst.lastCmdTime);
    ::sg_export(usercmd, dst.usercmd);
    ::sg_export(buttons, dst.buttons);
    ::sg_export(oldbuttons, dst.oldbuttons);
    ::sg_export(latched_buttons, dst.latched_buttons);
    ::sg_export(damage_armor, dst.damage_armor);
    ::sg_export(damage_blood, dst.damage_blood);
    ::sg_export(damage_knockback, dst.damage_knockback);
    ::sg_export(damage_from, dst.damage_from);
    ::sg_export(damage_fromWorld, dst.damage_fromWorld);
    ::sg_export(accurateCount, dst.accurateCount);
    ::sg_export(respawnTime, dst.respawnTime);
    ::sg_export(inactivityTime, dst.inactivityTime);
    ::sg_export(inactivityWarning, dst.inactivityWarning);
    ::sg_export(idleTime, dst.idleTime);
    ::sg_export(airOutTime, dst.airOutTime);
    ::sg_export(timeResidual, dst.timeResidual);
    ::sg_export(facial_blink, dst.facial_blink);
    ::sg_export(facial_frown, dst.facial_frown);
    ::sg_export(facial_aux, dst.facial_aux);
    ::sg_export(clientInfo, dst.clientInfo);
    ::sg_export(forced_forwardmove, dst.forced_forwardmove);
    ::sg_export(forced_rightmove, dst.forced_rightmove);
    ::sg_export(fireDelay, dst.fireDelay);
    ::sg_export(playerTeam, dst.playerTeam);
    ::sg_export(enemyTeam, dst.enemyTeam);
    ::sg_export(squadname, dst.squadname);
    ::sg_export(team_leader, dst.team_leader);
    ::sg_export(leader, dst.leader);
    ::sg_export(follower, dst.follower);
    ::sg_export(numFollowers, dst.numFollowers);
    ::sg_export(formationGoal, dst.formationGoal);
    ::sg_export(nextFormGoal, dst.nextFormGoal);
    ::sg_export(NPC_class, dst.NPC_class);
    ::sg_export(hiddenDist, dst.hiddenDist);
    ::sg_export(hiddenDir, dst.hiddenDir);
    ::sg_export(renderInfo, dst.renderInfo);
    ::sg_export(saberTrail, dst.saberTrail);
    ::sg_export(dismembered, dst.dismembered);
    ::sg_export(dismemberProbLegs, dst.dismemberProbLegs);
    ::sg_export(dismemberProbHead, dst.dismemberProbHead);
    ::sg_export(dismemberProbArms, dst.dismemberProbArms);
    ::sg_export(dismemberProbHands, dst.dismemberProbHands);
    ::sg_export(dismemberProbWaist, dst.dismemberProbWaist);
    ::sg_export(standheight, dst.standheight);
    ::sg_export(crouchheight, dst.crouchheight);
    ::sg_export(poisonDamage, dst.poisonDamage);
    ::sg_export(poisonTime, dst.poisonTime);
    ::sg_export(slopeRecalcTime, dst.slopeRecalcTime);
    ::sg_export(pushVec, dst.pushVec);
    ::sg_export(pushVecTime, dst.pushVecTime);
}

void gclient_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.ps, ps);
    ::sg_import(src.pers, pers);
    ::sg_import(src.sess, sess);
    ::sg_import(src.noclip, noclip);
    ::sg_import(src.lastCmdTime, lastCmdTime);
    ::sg_import(src.usercmd, usercmd);
    ::sg_import(src.buttons, buttons);
    ::sg_import(src.oldbuttons, oldbuttons);
    ::sg_import(src.latched_buttons, latched_buttons);
    ::sg_import(src.damage_armor, damage_armor);
    ::sg_import(src.damage_blood, damage_blood);
    ::sg_import(src.damage_knockback, damage_knockback);
    ::sg_import(src.damage_from, damage_from);
    ::sg_import(src.damage_fromWorld, damage_fromWorld);
    ::sg_import(src.accurateCount, accurateCount);
    ::sg_import(src.respawnTime, respawnTime);
    ::sg_import(src.inactivityTime, inactivityTime);
    ::sg_import(src.inactivityWarning, inactivityWarning);
    ::sg_import(src.idleTime, idleTime);
    ::sg_import(src.airOutTime, airOutTime);
    ::sg_import(src.timeResidual, timeResidual);
    ::sg_import(src.facial_blink, facial_blink);
    ::sg_import(src.facial_frown, facial_frown);
    ::sg_import(src.facial_aux, facial_aux);
    ::sg_import(src.clientInfo, clientInfo);
    ::sg_import(src.forced_forwardmove, forced_forwardmove);
    ::sg_import(src.forced_rightmove, forced_rightmove);
    ::sg_import(src.fireDelay, fireDelay);
    ::sg_import(src.playerTeam, playerTeam);
    ::sg_import(src.enemyTeam, enemyTeam);
    ::sg_import(src.squadname, squadname);
    ::sg_import(src.team_leader, team_leader);
    ::sg_import(src.leader, leader);
    ::sg_import(src.follower, follower);
    ::sg_import(src.numFollowers, numFollowers);
    ::sg_import(src.formationGoal, formationGoal);
    ::sg_import(src.nextFormGoal, nextFormGoal);
    ::sg_import(src.NPC_class, NPC_class);
    ::sg_import(src.hiddenDist, hiddenDist);
    ::sg_import(src.hiddenDir, hiddenDir);
    ::sg_import(src.renderInfo, renderInfo);
    ::sg_import(src.saberTrail, saberTrail);
    ::sg_import(src.dismembered, dismembered);
    ::sg_import(src.dismemberProbLegs, dismemberProbLegs);
    ::sg_import(src.dismemberProbHead, dismemberProbHead);
    ::sg_import(src.dismemberProbArms, dismemberProbArms);
    ::sg_import(src.dismemberProbHands, dismemberProbHands);
    ::sg_import(src.dismemberProbWaist, dismemberProbWaist);
    ::sg_import(src.standheight, standheight);
    ::sg_import(src.crouchheight, crouchheight);
    ::sg_import(src.poisonDamage, poisonDamage);
    ::sg_import(src.poisonTime, poisonTime);
    ::sg_import(src.slopeRecalcTime, slopeRecalcTime);
    ::sg_import(src.pushVec, pushVec);
    ::sg_import(src.pushVecTime, pushVecTime);
}


void parms_t::sg_export(
    SgType& dst) const
{
    ::sg_export(parm, dst.parm);
}

void parms_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.parm, parm);
}


void gentity_s::sg_export(
    SgType& dst) const
{
    ::sg_export(s, dst.s);
    ::sg_export(client, dst.client);
    ::sg_export(inuse, dst.inuse);
    ::sg_export(linked, dst.linked);
    ::sg_export(svFlags, dst.svFlags);
    ::sg_export(bmodel, dst.bmodel);
    ::sg_export(mins, dst.mins);
    ::sg_export(maxs, dst.maxs);
    ::sg_export(contents, dst.contents);
    ::sg_export(absmin, dst.absmin);
    ::sg_export(absmax, dst.absmax);
    ::sg_export(currentOrigin, dst.currentOrigin);
    ::sg_export(currentAngles, dst.currentAngles);
    ::sg_export(owner, dst.owner);
    ::sg_export(ghoul2, dst.ghoul2);
    ::sg_export(classname, dst.classname);
    ::sg_export(spawnflags, dst.spawnflags);
    ::sg_export(flags, dst.flags);
    ::sg_export(model, dst.model);
    ::sg_export(model2, dst.model2);
    ::sg_export(freetime, dst.freetime);
    ::sg_export(eventTime, dst.eventTime);
    ::sg_export(freeAfterEvent, dst.freeAfterEvent);
    ::sg_export(unlinkAfterEvent, dst.unlinkAfterEvent);
    ::sg_export(physicsBounce, dst.physicsBounce);
    ::sg_export(clipmask, dst.clipmask);
    ::sg_export(speed, dst.speed);
    ::sg_export(movedir, dst.movedir);
    ::sg_export(lastOrigin, dst.lastOrigin);
    ::sg_export(lastAngles, dst.lastAngles);
    ::sg_export(mass, dst.mass);
    ::sg_export(lastImpact, dst.lastImpact);
    ::sg_export(watertype, dst.watertype);
    ::sg_export(waterlevel, dst.waterlevel);
    ::sg_export(angle, dst.angle);
    ::sg_export(target, dst.target);
    ::sg_export(target2, dst.target2);
    ::sg_export(target3, dst.target3);
    ::sg_export(target4, dst.target4);
    ::sg_export(targetname, dst.targetname);
    ::sg_export(team, dst.team);
    ::sg_export(roff, dst.roff);
    ::sg_export(roff_ctr, dst.roff_ctr);
    ::sg_export(next_roff_time, dst.next_roff_time);
    ::sg_export(fx_time, dst.fx_time);
    ::sg_export(nextthink, dst.nextthink);
    ::sg_export(e_ThinkFunc, dst.e_ThinkFunc);
    ::sg_export(e_clThinkFunc, dst.e_clThinkFunc);
    ::sg_export(e_ReachedFunc, dst.e_ReachedFunc);
    ::sg_export(e_BlockedFunc, dst.e_BlockedFunc);
    ::sg_export(e_TouchFunc, dst.e_TouchFunc);
    ::sg_export(e_UseFunc, dst.e_UseFunc);
    ::sg_export(e_PainFunc, dst.e_PainFunc);
    ::sg_export(e_DieFunc, dst.e_DieFunc);
    ::sg_export(health, dst.health);
    ::sg_export(max_health, dst.max_health);
    ::sg_export(takedamage, dst.takedamage);
    ::sg_export(material, dst.material);
    ::sg_export(damage, dst.damage);
    ::sg_export(dflags, dst.dflags);
    ::sg_export(splashDamage, dst.splashDamage);
    ::sg_export(splashRadius, dst.splashRadius);
    ::sg_export(methodOfDeath, dst.methodOfDeath);
    ::sg_export(splashMethodOfDeath, dst.splashMethodOfDeath);
    ::sg_export(locationDamage, dst.locationDamage);
    ::sg_export(chain, dst.chain);
    ::sg_export(enemy, dst.enemy);
    ::sg_export(activator, dst.activator);
    ::sg_export(teamchain, dst.teamchain);
    ::sg_export(teammaster, dst.teammaster);
    ::sg_export(lastEnemy, dst.lastEnemy);
    ::sg_export(wait, dst.wait);
    ::sg_export(random, dst.random);
    ::sg_export(delay, dst.delay);
    ::sg_export(alt_fire, dst.alt_fire);
    ::sg_export(count, dst.count);
    ::sg_export(bounceCount, dst.bounceCount);
    ::sg_export(fly_sound_debounce_time, dst.fly_sound_debounce_time);
    ::sg_export(painDebounceTime, dst.painDebounceTime);
    ::sg_export(disconnectDebounceTime, dst.disconnectDebounceTime);
    ::sg_export(attackDebounceTime, dst.attackDebounceTime);
    ::sg_export(pushDebounceTime, dst.pushDebounceTime);
    ::sg_export(aimDebounceTime, dst.aimDebounceTime);
    ::sg_export(useDebounceTime, dst.useDebounceTime);
    ::sg_export(trigger_formation, dst.trigger_formation);
    ::sg_export(waypoint, dst.waypoint);
    ::sg_export(lastWaypoint, dst.lastWaypoint);
    ::sg_export(lastValidWaypoint, dst.lastValidWaypoint);
    ::sg_export(noWaypointTime, dst.noWaypointTime);
    ::sg_export(combatPoint, dst.combatPoint);
    ::sg_export(failedWaypoints, dst.failedWaypoints);
    ::sg_export(failedWaypointCheckTime, dst.failedWaypointCheckTime);
    ::sg_export(loopAnim, dst.loopAnim);
    ::sg_export(startFrame, dst.startFrame);
    ::sg_export(endFrame, dst.endFrame);
    ::sg_export(sequencer, dst.sequencer);
    ::sg_export(taskManager, dst.taskManager);
    ::sg_export(taskID, dst.taskID);
    ::sg_export(parms, dst.parms);
    ::sg_export(behaviorSet, dst.behaviorSet);
    ::sg_export(script_targetname, dst.script_targetname);
    ::sg_export(delayScriptTime, dst.delayScriptTime);
    ::sg_export(fullName, dst.fullName);
    ::sg_export(soundSet, dst.soundSet);
    ::sg_export(setTime, dst.setTime);
    ::sg_export(cameraGroup, dst.cameraGroup);
    ::sg_export(noDamageTeam, dst.noDamageTeam);
    ::sg_export(playerModel, dst.playerModel);
    ::sg_export(weaponModel, dst.weaponModel);
    ::sg_export(handRBolt, dst.handRBolt);
    ::sg_export(handLBolt, dst.handLBolt);
    ::sg_export(headBolt, dst.headBolt);
    ::sg_export(cervicalBolt, dst.cervicalBolt);
    ::sg_export(chestBolt, dst.chestBolt);
    ::sg_export(gutBolt, dst.gutBolt);
    ::sg_export(torsoBolt, dst.torsoBolt);
    ::sg_export(crotchBolt, dst.crotchBolt);
    ::sg_export(motionBolt, dst.motionBolt);
    ::sg_export(kneeLBolt, dst.kneeLBolt);
    ::sg_export(kneeRBolt, dst.kneeRBolt);
    ::sg_export(elbowLBolt, dst.elbowLBolt);
    ::sg_export(elbowRBolt, dst.elbowRBolt);
    ::sg_export(footLBolt, dst.footLBolt);
    ::sg_export(footRBolt, dst.footRBolt);
    ::sg_export(faceBone, dst.faceBone);
    ::sg_export(craniumBone, dst.craniumBone);
    ::sg_export(cervicalBone, dst.cervicalBone);
    ::sg_export(thoracicBone, dst.thoracicBone);
    ::sg_export(upperLumbarBone, dst.upperLumbarBone);
    ::sg_export(lowerLumbarBone, dst.lowerLumbarBone);
    ::sg_export(hipsBone, dst.hipsBone);
    ::sg_export(motionBone, dst.motionBone);
    ::sg_export(rootBone, dst.rootBone);
    ::sg_export(footLBone, dst.footLBone);
    ::sg_export(footRBone, dst.footRBone);
    ::sg_export(genericBone1, dst.genericBone1);
    ::sg_export(genericBone2, dst.genericBone2);
    ::sg_export(genericBone3, dst.genericBone3);
    ::sg_export(genericBolt1, dst.genericBolt1);
    ::sg_export(genericBolt2, dst.genericBolt2);
    ::sg_export(genericBolt3, dst.genericBolt3);
    ::sg_export(genericBolt4, dst.genericBolt4);
    ::sg_export(genericBolt5, dst.genericBolt5);
    ::sg_export(cinematicModel, dst.cinematicModel);
    ::sg_export(NPC, dst.NPC);
    ::sg_export(ownername, dst.ownername);
    ::sg_export(cantHitEnemyCounter, dst.cantHitEnemyCounter);
    ::sg_export(NPC_type, dst.NPC_type);
    ::sg_export(NPC_targetname, dst.NPC_targetname);
    ::sg_export(NPC_target, dst.NPC_target);
    ::sg_export(moverState, dst.moverState);
    ::sg_export(soundPos1, dst.soundPos1);
    ::sg_export(sound1to2, dst.sound1to2);
    ::sg_export(sound2to1, dst.sound2to1);
    ::sg_export(soundPos2, dst.soundPos2);
    ::sg_export(soundLoop, dst.soundLoop);
    ::sg_export(nextTrain, dst.nextTrain);
    ::sg_export(prevTrain, dst.prevTrain);
    ::sg_export(pos1, dst.pos1);
    ::sg_export(pos2, dst.pos2);
    ::sg_export(pos3, dst.pos3);
    ::sg_export(sounds, dst.sounds);
    ::sg_export(closetarget, dst.closetarget);
    ::sg_export(opentarget, dst.opentarget);
    ::sg_export(paintarget, dst.paintarget);
    ::sg_export(lockCount, dst.lockCount);
    ::sg_export(radius, dst.radius);
    ::sg_export(wpIndex, dst.wpIndex);
    ::sg_export(noise_index, dst.noise_index);
    ::sg_export(startRGBA, dst.startRGBA);
    ::sg_export(finalRGBA, dst.finalRGBA);
    ::sg_export(item, dst.item);
    ::sg_export(message, dst.message);
    ::sg_export(lightLevel, dst.lightLevel);
    ::sg_export(forcePushTime, dst.forcePushTime);
    ::sg_export(forcePuller, dst.forcePuller);
}

void gentity_s::sg_import(
    const SgType& src)
{
    ::sg_import(src.s, s);
    ::sg_import(src.client, client);
    ::sg_import(src.inuse, inuse);
    ::sg_import(src.linked, linked);
    ::sg_import(src.svFlags, svFlags);
    ::sg_import(src.bmodel, bmodel);
    ::sg_import(src.mins, mins);
    ::sg_import(src.maxs, maxs);
    ::sg_import(src.contents, contents);
    ::sg_import(src.absmin, absmin);
    ::sg_import(src.absmax, absmax);
    ::sg_import(src.currentOrigin, currentOrigin);
    ::sg_import(src.currentAngles, currentAngles);
    ::sg_import(src.owner, owner);
    ::sg_import(src.ghoul2, ghoul2);
    ::sg_import(src.classname, classname);
    ::sg_import(src.spawnflags, spawnflags);
    ::sg_import(src.flags, flags);
    ::sg_import(src.model, model);
    ::sg_import(src.model2, model2);
    ::sg_import(src.freetime, freetime);
    ::sg_import(src.eventTime, eventTime);
    ::sg_import(src.freeAfterEvent, freeAfterEvent);
    ::sg_import(src.unlinkAfterEvent, unlinkAfterEvent);
    ::sg_import(src.physicsBounce, physicsBounce);
    ::sg_import(src.clipmask, clipmask);
    ::sg_import(src.speed, speed);
    ::sg_import(src.movedir, movedir);
    ::sg_import(src.lastOrigin, lastOrigin);
    ::sg_import(src.lastAngles, lastAngles);
    ::sg_import(src.mass, mass);
    ::sg_import(src.lastImpact, lastImpact);
    ::sg_import(src.watertype, watertype);
    ::sg_import(src.waterlevel, waterlevel);
    ::sg_import(src.angle, angle);
    ::sg_import(src.target, target);
    ::sg_import(src.target2, target2);
    ::sg_import(src.target3, target3);
    ::sg_import(src.target4, target4);
    ::sg_import(src.targetname, targetname);
    ::sg_import(src.team, team);
    ::sg_import(src.roff, roff);
    ::sg_import(src.roff_ctr, roff_ctr);
    ::sg_import(src.next_roff_time, next_roff_time);
    ::sg_import(src.fx_time, fx_time);
    ::sg_import(src.nextthink, nextthink);
    ::sg_import(src.e_ThinkFunc, e_ThinkFunc);
    ::sg_import(src.e_clThinkFunc, e_clThinkFunc);
    ::sg_import(src.e_ReachedFunc, e_ReachedFunc);
    ::sg_import(src.e_BlockedFunc, e_BlockedFunc);
    ::sg_import(src.e_TouchFunc, e_TouchFunc);
    ::sg_import(src.e_UseFunc, e_UseFunc);
    ::sg_import(src.e_PainFunc, e_PainFunc);
    ::sg_import(src.e_DieFunc, e_DieFunc);
    ::sg_import(src.health, health);
    ::sg_import(src.max_health, max_health);
    ::sg_import(src.takedamage, takedamage);
    ::sg_import(src.material, material);
    ::sg_import(src.damage, damage);
    ::sg_import(src.dflags, dflags);
    ::sg_import(src.splashDamage, splashDamage);
    ::sg_import(src.splashRadius, splashRadius);
    ::sg_import(src.methodOfDeath, methodOfDeath);
    ::sg_import(src.splashMethodOfDeath, splashMethodOfDeath);
    ::sg_import(src.locationDamage, locationDamage);
    ::sg_import(src.chain, chain);
    ::sg_import(src.enemy, enemy);
    ::sg_import(src.activator, activator);
    ::sg_import(src.teamchain, teamchain);
    ::sg_import(src.teammaster, teammaster);
    ::sg_import(src.lastEnemy, lastEnemy);
    ::sg_import(src.wait, wait);
    ::sg_import(src.random, random);
    ::sg_import(src.delay, delay);
    ::sg_import(src.alt_fire, alt_fire);
    ::sg_import(src.count, count);
    ::sg_import(src.bounceCount, bounceCount);
    ::sg_import(src.fly_sound_debounce_time, fly_sound_debounce_time);
    ::sg_import(src.painDebounceTime, painDebounceTime);
    ::sg_import(src.disconnectDebounceTime, disconnectDebounceTime);
    ::sg_import(src.attackDebounceTime, attackDebounceTime);
    ::sg_import(src.pushDebounceTime, pushDebounceTime);
    ::sg_import(src.aimDebounceTime, aimDebounceTime);
    ::sg_import(src.useDebounceTime, useDebounceTime);
    ::sg_import(src.trigger_formation, trigger_formation);
    ::sg_import(src.waypoint, waypoint);
    ::sg_import(src.lastWaypoint, lastWaypoint);
    ::sg_import(src.lastValidWaypoint, lastValidWaypoint);
    ::sg_import(src.noWaypointTime, noWaypointTime);
    ::sg_import(src.combatPoint, combatPoint);
    ::sg_import(src.failedWaypoints, failedWaypoints);
    ::sg_import(src.failedWaypointCheckTime, failedWaypointCheckTime);
    ::sg_import(src.loopAnim, loopAnim);
    ::sg_import(src.startFrame, startFrame);
    ::sg_import(src.endFrame, endFrame);
    ::sg_import(src.sequencer, sequencer);
    ::sg_import(src.taskManager, taskManager);
    ::sg_import(src.taskID, taskID);
    ::sg_import(src.parms, parms);
    ::sg_import(src.behaviorSet, behaviorSet);
    ::sg_import(src.script_targetname, script_targetname);
    ::sg_import(src.delayScriptTime, delayScriptTime);
    ::sg_import(src.fullName, fullName);
    ::sg_import(src.soundSet, soundSet);
    ::sg_import(src.setTime, setTime);
    ::sg_import(src.cameraGroup, cameraGroup);
    ::sg_import(src.noDamageTeam, noDamageTeam);
    ::sg_import(src.playerModel, playerModel);
    ::sg_import(src.weaponModel, weaponModel);
    ::sg_import(src.handRBolt, handRBolt);
    ::sg_import(src.handLBolt, handLBolt);
    ::sg_import(src.headBolt, headBolt);
    ::sg_import(src.cervicalBolt, cervicalBolt);
    ::sg_import(src.chestBolt, chestBolt);
    ::sg_import(src.gutBolt, gutBolt);
    ::sg_import(src.torsoBolt, torsoBolt);
    ::sg_import(src.crotchBolt, crotchBolt);
    ::sg_import(src.motionBolt, motionBolt);
    ::sg_import(src.kneeLBolt, kneeLBolt);
    ::sg_import(src.kneeRBolt, kneeRBolt);
    ::sg_import(src.elbowLBolt, elbowLBolt);
    ::sg_import(src.elbowRBolt, elbowRBolt);
    ::sg_import(src.footLBolt, footLBolt);
    ::sg_import(src.footRBolt, footRBolt);
    ::sg_import(src.faceBone, faceBone);
    ::sg_import(src.craniumBone, craniumBone);
    ::sg_import(src.cervicalBone, cervicalBone);
    ::sg_import(src.thoracicBone, thoracicBone);
    ::sg_import(src.upperLumbarBone, upperLumbarBone);
    ::sg_import(src.lowerLumbarBone, lowerLumbarBone);
    ::sg_import(src.hipsBone, hipsBone);
    ::sg_import(src.motionBone, motionBone);
    ::sg_import(src.rootBone, rootBone);
    ::sg_import(src.footLBone, footLBone);
    ::sg_import(src.footRBone, footRBone);
    ::sg_import(src.genericBone1, genericBone1);
    ::sg_import(src.genericBone2, genericBone2);
    ::sg_import(src.genericBone3, genericBone3);
    ::sg_import(src.genericBolt1, genericBolt1);
    ::sg_import(src.genericBolt2, genericBolt2);
    ::sg_import(src.genericBolt3, genericBolt3);
    ::sg_import(src.genericBolt4, genericBolt4);
    ::sg_import(src.genericBolt5, genericBolt5);
    ::sg_import(src.cinematicModel, cinematicModel);
    ::sg_import(src.NPC, NPC);
    ::sg_import(src.ownername, ownername);
    ::sg_import(src.cantHitEnemyCounter, cantHitEnemyCounter);
    ::sg_import(src.NPC_type, NPC_type);
    ::sg_import(src.NPC_targetname, NPC_targetname);
    ::sg_import(src.NPC_target, NPC_target);
    ::sg_import(src.moverState, moverState);
    ::sg_import(src.soundPos1, soundPos1);
    ::sg_import(src.sound1to2, sound1to2);
    ::sg_import(src.sound2to1, sound2to1);
    ::sg_import(src.soundPos2, soundPos2);
    ::sg_import(src.soundLoop, soundLoop);
    ::sg_import(src.nextTrain, nextTrain);
    ::sg_import(src.prevTrain, prevTrain);
    ::sg_import(src.pos1, pos1);
    ::sg_import(src.pos2, pos2);
    ::sg_import(src.pos3, pos3);
    ::sg_import(src.sounds, sounds);
    ::sg_import(src.closetarget, closetarget);
    ::sg_import(src.opentarget, opentarget);
    ::sg_import(src.paintarget, paintarget);
    ::sg_import(src.lockCount, lockCount);
    ::sg_import(src.radius, radius);
    ::sg_import(src.wpIndex, wpIndex);
    ::sg_import(src.noise_index, noise_index);
    ::sg_import(src.startRGBA, startRGBA);
    ::sg_import(src.finalRGBA, finalRGBA);
    ::sg_import(src.item, item);
    ::sg_import(src.message, message);
    ::sg_import(src.lightLevel, lightLevel);
    ::sg_import(src.forcePushTime, forcePushTime);
    ::sg_import(src.forcePuller, forcePuller);
}


void CGhoul2Info_v::sg_export(
    SgType& dst) const
{
    ::sg_export(mItem, dst.mItem);
}

void CGhoul2Info_v::sg_import(
    const SgType& src)
{
    ::sg_export(src.mItem, mItem);
}
