#include "qcommon/q_shared.h"
#include "mdx_format.h"
#include "qcommon/ojk_sg_wrappers.h"


void mdxaBone_t::sg_export(
    SgType& dst) const
{
    ::sg_export(matrix, dst.matrix);
}

void mdxaBone_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.matrix, matrix);
}
