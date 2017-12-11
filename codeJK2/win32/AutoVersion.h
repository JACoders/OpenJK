/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#ifndef __AUTO_VERSION_HEADER
#define __AUTO_VERSION_HEADER

#define VERSION_MAJOR_RELEASE		1
#define VERSION_MINOR_RELEASE		0
#define VERSION_EXTERNAL_BUILD		1
#define VERSION_INTERNAL_BUILD		1

#define VERSION_STRING				"1, 0, 1, 1"
#define VERSION_STRING_DOTTED		"1.0.1.1"

#define VERSION_BUILD_NUMBER		80

// BEGIN COMMENTS
// 1.0.1.0		10/24/2003 16:29:53		jmonroe		patch rc1
// 1.0.0.4		10/17/2003 17:45:31		jmonroe		fix ati texture rect hack
// 1.0.0.3		08/29/2003 18:17:37		jmonroe		hide saberrealisticcombat and g_dismemberment debug value
// 1.0.0.2		08/12/2003 17:48:44		jmonroe		demo, take 3
// 1.0.0.1		08/07/2003 19:53:23		jmonroe		demo round 2
// 1.0.0.0		08/01/2003 16:59:44		jmonroe		push demo code changes
// 1.0.0.0		07/21/2003 16:09:23		jmonroe		going gold
// 0.0.12.0		07/20/2003 19:04:44		jmonroe		build 0.12 for qa
// 0.0.11.0		07/19/2003 15:30:08		jmonroe		stuff
// 0.0.11.0		07/17/2003 17:08:09		jmonroe		build 0.11 for qa
// 0.0.10.1		07/16/2003 17:33:28		mgummelt		General Update With Latest Code
// 0.0.10.0		07/16/2003 10:28:51		jmonroe		new product id, remove outcast.roq at end, menu ignoreescape, eax looping sounds support chan_less_atten
// 0.0.10.0		07/14/2003 19:04:42		jmonroe		build 0.10 for qa
// 0.0.9.0		07/12/2003 13:05:57		jmonroe		increase numsnapshot ents
// 0.0.9.0		07/11/2003 17:48:40		jmonroe		buil 0.09 for qa
// 0.0.8.4		07/11/2003 15:47:55		jmonroe		weatherzone caching
// 0.0.8.3		07/11/2003 12:05:44		mgummelt		Lava & Acid fixes
// 0.0.8.2		07/10/2003 17:26:42		mgummelt		General Update with today's bug fixes
// 0.0.8.1		07/09/2003 19:24:19		jmonroe		eax voice stomp fix, increased weather zones, ...
// 0.0.8.0		07/08/2003 17:28:27		jmonroe		build 0.08 for qa
// 0.0.7.2		07/07/2003 16:28:40		mgummelt		Blaster Pistol alt-fire returns, Rancor spawnflags corrected
// 0.0.7.1		07/07/2003 10:48:29		jmonroe		load menu fixes, darkside autoload fix,...
// 0.0.7.0		07/02/2003 19:31:37		jmonroe		buil 7 for qa
// 0.0.6.5		07/02/2003 18:17:17		mgummelt		Boss balancing
// 0.0.6.3		07/02/2003 01:34:48		jmonroe		saber in moves menu
// 0.0.6.2		06/30/2003 15:33:37		mgummelt		Force Sight change (designers readme)
// 0.0.6.1		06/29/2003 19:00:17		jmonroe		vv merged, aev_soundchan
// 0.0.6.0		06/28/2003 16:38:27		jmonroe		eax update, force sight on key dudes
// 0.0.6.0		06/26/2003 16:45:26		jmonroe		qa build
// 0.0.5.6		06/25/2003 21:22:38		mgummelt		"redcrosshair" field on misc_model_breakables and func_breakables
// 0.0.5.5		06/25/2003 18:31:52		mgummelt		Force Visible on all applicable ents
// 0.0.5.4		06/25/2003 10:45:12		jmonroe		EAX 4.0
// 0.0.5.3		06/24/2003 22:59:31		mgummelt		Reborn tweaks
// 0.0.5.2		06/24/2003 18:53:55		mgummelt		New Reborn Master
// 0.0.5.1		06/23/2003 20:47:44		jmonroe		fix NULL NPC usage
// 0.0.5.0		06/21/2003 13:04:27		jmonroe		fix eweb crash
// 0.0.4.2		06/18/2003 14:24:35		jmonroe		script cmd SET_WEAPON  now precaches weapon, gil's optimized dlights
// 0.0.4.1		06/17/2003 19:04:28		mgummelt		Various Force Power, saber move & Enemy Jedi tweaks
// 0.0.4.0		06/16/2003 19:41:54		jmonroe		bug stuff
// 0.0.4.0		06/13/2003 23:52:36		jmonroe		inc version to match qa beta
// 0.0.2.6		06/12/2003 13:11:21		creed		new z-far cull
// 0.0.2.5		06/10/2003 17:02:17		creed		targetJump & other navigation
// 0.0.2.4		06/10/2003 11:18:58		jmonroe		footstep sounds in per material
// 0.0.2.3		06/10/2003 01:37:10		mgummelt		helping with entity limit at spawntime
// 0.0.2.2		06/09/2003 14:10:32		scork		fix for asian languages being bust by bad font
// 0.0.2.1		06/08/2003 17:57:11		mgummelt		Area portal fix
// 0.0.2.0		06/08/2003 14:02:03		mgummelt		Force sight change
// 0.0.2.0		06/08/2003 00:03:58		jmonroe		CGEN_LIGHTING_DIFFUSE_ENTITY merges lightingdiffuse and entity color
// 0.0.1.25		06/05/2003 20:51:30		mgummelt		Saber pull-attacks done
// 0.0.1.24		06/05/2003 18:11:06		mgummelt		Both saber control schemes implemented
// 0.0.1.23		05/30/2003 17:12:34		mgummelt		Fixed Noghri and Sand Creature
// 0.0.1.22		05/30/2003 12:15:37		mgummelt		Saboteur change, new cultist
// 0.0.1.21		05/29/2003 16:02:48		creed		Fixes For Assassin, ST AI changes
// 0.0.1.20		05/29/2003 12:00:18		mgummelt		Navigation changes
// 0.0.1.19		05/29/2003 11:08:06		mgummelt		various tweaks
// 0.0.1.18		05/28/2003 22:08:16		creed		Boba Fett ++
// 0.0.1.17		05/28/2003 20:38:33		mgummelt		force grip and force sense changes
// 0.0.1.15		05/28/2003 17:07:26		mgummelt		tweaks of force drain, protect and absorb
// 0.0.1.14		05/28/2003 14:48:57		mgummelt		various saber & AI fixes
// 0.0.1.13		05/27/2003 17:26:30		areis		Ported Glow stuff from MP (with support for nVidia and ATI cards). Added solid flag for roffs (in behaved) and made tie-bombers explode with effect.
// 0.0.1.12		05/27/2003 13:06:03		mgummelt		Noghri stick weapon shoots a projectile, E-Web uses proper sounds
// 0.0.1.7		05/19/2003 15:00:10		mgummelt		adding random jedi, elder prisoners, jedi master
// 0.0.1.6		05/19/2003 09:22:18		creed		Weather Effects & Haz Trooper
// 0.0.1.5		05/16/2003 18:55:10		creed		New Wind Spawn Flags For Dusty Fog & 1/2 way through Haz Trooper Fixins
// 0.0.1.4		05/15/2003 14:29:35		jmonroe		misc_model_static stay around after a vid_start
// 0.0.1.3		05/14/2003 17:53:44		mgummelt		testing misc_model_breakable scaling
// 0.0.1.2		05/13/2003 20:48:35		jmonroe		vv post merge
// 0.0.1.1		05/13/2003 14:17:20		scork		ste test comment
// END COMMENTS

#endif // __AUTO_VERSION_HEADER
