#include "../cgame/cg_local.h"
#include "ai.h"
#include "qcommon/ojk_sg_wrappers.h"


void AIGroupMember_t::sg_export(
    SgType& dst) const
{
    ::sg_export(number, dst.number);
    ::sg_export(waypoint, dst.waypoint);
    ::sg_export(pathCostToEnemy, dst.pathCostToEnemy);
    ::sg_export(closestBuddy, dst.closestBuddy);
}

void AIGroupMember_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.number, number);
    ::sg_import(src.waypoint, waypoint);
    ::sg_import(src.pathCostToEnemy, pathCostToEnemy);
    ::sg_import(src.closestBuddy, closestBuddy);
}

void AIGroupInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(numGroup, dst.numGroup);
    ::sg_export(processed, dst.processed);
    ::sg_export(team, dst.team);
    ::sg_export(enemy, dst.enemy);
    ::sg_export(enemyWP, dst.enemyWP);
    ::sg_export(speechDebounceTime, dst.speechDebounceTime);
    ::sg_export(lastClearShotTime, dst.lastClearShotTime);
    ::sg_export(lastSeenEnemyTime, dst.lastSeenEnemyTime);
    ::sg_export(morale, dst.morale);
    ::sg_export(moraleAdjust, dst.moraleAdjust);
    ::sg_export(moraleDebounce, dst.moraleDebounce);
    ::sg_export(memberValidateTime, dst.memberValidateTime);
    ::sg_export(activeMemberNum, dst.activeMemberNum);
    ::sg_export(commander, dst.commander);
    ::sg_export(enemyLastSeenPos, dst.enemyLastSeenPos);
    ::sg_export(numState, dst.numState);
    ::sg_export(member, dst.member);
}

void AIGroupInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.numGroup, numGroup);
    ::sg_import(src.processed, processed);
    ::sg_import(src.team, team);
    ::sg_import(src.enemy, enemy);
    ::sg_import(src.enemyWP, enemyWP);
    ::sg_import(src.speechDebounceTime, speechDebounceTime);
    ::sg_import(src.lastClearShotTime, lastClearShotTime);
    ::sg_import(src.lastSeenEnemyTime, lastSeenEnemyTime);
    ::sg_import(src.morale, morale);
    ::sg_import(src.moraleAdjust, moraleAdjust);
    ::sg_import(src.moraleDebounce, moraleDebounce);
    ::sg_import(src.memberValidateTime, memberValidateTime);
    ::sg_import(src.activeMemberNum, activeMemberNum);
    ::sg_import(src.commander, commander);
    ::sg_import(src.enemyLastSeenPos, enemyLastSeenPos);
    ::sg_import(src.numState, numState);
    ::sg_import(src.member, member);
}
