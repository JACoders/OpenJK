#include "qcommon/q_shared.h"
#include "mdx_format.h"
#include "qcommon/ojk_i_saved_game.h"


void mdxaBone_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<float>(matrix);
}

void mdxaBone_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<float>(matrix);
}
