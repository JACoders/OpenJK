#include "g_local.h"
#include "qcommon/ojk_i_saved_game.h"


void alertEvent_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<float>(position);
    saved_game->write<float>(radius);
    saved_game->write<int32_t>(level);
    saved_game->write<int32_t>(type);
    saved_game->write<int32_t>(owner);
    saved_game->write<float>(light);
    saved_game->write<float>(addLight);
    saved_game->write<int32_t>(ID);
    saved_game->write<int32_t>(timestamp);
}

void alertEvent_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<float>(position);
    saved_game->read<float>(radius);
    saved_game->read<int32_t>(level);
    saved_game->read<int32_t>(type);
    saved_game->read<int32_t>(owner);
    saved_game->read<float>(light);
    saved_game->read<float>(addLight);
    saved_game->read<int32_t>(ID);
    saved_game->read<int32_t>(timestamp);
}


void level_locals_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(clients);
    saved_game->write<int32_t>(maxclients);
    saved_game->write<int32_t>(framenum);
    saved_game->write<int32_t>(time);
    saved_game->write<int32_t>(previousTime);
    saved_game->write<int32_t>(globalTime);
    saved_game->write<int8_t>(mapname);
    saved_game->write<int32_t>(locationLinked);
    saved_game->write<int32_t>(locationHead);
    saved_game->write<>(alertEvents);
    saved_game->write<int32_t>(numAlertEvents);
    saved_game->write<int32_t>(curAlertID);
    saved_game->write<>(groups);
    saved_game->write<>(knownAnimFileSets);
    saved_game->write<int32_t>(numKnownAnimFileSets);
    saved_game->write<int32_t>(worldFlags);
    saved_game->write<int32_t>(dmState);
}

void level_locals_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(clients);
    saved_game->read<int32_t>(maxclients);
    saved_game->read<int32_t>(framenum);
    saved_game->read<int32_t>(time);
    saved_game->read<int32_t>(previousTime);
    saved_game->read<int32_t>(globalTime);
    saved_game->read<int8_t>(mapname);
    saved_game->read<int32_t>(locationLinked);
    saved_game->read<int32_t>(locationHead);
    saved_game->read<>(alertEvents);
    saved_game->read<int32_t>(numAlertEvents);
    saved_game->read<int32_t>(curAlertID);
    saved_game->read<>(groups);
    saved_game->read<>(knownAnimFileSets);
    saved_game->read<int32_t>(numKnownAnimFileSets);
    saved_game->read<int32_t>(worldFlags);
    saved_game->read<int32_t>(dmState);
}
