/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifndef __OBJECTIVES_H__
#define __OBJECTIVES_H__

// mission Objectives

// DO NOT CHANGE MAX_MISSION_OBJ. IT AFFECTS THE SAVEGAME STRUCTURE

typedef enum //# Objective_e
{
  //=================================================
  //
  //=================================================

  LIGHTSIDE_OBJ = 0,
  HOTH2_OBJ1,
  HOTH2_OBJ2,
  HOTH2_OBJ3,
  HOTH3_OBJ1,
  HOTH3_OBJ2,
  HOTH3_OBJ3,
  T2_DPREDICAMENT_OBJ1,
  T2_DPREDICAMENT_OBJ2,
  T2_DPREDICAMENT_OBJ3,
  T2_DPREDICAMENT_OBJ4,
  T2_RANCOR_OBJ1,
  T2_RANCOR_OBJ2,
  T2_RANCOR_OBJ3,
  T2_RANCOR_OBJ4,
  T2_RANCOR_OBJ5,
  T2_RANCOR_OBJ5_2,
  T2_RANCOR_OBJ6,
  T2_WEDGE_OBJ1,
  T2_WEDGE_OBJ2,
  T2_WEDGE_OBJ3,
  T2_WEDGE_OBJ4,
  T2_WEDGE_OBJ5,
  T2_WEDGE_OBJ6,
  T2_WEDGE_OBJ7,
  T2_WEDGE_OBJ8,
  T2_WEDGE_OBJ9,
  T2_WEDGE_OBJ10,
  T2_WEDGE_OBJ11,
  T2_WEDGE_OBJ12,
  T3_RIFT_OBJ1,
  T3_RIFT_OBJ2,
  T3_RIFT_OBJ3,
  T1_DANGER_OBJ1,
  T1_DANGER_OBJ2,
  T1_DANGER_OBJ3,
  T1_DANGER_OBJ4,
  T1_DANGER_OBJ5,
  T3_BOUNTY_OBJ1,
  T3_BOUNTY_OBJ2,
  T3_BOUNTY_OBJ3,
  T3_BOUNTY_OBJ4,
  T3_BOUNTY_OBJ5,
  T3_BOUNTY_OBJ6,
  T3_BOUNTY_OBJ7,
  T3_BOUNTY_OBJ8,
  T3_BOUNTY_OBJ9,
  T2_ROGUE_OBJ1,
  T2_ROGUE_OBJ2,
  T2_TRIP_OBJ1,
  T2_TRIP_OBJ2,
  T3_BYSS_OBJ1,
  T3_BYSS_OBJ2,
  T3_BYSS_OBJ3,
  T3_HEVIL_OBJ1,
  T3_HEVIL_OBJ2,
  T3_HEVIL_OBJ3,
  T3_STAMP_OBJ1,
  T3_STAMP_OBJ2,
  T3_STAMP_OBJ3,
  T3_STAMP_OBJ4,
  TASPIR1_OBJ1,
  TASPIR1_OBJ2,
  TASPIR1_OBJ3,
  TASPIR1_OBJ4,
  TASPIR2_OBJ1,
  TASPIR2_OBJ2,
  VJUN1_OBJ1,
  VJUN1_OBJ2,
  VJUN2_OBJ1,
  VJUN3_OBJ1,
  YAVIN1_OBJ1,
  YAVIN1_OBJ2,
  YAVIN2_OBJ1,
  T1_FATAL_OBJ1,
  T1_FATAL_OBJ2,
  T1_FATAL_OBJ3,
  T1_FATAL_OBJ4,
  T1_FATAL_OBJ5,
  T1_FATAL_OBJ6,
  KOR1_OBJ1,
  KOR1_OBJ2,
  KOR2_OBJ1,
  KOR2_OBJ2,
  KOR2_OBJ3,
  KOR2_OBJ4,
  T1_RAIL_OBJ1,
  T1_RAIL_OBJ2,
  T1_RAIL_OBJ3,
  T1_SOUR_OBJ1,
  T1_SOUR_OBJ2,
  T1_SOUR_OBJ3,
  T1_SOUR_OBJ4,
  T1_SURPRISE_OBJ1,
  T1_SURPRISE_OBJ2,
  T1_SURPRISE_OBJ3,
  T1_SURPRISE_OBJ4,

  //# #eol
  MAX_OBJECTIVES,
} objectiveNumber_t;

typedef enum                       //# MissionFailed_e
{ MISSIONFAILED_JAN = 0,           //#
  MISSIONFAILED_LUKE,              //#
  MISSIONFAILED_LANDO,             //#
  MISSIONFAILED_R5D2,              //#
  MISSIONFAILED_WARDEN,            //#
  MISSIONFAILED_PRISONERS,         //#
  MISSIONFAILED_EMPLACEDGUNS,      //#
  MISSIONFAILED_LADYLUCK,          //#
  MISSIONFAILED_KYLECAPTURE,       //#
  MISSIONFAILED_TOOMANYALLIESDIED, //#
  MISSIONFAILED_CHEWIE,            //#
  MISSIONFAILED_KYLE,              //#
  MISSIONFAILED_ROSH,              //#
  MISSIONFAILED_WEDGE,             //#
  MISSIONFAILED_TURNED,            //# Turned on your friends.

  //# #eol
  MAX_MISSIONFAILED,
} missionFailed_t;

typedef enum //# StatusText_e
{
  //=================================================
  //
  //=================================================
  STAT_INSUBORDINATION = 0, //# Starfleet will not tolerate such insubordination
  STAT_YOUCAUSEDDEATHOFTEAMMATE, //# You caused the death of a teammate.
  STAT_DIDNTPROTECTTECH,  //# You failed to protect Chell, your technician.
  STAT_DIDNTPROTECT7OF9,  //# You failed to protect 7 of 9
  STAT_NOTSTEALTHYENOUGH, //# You weren't quite stealthy enough
  STAT_STEALTHTACTICSNECESSARY, //# Starfleet will not tolerate such
                                //insubordination
  STAT_WATCHYOURSTEP,           //# Watch your step
  STAT_JUDGEMENTMUCHDESIRED,    //# Your judgement leaves much to be desired

  //# #eol
  MAX_STATUSTEXT,
} statusText_t;

extern qboolean missionInfo_Updated;

#define SET_TACTICAL_OFF 0
#define SET_TACTICAL_ON 1

#define SET_OBJ_HIDE 0
#define SET_OBJ_SHOW 1
#define SET_OBJ_PENDING 2
#define SET_OBJ_SUCCEEDED 3
#define SET_OBJ_FAILED 4

#define OBJECTIVE_HIDE 0
#define OBJECTIVE_SHOW 1

#define OBJECTIVE_STAT_PENDING 0
#define OBJECTIVE_STAT_SUCCEEDED 1
#define OBJECTIVE_STAT_FAILED 2

extern int statusTextIndex;

void OBJ_SaveObjectiveData(void);
void OBJ_LoadObjectiveData(void);
extern void OBJ_SetPendingObjectives(gentity_t *ent);

#ifndef G_OBJECTIVES_CPP

extern stringID_table_t objectiveTable[];
extern stringID_table_t statusTextTable[];
extern stringID_table_t missionFailedTable[];

#else

stringID_table_t objectiveTable[] = {
    //=================================================
    //
    //=================================================
    ENUM2STRING(LIGHTSIDE_OBJ),
    ENUM2STRING(HOTH2_OBJ1),
    ENUM2STRING(HOTH2_OBJ2),
    ENUM2STRING(HOTH2_OBJ3),
    ENUM2STRING(HOTH3_OBJ1),
    ENUM2STRING(HOTH3_OBJ2),
    ENUM2STRING(HOTH3_OBJ3),
    ENUM2STRING(T2_DPREDICAMENT_OBJ1),
    ENUM2STRING(T2_DPREDICAMENT_OBJ2),
    ENUM2STRING(T2_DPREDICAMENT_OBJ3),
    ENUM2STRING(T2_DPREDICAMENT_OBJ4),
    ENUM2STRING(T2_RANCOR_OBJ1),
    ENUM2STRING(T2_RANCOR_OBJ2),
    ENUM2STRING(T2_RANCOR_OBJ3),
    ENUM2STRING(T2_RANCOR_OBJ4),
    ENUM2STRING(T2_RANCOR_OBJ5),
    ENUM2STRING(T2_RANCOR_OBJ5_2),
    ENUM2STRING(T2_RANCOR_OBJ6),
    ENUM2STRING(T2_WEDGE_OBJ1),
    ENUM2STRING(T2_WEDGE_OBJ2),
    ENUM2STRING(T2_WEDGE_OBJ3),
    ENUM2STRING(T2_WEDGE_OBJ4),
    ENUM2STRING(T2_WEDGE_OBJ5),
    ENUM2STRING(T2_WEDGE_OBJ6),
    ENUM2STRING(T2_WEDGE_OBJ7),
    ENUM2STRING(T2_WEDGE_OBJ8),
    ENUM2STRING(T2_WEDGE_OBJ9),
    ENUM2STRING(T2_WEDGE_OBJ10),
    ENUM2STRING(T2_WEDGE_OBJ11),
    ENUM2STRING(T2_WEDGE_OBJ12),
    ENUM2STRING(T3_RIFT_OBJ1),
    ENUM2STRING(T3_RIFT_OBJ2),
    ENUM2STRING(T3_RIFT_OBJ3),
    ENUM2STRING(T1_DANGER_OBJ1),
    ENUM2STRING(T1_DANGER_OBJ2),
    ENUM2STRING(T1_DANGER_OBJ3),
    ENUM2STRING(T1_DANGER_OBJ4),
    ENUM2STRING(T1_DANGER_OBJ5),
    ENUM2STRING(T3_BOUNTY_OBJ1),
    ENUM2STRING(T3_BOUNTY_OBJ2),
    ENUM2STRING(T3_BOUNTY_OBJ3),
    ENUM2STRING(T3_BOUNTY_OBJ4),
    ENUM2STRING(T3_BOUNTY_OBJ5),
    ENUM2STRING(T3_BOUNTY_OBJ6),
    ENUM2STRING(T3_BOUNTY_OBJ7),
    ENUM2STRING(T3_BOUNTY_OBJ8),
    ENUM2STRING(T3_BOUNTY_OBJ9),
    ENUM2STRING(T2_ROGUE_OBJ1),
    ENUM2STRING(T2_ROGUE_OBJ2),
    ENUM2STRING(T2_TRIP_OBJ1),
    ENUM2STRING(T2_TRIP_OBJ2),
    ENUM2STRING(T3_BYSS_OBJ1),
    ENUM2STRING(T3_BYSS_OBJ2),
    ENUM2STRING(T3_BYSS_OBJ3),
    ENUM2STRING(T3_HEVIL_OBJ1),
    ENUM2STRING(T3_HEVIL_OBJ2),
    ENUM2STRING(T3_HEVIL_OBJ3),
    ENUM2STRING(T3_STAMP_OBJ1),
    ENUM2STRING(T3_STAMP_OBJ2),
    ENUM2STRING(T3_STAMP_OBJ3),
    ENUM2STRING(T3_STAMP_OBJ4),
    ENUM2STRING(TASPIR1_OBJ1),
    ENUM2STRING(TASPIR1_OBJ2),
    ENUM2STRING(TASPIR1_OBJ3),
    ENUM2STRING(TASPIR1_OBJ4),
    ENUM2STRING(TASPIR2_OBJ1),
    ENUM2STRING(TASPIR2_OBJ2),
    ENUM2STRING(VJUN1_OBJ1),
    ENUM2STRING(VJUN1_OBJ2),
    ENUM2STRING(VJUN2_OBJ1),
    ENUM2STRING(VJUN3_OBJ1),
    ENUM2STRING(YAVIN1_OBJ1),
    ENUM2STRING(YAVIN1_OBJ2),
    ENUM2STRING(YAVIN2_OBJ1),
    ENUM2STRING(T1_FATAL_OBJ1),
    ENUM2STRING(T1_FATAL_OBJ2),
    ENUM2STRING(T1_FATAL_OBJ3),
    ENUM2STRING(T1_FATAL_OBJ4),
    ENUM2STRING(T1_FATAL_OBJ5),
    ENUM2STRING(T1_FATAL_OBJ6),
    ENUM2STRING(KOR1_OBJ1),
    ENUM2STRING(KOR1_OBJ2),
    ENUM2STRING(KOR2_OBJ1),
    ENUM2STRING(KOR2_OBJ2),
    ENUM2STRING(KOR2_OBJ3),
    ENUM2STRING(KOR2_OBJ4),
    ENUM2STRING(T1_RAIL_OBJ1),
    ENUM2STRING(T1_RAIL_OBJ2),
    ENUM2STRING(T1_RAIL_OBJ3),
    ENUM2STRING(T1_SOUR_OBJ1),
    ENUM2STRING(T1_SOUR_OBJ2),
    ENUM2STRING(T1_SOUR_OBJ3),
    ENUM2STRING(T1_SOUR_OBJ4),
    ENUM2STRING(T1_SURPRISE_OBJ1),
    ENUM2STRING(T1_SURPRISE_OBJ2),
    ENUM2STRING(T1_SURPRISE_OBJ3),
    ENUM2STRING(T1_SURPRISE_OBJ4),

    // stringID_table_t Must end with a null entry
    {"", 0}};

stringID_table_t missionFailedTable[] = {
    ENUM2STRING(MISSIONFAILED_JAN),               //# JAN DIED
    ENUM2STRING(MISSIONFAILED_LUKE),              //# LUKE DIED
    ENUM2STRING(MISSIONFAILED_LANDO),             //# LANDO DIED
    ENUM2STRING(MISSIONFAILED_R5D2),              //# R5D2 DIED
    ENUM2STRING(MISSIONFAILED_WARDEN),            //# THE WARDEN DIED
    ENUM2STRING(MISSIONFAILED_PRISONERS),         //#	TOO MANY PRISONERS DIED
    ENUM2STRING(MISSIONFAILED_EMPLACEDGUNS),      //#	ALL EMPLACED GUNS GONE
    ENUM2STRING(MISSIONFAILED_LADYLUCK),          //#	LADY LUCK DISTROYED
    ENUM2STRING(MISSIONFAILED_KYLECAPTURE),       //# KYLE HAS BEEN CAPTURED
    ENUM2STRING(MISSIONFAILED_TOOMANYALLIESDIED), //# TOO MANY ALLIES DIED
    ENUM2STRING(MISSIONFAILED_CHEWIE),
    ENUM2STRING(MISSIONFAILED_KYLE),
    ENUM2STRING(MISSIONFAILED_ROSH),
    ENUM2STRING(MISSIONFAILED_WEDGE),
    ENUM2STRING(MISSIONFAILED_TURNED), //# Turned on your friends.

    // stringID_table_t Must end with a null entry
    {"", 0}};

stringID_table_t statusTextTable[] = {
    //=================================================
    //
    //=================================================
    ENUM2STRING(STAT_INSUBORDINATION), //# Starfleet will not tolerate such
                                       //insubordination
    ENUM2STRING(
        STAT_YOUCAUSEDDEATHOFTEAMMATE),  //# You caused the death of a teammate.
    ENUM2STRING(STAT_DIDNTPROTECTTECH),  //# You failed to protect Chell, your
                                         //technician.
    ENUM2STRING(STAT_DIDNTPROTECT7OF9),  //# You failed to protect 7 of 9
    ENUM2STRING(STAT_NOTSTEALTHYENOUGH), //# You weren't quite stealthy enough
    ENUM2STRING(STAT_STEALTHTACTICSNECESSARY), //# Starfleet will not tolerate
                                               //such insubordination
    ENUM2STRING(STAT_WATCHYOURSTEP),           //# Watch your step
    ENUM2STRING(STAT_JUDGEMENTMUCHDESIRED), //# Your judgement leaves much to be
                                            //desired stringID_table_t Must end
                                            // with a null entry
    {"", 0}};

#endif // #ifndef G_OBJECTIVES_CPP

#endif // #ifndef __OBJECTIVES_H__
