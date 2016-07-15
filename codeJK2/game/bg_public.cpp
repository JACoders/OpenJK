#include "bg_public.h"
#include "qcommon/ojk_i_saved_game.h"


void animation_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(firstFrame);
    saved_game->write<int32_t>(numFrames);
    saved_game->write<int32_t>(loopFrames);
    saved_game->write<int32_t>(frameLerp);
    saved_game->write<int32_t>(initialLerp);
}

void animation_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(firstFrame);
    saved_game->read<int32_t>(numFrames);
    saved_game->read<int32_t>(loopFrames);
    saved_game->read<int32_t>(frameLerp);
    saved_game->read<int32_t>(initialLerp);
}


void animsounds_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(keyFrame);
    saved_game->write<int32_t>(soundIndex);
    saved_game->write<int32_t>(numRandomAnimSounds);
    saved_game->write<int32_t>(probability);
}

void animsounds_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(keyFrame);
    saved_game->read<int32_t>(soundIndex);
    saved_game->read<int32_t>(numRandomAnimSounds);
    saved_game->read<int32_t>(probability);
}
