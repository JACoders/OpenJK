#include "../cgame/cg_local.h"
#include "ai.h"
#include "qcommon/ojk_i_saved_game.h"


void AIGroupMember_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(number);
    saved_game->write<int32_t>(waypoint);
    saved_game->write<int32_t>(pathCostToEnemy);
    saved_game->write<int32_t>(closestBuddy);
}

void AIGroupMember_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(number);
    saved_game->read<int32_t>(waypoint);
    saved_game->read<int32_t>(pathCostToEnemy);
    saved_game->read<int32_t>(closestBuddy);
}

void AIGroupInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(numGroup);
    saved_game->write<int32_t>(processed);
    saved_game->write<int32_t>(team);
    saved_game->write<int32_t>(enemy);
    saved_game->write<int32_t>(enemyWP);
    saved_game->write<int32_t>(speechDebounceTime);
    saved_game->write<int32_t>(lastClearShotTime);
    saved_game->write<int32_t>(lastSeenEnemyTime);
    saved_game->write<int32_t>(morale);
    saved_game->write<int32_t>(moraleAdjust);
    saved_game->write<int32_t>(moraleDebounce);
    saved_game->write<int32_t>(memberValidateTime);
    saved_game->write<int32_t>(activeMemberNum);
    saved_game->write<int32_t>(commander);
    saved_game->write<float>(enemyLastSeenPos);
    saved_game->write<int32_t>(numState);
    saved_game->write<>(member);
}

void AIGroupInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(numGroup);
    saved_game->read<int32_t>(processed);
    saved_game->read<int32_t>(team);
    saved_game->read<int32_t>(enemy);
    saved_game->read<int32_t>(enemyWP);
    saved_game->read<int32_t>(speechDebounceTime);
    saved_game->read<int32_t>(lastClearShotTime);
    saved_game->read<int32_t>(lastSeenEnemyTime);
    saved_game->read<int32_t>(morale);
    saved_game->read<int32_t>(moraleAdjust);
    saved_game->read<int32_t>(moraleDebounce);
    saved_game->read<int32_t>(memberValidateTime);
    saved_game->read<int32_t>(activeMemberNum);
    saved_game->read<int32_t>(commander);
    saved_game->read<float>(enemyLastSeenPos);
    saved_game->read<int32_t>(numState);
    saved_game->read<>(member);
}
