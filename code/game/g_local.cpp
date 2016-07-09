#include "g_local.h"
#include "qcommon/ojk_sg_wrappers.h"


void animFileSet_t::sg_export(
    SgType& dst) const
{
    ::sg_export(filename, dst.filename);
    ::sg_export(animations, dst.animations);
    ::sg_export(torsoAnimEvents, dst.torsoAnimEvents);
    ::sg_export(legsAnimEvents, dst.legsAnimEvents);
    ::sg_export(torsoAnimEventCount, dst.torsoAnimEventCount);
    ::sg_export(legsAnimEventCount, dst.legsAnimEventCount);
}

void animFileSet_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.filename, filename);
    ::sg_import(src.animations, animations);
    ::sg_import(src.torsoAnimEvents, torsoAnimEvents);
    ::sg_import(src.legsAnimEvents, legsAnimEvents);
    ::sg_import(src.torsoAnimEventCount, torsoAnimEventCount);
    ::sg_import(src.legsAnimEventCount, legsAnimEventCount);
}


void alertEvent_t::sg_export(
    SgType& dst) const
{
    ::sg_export(position, dst.position);
    ::sg_export(radius, dst.radius);
    ::sg_export(level, dst.level);
    ::sg_export(type, dst.type);
    ::sg_export(owner, dst.owner);
    ::sg_export(light, dst.light);
    ::sg_export(addLight, dst.addLight);
    ::sg_export(ID, dst.ID);
    ::sg_export(timestamp, dst.timestamp);
    ::sg_export(onGround, dst.onGround);
}

void alertEvent_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.position, position);
    ::sg_import(src.radius, radius);
    ::sg_import(src.level, level);
    ::sg_import(src.type, type);
    ::sg_import(src.owner, owner);
    ::sg_import(src.light, light);
    ::sg_import(src.addLight, addLight);
    ::sg_import(src.ID, ID);
    ::sg_import(src.timestamp, timestamp);
    ::sg_import(src.onGround, onGround);
}


void level_locals_t::sg_export(
    SgType& dst) const
{
    ::sg_export(clients, dst.clients);
    ::sg_export(maxclients, dst.maxclients);
    ::sg_export(framenum, dst.framenum);
    ::sg_export(time, dst.time);
    ::sg_export(previousTime, dst.previousTime);
    ::sg_export(globalTime, dst.globalTime);
    ::sg_export(mapname, dst.mapname);
    ::sg_export(locationLinked, dst.locationLinked);
    ::sg_export(locationHead, dst.locationHead);
    ::sg_export(alertEvents, dst.alertEvents);
    ::sg_export(numAlertEvents, dst.numAlertEvents);
    ::sg_export(curAlertID, dst.curAlertID);
    ::sg_export(groups, dst.groups);
    ::sg_export(knownAnimFileSets, dst.knownAnimFileSets);
    ::sg_export(numKnownAnimFileSets, dst.numKnownAnimFileSets);
    ::sg_export(worldFlags, dst.worldFlags);
    ::sg_export(dmState, dst.dmState);
}

void level_locals_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.clients, clients);
    ::sg_import(src.maxclients, maxclients);
    ::sg_import(src.framenum, framenum);
    ::sg_import(src.time, time);
    ::sg_import(src.previousTime, previousTime);
    ::sg_import(src.globalTime, globalTime);
    ::sg_import(src.mapname, mapname);
    ::sg_import(src.locationLinked, locationLinked);
    ::sg_import(src.locationHead, locationHead);
    ::sg_import(src.alertEvents, alertEvents);
    ::sg_import(src.numAlertEvents, numAlertEvents);
    ::sg_import(src.curAlertID, curAlertID);
    ::sg_import(src.groups, groups);
    ::sg_import(src.knownAnimFileSets, knownAnimFileSets);
    ::sg_import(src.numKnownAnimFileSets, numKnownAnimFileSets);
    ::sg_import(src.worldFlags, worldFlags);
    ::sg_import(src.dmState, dmState);
}
