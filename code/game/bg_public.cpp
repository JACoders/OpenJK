#include "bg_public.h"
#include "qcommon/ojk_i_saved_game.h"


void animation_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<uint16_t>(firstFrame);
    saved_game->write<uint16_t>(numFrames);
    saved_game->write<int16_t>(frameLerp);
    saved_game->write<int8_t>(loopFrames);
    saved_game->write<uint8_t>(glaIndex);
}

void animation_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<uint16_t>(firstFrame);
    saved_game->read<uint16_t>(numFrames);
    saved_game->read<int16_t>(frameLerp);
    saved_game->read<int8_t>(loopFrames);
    saved_game->read<uint8_t>(glaIndex);
}

void animevent_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(eventType);
    saved_game->write<int16_t>(modelOnly);
    saved_game->write<uint16_t>(glaIndex);
    saved_game->write<uint16_t>(keyFrame);
    saved_game->write<int16_t>(eventData);
    saved_game->write<int32_t>(stringData);
}

void animevent_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(eventType);
    saved_game->read<int16_t>(modelOnly);
    saved_game->read<uint16_t>(glaIndex);
    saved_game->read<uint16_t>(keyFrame);
    saved_game->read<int16_t>(eventData);
    saved_game->read<int32_t>(stringData);
}
