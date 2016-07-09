#include "bg_public.h"
#include "qcommon/ojk_sg_wrappers.h"


void animation_t::sg_export(
    SgType& dst) const
{
    ::sg_export(firstFrame, dst.firstFrame);
    ::sg_export(numFrames, dst.numFrames);
    ::sg_export(loopFrames, dst.loopFrames);
    ::sg_export(frameLerp, dst.frameLerp);
    ::sg_export(initialLerp, dst.initialLerp);
}

void animation_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.firstFrame, firstFrame);
    ::sg_import(src.numFrames, numFrames);
    ::sg_import(src.loopFrames, loopFrames);
    ::sg_import(src.frameLerp, frameLerp);
    ::sg_import(src.initialLerp, initialLerp);
}


void animsounds_t::sg_export(
    SgType& dst) const
{
    ::sg_export(keyFrame, dst.keyFrame);
    ::sg_export(soundIndex, dst.soundIndex);
    ::sg_export(numRandomAnimSounds, dst.numRandomAnimSounds);
    ::sg_export(probability, dst.probability);
}

void animsounds_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.keyFrame, keyFrame);
    ::sg_import(src.soundIndex, soundIndex);
    ::sg_import(src.numRandomAnimSounds, numRandomAnimSounds);
    ::sg_import(src.probability, probability);
}
