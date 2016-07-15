#include "g_local.h"
#include "anims.h"
#include "qcommon/ojk_i_saved_game.h"


void animFileSet_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int8_t>(filename);
    saved_game->write<>(animations);
    saved_game->write<>(torsoAnimSnds);
    saved_game->write<>(legsAnimSnds);
    saved_game->write<int32_t>(soundsCached);
}

void animFileSet_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int8_t>(filename);
    saved_game->read<>(animations);
    saved_game->read<>(torsoAnimSnds);
    saved_game->read<>(legsAnimSnds);
    saved_game->read<int32_t>(soundsCached);
}
