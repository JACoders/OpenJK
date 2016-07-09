#include "bg_public.h"
#include "qcommon/ojk_sg_wrappers.h"


void animation_t::sg_export(
    SgType& dst) const
{
    ::sg_export(firstFrame, dst.firstFrame);
    ::sg_export(numFrames, dst.numFrames);
    ::sg_export(frameLerp, dst.frameLerp);
    ::sg_export(loopFrames, dst.loopFrames);
    ::sg_export(glaIndex, dst.glaIndex);
}

void animation_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.firstFrame, firstFrame);
    ::sg_import(src.numFrames, numFrames);
    ::sg_import(src.frameLerp, frameLerp);
    ::sg_import(src.loopFrames, loopFrames);
    ::sg_import(src.glaIndex, glaIndex);
}

void animevent_t::sg_export(
    SgType& dst) const
{
    ::sg_export(eventType, dst.eventType);
    ::sg_export(modelOnly, dst.modelOnly);
    ::sg_export(glaIndex, dst.glaIndex);
    ::sg_export(keyFrame, dst.keyFrame);
    ::sg_export(eventData, dst.eventData);
    ::sg_export(stringData, dst.stringData);
}

void animevent_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.eventType, eventType);
    ::sg_import(src.modelOnly, modelOnly);
    ::sg_import(src.glaIndex, glaIndex);
    ::sg_import(src.keyFrame, keyFrame);
    ::sg_import(src.eventData, eventData);
    ::sg_import(src.stringData, stringData);
}
