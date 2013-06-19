PLATFORM=$(shell uname|tr A-Z a-z)

CC				= gcc
CXX				= g++
B				= Build
INST			= Install
BASE    		= Base
JAMPG    		= jampgamei386
CG    			= cgamei386
UI    			= uii386
OPENJK    		= openjk.i386
OPENJKDED    	= openjkded.i386
RDVA    		= rd-vanilla_i386
LIB	    		= lib

ifeq ($(PLATFORM),darwin)
	DLL_EXT		= dylib
else
	DLL_EXT		= so
endif

GAME_NAME		= $(B)/$(INST)/$(BASE)/jampgamei386.$(DLL_EXT)
GAME_CFLAGS		= -m32 -w -O3 -fPIC -I. -DQAGAME -D_JK2 -DNDEBUG -DFINAL_BUILD
GAME_LDFLAGS	= -m32 -shared -lm
GAME_OBJS		= $(B)/$(JAMPG)/game/ai_main.o \
				  $(B)/$(JAMPG)/game/ai_util.o \
				  $(B)/$(JAMPG)/game/ai_wpnav.o \
				  $(B)/$(JAMPG)/game/AnimalNPC.o \
				  $(B)/$(JAMPG)/game/bg_g2_utils.o \
				  $(B)/$(JAMPG)/game/bg_misc.o \
				  $(B)/$(JAMPG)/game/bg_panimate.o \
				  $(B)/$(JAMPG)/game/bg_pmove.o \
				  $(B)/$(JAMPG)/game/bg_saber.o \
				  $(B)/$(JAMPG)/game/bg_saberLoad.o \
				  $(B)/$(JAMPG)/game/bg_saga.o \
				  $(B)/$(JAMPG)/game/bg_slidemove.o \
				  $(B)/$(JAMPG)/game/bg_vehicleLoad.o \
				  $(B)/$(JAMPG)/game/bg_weapons.o \
				  $(B)/$(JAMPG)/game/FighterNPC.o \
				  $(B)/$(JAMPG)/game/g_active.o \
				  $(B)/$(JAMPG)/game/g_bot.o \
				  $(B)/$(JAMPG)/game/g_client.o \
				  $(B)/$(JAMPG)/game/g_cmds.o \
				  $(B)/$(JAMPG)/game/g_combat.o \
				  $(B)/$(JAMPG)/game/g_cvar.o \
				  $(B)/$(JAMPG)/game/g_exphysics.o \
				  $(B)/$(JAMPG)/game/g_ICARUScb.o \
				  $(B)/$(JAMPG)/game/g_items.o \
				  $(B)/$(JAMPG)/game/g_log.o \
				  $(B)/$(JAMPG)/game/g_main.o \
				  $(B)/$(JAMPG)/game/g_mem.o \
				  $(B)/$(JAMPG)/game/g_misc.o \
				  $(B)/$(JAMPG)/game/g_missile.o \
				  $(B)/$(JAMPG)/game/g_mover.o \
				  $(B)/$(JAMPG)/game/g_nav.o \
				  $(B)/$(JAMPG)/game/g_navnew.o \
				  $(B)/$(JAMPG)/game/g_object.o \
				  $(B)/$(JAMPG)/game/g_saga.o \
				  $(B)/$(JAMPG)/game/g_session.o \
				  $(B)/$(JAMPG)/game/g_spawn.o \
				  $(B)/$(JAMPG)/game/g_strap.o \
				  $(B)/$(JAMPG)/game/g_svcmds.o \
				  $(B)/$(JAMPG)/game/g_syscalls.o \
				  $(B)/$(JAMPG)/game/g_target.o \
				  $(B)/$(JAMPG)/game/g_team.o \
				  $(B)/$(JAMPG)/game/g_timer.o \
				  $(B)/$(JAMPG)/game/g_trigger.o \
				  $(B)/$(JAMPG)/game/g_turret_G2.o \
				  $(B)/$(JAMPG)/game/g_turret.o \
				  $(B)/$(JAMPG)/game/g_utils.o \
				  $(B)/$(JAMPG)/game/g_vehicles.o \
				  $(B)/$(JAMPG)/game/g_vehicleTurret.o \
				  $(B)/$(JAMPG)/game/g_weapon.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Atst.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Default.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Droid.o \
				  $(B)/$(JAMPG)/game/NPC_AI_GalakMech.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Grenadier.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Howler.o \
				  $(B)/$(JAMPG)/game/NPC_AI_ImperialProbe.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Interrogator.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Jedi.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Mark1.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Mark2.o \
				  $(B)/$(JAMPG)/game/NPC_AI_MineMonster.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Rancor.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Remote.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Seeker.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Sentry.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Sniper.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Stormtrooper.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Utils.o \
				  $(B)/$(JAMPG)/game/NPC_AI_Wampa.o \
				  $(B)/$(JAMPG)/game/NPC_behavior.o \
				  $(B)/$(JAMPG)/game/NPC_combat.o \
				  $(B)/$(JAMPG)/game/NPC_goal.o \
				  $(B)/$(JAMPG)/game/NPC_misc.o \
				  $(B)/$(JAMPG)/game/NPC_move.o \
				  $(B)/$(JAMPG)/game/NPC_reactions.o \
				  $(B)/$(JAMPG)/game/NPC_senses.o \
				  $(B)/$(JAMPG)/game/NPC_sounds.o \
				  $(B)/$(JAMPG)/game/NPC_spawn.o \
				  $(B)/$(JAMPG)/game/NPC_stats.o \
				  $(B)/$(JAMPG)/game/NPC_utils.o \
				  $(B)/$(JAMPG)/game/NPC.o \
				  $(B)/$(JAMPG)/game/SpeederNPC.o \
				  $(B)/$(JAMPG)/game/tri_coll_test.o \
				  $(B)/$(JAMPG)/game/w_force.o \
				  $(B)/$(JAMPG)/game/w_saber.o \
				  $(B)/$(JAMPG)/game/WalkerNPC.o \
				  $(B)/$(JAMPG)/qcommon/q_math.o \
				  $(B)/$(JAMPG)/qcommon/q_shared.o

CGAME_NAME		= $(B)/$(INST)/$(BASE)/cgamei386.$(DLL_EXT)
CGAME_CFLAGS	= -m32 -w -O3 -fPIC -I. -DCGAME -D_JK2 -DNDEBUG -DFINAL_BUILD
CGAME_LDFLAGS	= -m32 -shared -lm
CGAME_OBJS		= $(B)/$(CG)/cgame/cg_consolecmds.o \
				  $(B)/$(CG)/cgame/cg_draw.o \
				  $(B)/$(CG)/cgame/cg_drawtools.o \
				  $(B)/$(CG)/cgame/cg_effects.o \
				  $(B)/$(CG)/cgame/cg_ents.o \
				  $(B)/$(CG)/cgame/cg_event.o \
				  $(B)/$(CG)/cgame/cg_info.o \
				  $(B)/$(CG)/cgame/cg_light.o \
				  $(B)/$(CG)/cgame/cg_localents.o \
				  $(B)/$(CG)/cgame/cg_main.o \
				  $(B)/$(CG)/cgame/cg_marks.o \
				  $(B)/$(CG)/cgame/cg_newDraw.o \
				  $(B)/$(CG)/cgame/cg_players.o \
				  $(B)/$(CG)/cgame/cg_playerstate.o \
				  $(B)/$(CG)/cgame/cg_predict.o \
				  $(B)/$(CG)/cgame/cg_saga.o \
				  $(B)/$(CG)/cgame/cg_scoreboard.o \
				  $(B)/$(CG)/cgame/cg_servercmds.o \
				  $(B)/$(CG)/cgame/cg_snapshot.o \
				  $(B)/$(CG)/cgame/cg_spawn.o \
				  $(B)/$(CG)/cgame/cg_strap.o \
				  $(B)/$(CG)/cgame/cg_syscalls.o \
				  $(B)/$(CG)/cgame/cg_turret.o \
				  $(B)/$(CG)/cgame/cg_view.o \
				  $(B)/$(CG)/cgame/cg_weaponinit.o \
				  $(B)/$(CG)/cgame/cg_weapons.o \
				  $(B)/$(CG)/cgame/fx_blaster.o \
				  $(B)/$(CG)/cgame/fx_bowcaster.o \
				  $(B)/$(CG)/cgame/fx_bryarpistol.o \
				  $(B)/$(CG)/cgame/fx_demp2.o \
				  $(B)/$(CG)/cgame/fx_disruptor.o \
				  $(B)/$(CG)/cgame/fx_flechette.o \
				  $(B)/$(CG)/cgame/fx_force.o \
				  $(B)/$(CG)/cgame/fx_heavyrepeater.o \
				  $(B)/$(CG)/cgame/fx_rocketlauncher.o \
				  $(B)/$(CG)/game/AnimalNPC.o \
				  $(B)/$(CG)/game/bg_g2_utils.o \
				  $(B)/$(CG)/game/bg_misc.o \
				  $(B)/$(CG)/game/bg_panimate.o \
				  $(B)/$(CG)/game/bg_pmove.o \
				  $(B)/$(CG)/game/bg_saber.o \
				  $(B)/$(CG)/game/bg_saberLoad.o \
				  $(B)/$(CG)/game/bg_saga.o \
				  $(B)/$(CG)/game/bg_slidemove.o \
				  $(B)/$(CG)/game/bg_vehicleLoad.o \
				  $(B)/$(CG)/game/bg_weapons.o \
				  $(B)/$(CG)/game/FighterNPC.o \
				  $(B)/$(CG)/game/SpeederNPC.o \
				  $(B)/$(CG)/game/WalkerNPC.o \
				  $(B)/$(CG)/qcommon/q_math.o \
				  $(B)/$(CG)/qcommon/q_shared.o \
				  $(B)/$(CG)/ui/ui_shared.o

UI_NAME			= $(B)/$(INST)/$(BASE)/uii386.$(DLL_EXT)
UI_CFLAGS		= -m32 -w -O3 -fPIC -I. -DUI_EXPORTS -D_JK2 -DNDEBUG -DFINAL_BUILD
UI_LDFLAGS		= -m32 -shared -lm
UI_OBJS			= $(B)/$(UI)/ui/ui_atoms.o \
				  $(B)/$(UI)/game/bg_misc.o \
				  $(B)/$(UI)/game/bg_saga.o \
				  $(B)/$(UI)/game/bg_vehicleLoad.o \
				  $(B)/$(UI)/game/bg_weapons.o \
				  $(B)/$(UI)/qcommon/q_math.o \
				  $(B)/$(UI)/qcommon/q_shared.o \
				  $(B)/$(UI)/ui/ui_force.o \
				  $(B)/$(UI)/ui/ui_gameinfo.o \
				  $(B)/$(UI)/ui/ui_main.o \
				  $(B)/$(UI)/ui/ui_saber.o \
				  $(B)/$(UI)/ui/ui_shared.o \
				  $(B)/$(UI)/ui/ui_syscalls.o

OPENJK_NAME		= $(B)/$(INST)/openjk.i386
OPENJK_CFLAGS		= -m32 -w -O3 -I. -I../lib -D_JK2 -DNDEBUG -DFINAL_BUILD `sdl2-config --cflags`
OPENJK_LDFLAGS		= -m32 `sdl2-config --libs` -lz
ifeq ($(PLATFORM),darwin)
	OPENJK_LDFLAGS		+= -framework OpenAL
else
	OPENJK_LDFLAGS		+= -lopenal
endif
OPENJK_OBJS		= $(B)/$(OPENJK)/botlib/be_aas_bspq3.o \
				  $(B)/$(OPENJK)/botlib/be_aas_cluster.o \
				  $(B)/$(OPENJK)/botlib/be_aas_debug.o \
				  $(B)/$(OPENJK)/botlib/be_aas_entity.o \
				  $(B)/$(OPENJK)/botlib/be_aas_file.o \
				  $(B)/$(OPENJK)/botlib/be_aas_main.o \
				  $(B)/$(OPENJK)/botlib/be_aas_move.o \
				  $(B)/$(OPENJK)/botlib/be_aas_optimize.o \
				  $(B)/$(OPENJK)/botlib/be_aas_reach.o \
				  $(B)/$(OPENJK)/botlib/be_aas_route.o \
				  $(B)/$(OPENJK)/botlib/be_aas_routealt.o \
				  $(B)/$(OPENJK)/botlib/be_aas_sample.o \
				  $(B)/$(OPENJK)/botlib/be_ai_char.o \
				  $(B)/$(OPENJK)/botlib/be_ai_chat.o \
				  $(B)/$(OPENJK)/botlib/be_ai_gen.o \
				  $(B)/$(OPENJK)/botlib/be_ai_goal.o \
				  $(B)/$(OPENJK)/botlib/be_ai_move.o \
				  $(B)/$(OPENJK)/botlib/be_ai_weap.o \
				  $(B)/$(OPENJK)/botlib/be_ai_weight.o \
				  $(B)/$(OPENJK)/botlib/be_ea.o \
				  $(B)/$(OPENJK)/botlib/be_interface.o \
				  $(B)/$(OPENJK)/botlib/l_crc.o \
				  $(B)/$(OPENJK)/botlib/l_libvar.o \
				  $(B)/$(OPENJK)/botlib/l_log.o \
				  $(B)/$(OPENJK)/botlib/l_memory.o \
				  $(B)/$(OPENJK)/botlib/l_precomp.o \
				  $(B)/$(OPENJK)/botlib/l_script.o \
				  $(B)/$(OPENJK)/botlib/l_struct.o \
				  $(B)/$(OPENJK)/client/cl_avi.o \
				  $(B)/$(OPENJK)/client/cl_cgame.o \
				  $(B)/$(OPENJK)/client/cl_cin.o \
				  $(B)/$(OPENJK)/client/cl_console.o \
				  $(B)/$(OPENJK)/client/cl_input.o \
				  $(B)/$(OPENJK)/client/cl_keys.o \
				  $(B)/$(OPENJK)/client/cl_main.o \
				  $(B)/$(OPENJK)/client/cl_net_chan.o \
				  $(B)/$(OPENJK)/client/cl_parse.o \
				  $(B)/$(OPENJK)/client/cl_scrn.o \
				  $(B)/$(OPENJK)/client/cl_ui.o \
				  $(B)/$(OPENJK)/client/FXExport.o \
				  $(B)/$(OPENJK)/client/FxPrimitives.o \
				  $(B)/$(OPENJK)/client/FxScheduler.o \
				  $(B)/$(OPENJK)/client/FxSystem.o \
				  $(B)/$(OPENJK)/client/FxTemplate.o \
				  $(B)/$(OPENJK)/client/FxUtil.o \
				  $(B)/$(OPENJK)/client/snd_ambient.o \
				  $(B)/$(OPENJK)/client/snd_dma.o \
				  $(B)/$(OPENJK)/client/snd_mem.o \
				  $(B)/$(OPENJK)/client/snd_mix.o \
				  $(B)/$(OPENJK)/client/snd_mp3.o \
				  $(B)/$(OPENJK)/client/snd_music.o \
				  $(B)/$(OPENJK)/icarus/BlockStream.o \
				  $(B)/$(OPENJK)/icarus/GameInterface.o \
				  $(B)/$(OPENJK)/icarus/Instance.o \
				  $(B)/$(OPENJK)/icarus/Interface.o \
				  $(B)/$(OPENJK)/icarus/Memory.o \
				  $(B)/$(OPENJK)/icarus/Q3_Interface.o \
				  $(B)/$(OPENJK)/icarus/Q3_Registers.o \
				  $(B)/$(OPENJK)/icarus/Sequence.o \
				  $(B)/$(OPENJK)/icarus/Sequencer.o \
				  $(B)/$(OPENJK)/icarus/TaskManager.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jaricom.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcapimin.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcapistd.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcarith.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jccoefct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jccolor.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcdctmgr.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jchuff.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcinit.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcmainct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcmarker.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcmaster.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcomapi.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcparam.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcprepct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jcsample.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jctrans.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdapimin.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdapistd.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdarith.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdatadst.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdatasrc.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdcoefct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdcolor.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jddctmgr.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdhuff.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdinput.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdmainct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdmarker.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdmaster.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdmerge.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdpostct.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdsample.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jdtrans.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jerror.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jfdctflt.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jfdctfst.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jfdctint.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jidctflt.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jidctfst.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jidctint.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jmemmgr.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jmemnobs.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jquant1.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jquant2.o \
				  $(B)/$(OPENJK)/$(LIB)/jpeg-8c/jutils.o \
				  $(B)/$(OPENJK)/mp3code/cdct.o \
				  $(B)/$(OPENJK)/mp3code/csbt.o \
				  $(B)/$(OPENJK)/mp3code/csbtb.o \
				  $(B)/$(OPENJK)/mp3code/csbtl3.o \
				  $(B)/$(OPENJK)/mp3code/cup.o \
				  $(B)/$(OPENJK)/mp3code/cupini.o \
				  $(B)/$(OPENJK)/mp3code/cupl1.o \
				  $(B)/$(OPENJK)/mp3code/cupl3.o \
				  $(B)/$(OPENJK)/mp3code/cwin.o \
				  $(B)/$(OPENJK)/mp3code/cwinb.o \
				  $(B)/$(OPENJK)/mp3code/cwinm.o \
				  $(B)/$(OPENJK)/mp3code/hwin.o \
				  $(B)/$(OPENJK)/mp3code/l3dq.o \
				  $(B)/$(OPENJK)/mp3code/l3init.o \
				  $(B)/$(OPENJK)/mp3code/mdct.o \
				  $(B)/$(OPENJK)/mp3code/mhead.o \
				  $(B)/$(OPENJK)/mp3code/msis.o \
				  $(B)/$(OPENJK)/mp3code/towave.o \
				  $(B)/$(OPENJK)/mp3code/uph.o \
				  $(B)/$(OPENJK)/mp3code/upsf.o \
				  $(B)/$(OPENJK)/mp3code/wavep.o \
				  $(B)/$(OPENJK)/qcommon/cm_draw.o \
				  $(B)/$(OPENJK)/qcommon/cm_load.o \
				  $(B)/$(OPENJK)/qcommon/cm_patch.o \
				  $(B)/$(OPENJK)/qcommon/cm_polylib.o \
				  $(B)/$(OPENJK)/qcommon/cm_randomterrain.o \
				  $(B)/$(OPENJK)/qcommon/cm_shader.o \
				  $(B)/$(OPENJK)/qcommon/cm_terrain.o \
				  $(B)/$(OPENJK)/qcommon/cm_terrainmap.o \
				  $(B)/$(OPENJK)/qcommon/cm_test.o \
				  $(B)/$(OPENJK)/qcommon/cm_trace.o \
				  $(B)/$(OPENJK)/qcommon/cmd_common.o \
				  $(B)/$(OPENJK)/qcommon/cmd_pc.o \
				  $(B)/$(OPENJK)/qcommon/CNetProfile.o \
				  $(B)/$(OPENJK)/qcommon/common.o \
				  $(B)/$(OPENJK)/qcommon/cvar.o \
				  $(B)/$(OPENJK)/qcommon/exe_headers.o \
				  $(B)/$(OPENJK)/qcommon/files_common.o \
				  $(B)/$(OPENJK)/qcommon/files_pc.o \
				  $(B)/$(OPENJK)/qcommon/GenericParser2.o \
				  $(B)/$(OPENJK)/qcommon/hstring.o \
				  $(B)/$(OPENJK)/qcommon/huffman.o \
				  $(B)/$(OPENJK)/qcommon/matcomp.o \
				  $(B)/$(OPENJK)/qcommon/md4.o \
				  $(B)/$(OPENJK)/qcommon/msg.o \
				  $(B)/$(OPENJK)/qcommon/net_chan.o \
				  $(B)/$(OPENJK)/qcommon/q_math.o \
				  $(B)/$(OPENJK)/qcommon/q_shared.o \
				  $(B)/$(OPENJK)/qcommon/RoffSystem.o \
				  $(B)/$(OPENJK)/qcommon/stringed_ingame.o \
				  $(B)/$(OPENJK)/qcommon/stringed_interface.o \
				  $(B)/$(OPENJK)/qcommon/unzip.o \
				  $(B)/$(OPENJK)/qcommon/vm_interpreted.o \
				  $(B)/$(OPENJK)/qcommon/vm.o \
				  $(B)/$(OPENJK)/qcommon/z_memman_pc.o \
				  $(B)/$(OPENJK)/RMG/RM_Area.o \
				  $(B)/$(OPENJK)/RMG/RM_Instance_BSP.o \
				  $(B)/$(OPENJK)/RMG/RM_Instance_Group.o \
				  $(B)/$(OPENJK)/RMG/RM_Instance_Random.o \
				  $(B)/$(OPENJK)/RMG/RM_Instance_Void.o \
				  $(B)/$(OPENJK)/RMG/RM_Instance.o \
				  $(B)/$(OPENJK)/RMG/RM_InstanceFile.o \
				  $(B)/$(OPENJK)/RMG/RM_Manager.o \
				  $(B)/$(OPENJK)/RMG/RM_Mission.o \
				  $(B)/$(OPENJK)/RMG/RM_Objective.o \
				  $(B)/$(OPENJK)/RMG/RM_Path.o \
				  $(B)/$(OPENJK)/RMG/RM_Terrain.o \
				  $(B)/$(OPENJK)/sdl/sdl_input.o \
				  $(B)/$(OPENJK)/sdl/sdl_snd.o \
				  $(B)/$(OPENJK)/server/NPCNav/gameCallbacks.o \
				  $(B)/$(OPENJK)/server/NPCNav/navigator.o \
				  $(B)/$(OPENJK)/server/sv_bot.o \
				  $(B)/$(OPENJK)/server/sv_ccmds.o \
				  $(B)/$(OPENJK)/server/sv_client.o \
				  $(B)/$(OPENJK)/server/sv_game.o \
				  $(B)/$(OPENJK)/server/sv_init.o \
				  $(B)/$(OPENJK)/server/sv_main.o \
				  $(B)/$(OPENJK)/server/sv_net_chan.o \
				  $(B)/$(OPENJK)/server/sv_snapshot.o \
				  $(B)/$(OPENJK)/server/sv_world.o \
				  $(B)/$(OPENJK)/sys/snapvector.o \
				  $(B)/$(OPENJK)/sys/sys_main.o \
				  $(B)/$(OPENJK)/sys/sys_net.o \
				  $(B)/$(OPENJK)/sys/sys_unix.o \
				  $(B)/$(OPENJK)/sys/vm_x86.o \
				  $(B)/$(OPENJK)/$(LIB)/zlib/ioapi.o

OPENJKDED_NAME		= $(B)/$(INST)/openjkded.i386
OPENJKDED_CFLAGS	= -m32 -w -O3 -I. -I../lib -D_JK2 -DDEDICATED -DNDEBUG -DFINAL_BUILD
OPENJKDED_LDFLAGS	= -m32 -ldl -lz
OPENJKDED_OBJS		= $(B)/$(OPENJKDED)/botlib/be_aas_bspq3.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_cluster.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_debug.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_entity.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_file.o \
				 	  $(B)/$(OPENJKDED)/botlib/be_aas_main.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_move.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_optimize.o \
				      $(B)/$(OPENJKDED)/botlib/be_aas_reach.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_route.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_routealt.o \
					  $(B)/$(OPENJKDED)/botlib/be_aas_sample.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_char.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_chat.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_gen.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_goal.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_move.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_weap.o \
					  $(B)/$(OPENJKDED)/botlib/be_ai_weight.o \
					  $(B)/$(OPENJKDED)/botlib/be_ea.o \
					  $(B)/$(OPENJKDED)/botlib/be_interface.o \
					  $(B)/$(OPENJKDED)/botlib/l_crc.o \
					  $(B)/$(OPENJKDED)/botlib/l_libvar.o \
					  $(B)/$(OPENJKDED)/botlib/l_log.o \
					  $(B)/$(OPENJKDED)/botlib/l_memory.o \
					  $(B)/$(OPENJKDED)/botlib/l_precomp.o \
					  $(B)/$(OPENJKDED)/botlib/l_script.o \
					  $(B)/$(OPENJKDED)/botlib/l_struct.o \
					  $(B)/$(OPENJKDED)/icarus/BlockStream.o \
					  $(B)/$(OPENJKDED)/icarus/GameInterface.o \
					  $(B)/$(OPENJKDED)/icarus/Instance.o \
					  $(B)/$(OPENJKDED)/icarus/Interface.o \
					  $(B)/$(OPENJKDED)/icarus/Memory.o \
					  $(B)/$(OPENJKDED)/icarus/Q3_Interface.o \
					  $(B)/$(OPENJKDED)/icarus/Q3_Registers.o \
					  $(B)/$(OPENJKDED)/icarus/Sequence.o \
					  $(B)/$(OPENJKDED)/icarus/Sequencer.o \
					  $(B)/$(OPENJKDED)/icarus/TaskManager.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/png.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngerror.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngget.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngmem.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngpread.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngread.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngrio.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngrtran.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngrutil.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngset.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngtrans.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngwio.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngwrite.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngwtran.o \
					  $(B)/$(OPENJKDED)/$(LIB)/libpng/pngwutil.o \
					  $(B)/$(OPENJKDED)/null/null_client.o \
					  $(B)/$(OPENJKDED)/null/null_input.o \
					  $(B)/$(OPENJKDED)/null/null_renderer.o \
					  $(B)/$(OPENJKDED)/null/null_snddma.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_draw.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_load.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_patch.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_polylib.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_randomterrain.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_shader.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_terrain.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_test.o \
					  $(B)/$(OPENJKDED)/qcommon/cm_trace.o \
					  $(B)/$(OPENJKDED)/qcommon/cmd_common.o \
					  $(B)/$(OPENJKDED)/qcommon/cmd_pc.o \
					  $(B)/$(OPENJKDED)/qcommon/common.o \
					  $(B)/$(OPENJKDED)/qcommon/cvar.o \
					  $(B)/$(OPENJKDED)/qcommon/files_common.o \
					  $(B)/$(OPENJKDED)/qcommon/files_pc.o \
					  $(B)/$(OPENJKDED)/qcommon/GenericParser2.o \
					  $(B)/$(OPENJKDED)/qcommon/huffman.o \
					  $(B)/$(OPENJKDED)/qcommon/matcomp.o \
					  $(B)/$(OPENJKDED)/qcommon/md4.o \
					  $(B)/$(OPENJKDED)/qcommon/msg.o \
					  $(B)/$(OPENJKDED)/qcommon/net_chan.o \
					  $(B)/$(OPENJKDED)/qcommon/q_math.o \
					  $(B)/$(OPENJKDED)/qcommon/q_shared.o \
					  $(B)/$(OPENJKDED)/qcommon/RoffSystem.o \
					  $(B)/$(OPENJKDED)/qcommon/stringed_ingame.o \
					  $(B)/$(OPENJKDED)/qcommon/stringed_interface.o \
					  $(B)/$(OPENJKDED)/qcommon/unzip.o \
					  $(B)/$(OPENJKDED)/qcommon/vm_interpreted.o \
					  $(B)/$(OPENJKDED)/qcommon/vm.o \
					  $(B)/$(OPENJKDED)/qcommon/z_memman_pc.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/G2_API.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/G2_bolts.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/G2_bones.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/G2_misc.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/G2_surfaces.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_backend.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_ghoul2.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_image.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_init.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_main.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_mesh.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_model.o \
					  $(B)/$(OPENJKDED)/rd-dedicated/tr_shader.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Area.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Instance_BSP.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Instance_Group.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Instance_Random.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Instance_Void.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Instance.o \
					  $(B)/$(OPENJKDED)/RMG/RM_InstanceFile.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Manager.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Mission.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Objective.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Path.o \
					  $(B)/$(OPENJKDED)/RMG/RM_Terrain.o \
					  $(B)/$(OPENJKDED)/server/NPCNav/gameCallbacks.o \
					  $(B)/$(OPENJKDED)/server/NPCNav/navigator.o \
					  $(B)/$(OPENJKDED)/server/sv_bot.o \
					  $(B)/$(OPENJKDED)/server/sv_ccmds.o \
					  $(B)/$(OPENJKDED)/server/sv_client.o \
					  $(B)/$(OPENJKDED)/server/sv_game.o \
					  $(B)/$(OPENJKDED)/server/sv_init.o \
					  $(B)/$(OPENJKDED)/server/sv_main.o \
					  $(B)/$(OPENJKDED)/server/sv_net_chan.o \
					  $(B)/$(OPENJKDED)/server/sv_snapshot.o \
					  $(B)/$(OPENJKDED)/server/sv_world.o \
					  $(B)/$(OPENJKDED)/sys/snapvector.o \
					  $(B)/$(OPENJKDED)/sys/sys_main.o \
					  $(B)/$(OPENJKDED)/sys/sys_net.o \
					  $(B)/$(OPENJKDED)/sys/sys_unix.o \
					  $(B)/$(OPENJKDED)/sys/vm_x86.o \
					  $(B)/$(OPENJKDED)/$(LIB)/zlib/ioapi.o

RDVANILLA_NAME			= $(B)/$(INST)/rd-vanilla_i386.$(DLL_EXT)
RDVANILLA_CFLAGS		= -m32 -w -O3 -fPIC -I. -I../lib -D_JK2 -DNDEBUG -DFINAL_BUILD `sdl2-config --cflags`
RDVANILLA_LDFLAGS		= -m32 -shared `sdl2-config --libs`
ifeq ($(PLATFORM),darwin)
	RDVANILLA_LDFLAGS		+= -framework OpenGL
else
	RDVANILLA_LDFLAGS		+= -lGL
endif
RDVANILLA_OBJS			= $(B)/$(RDVA)/$(LIB)/jpeg-8c/jaricom.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcapimin.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcapistd.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcarith.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jccoefct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jccolor.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcdctmgr.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jchuff.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcinit.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcmainct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcmarker.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcmaster.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcomapi.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcparam.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcprepct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jcsample.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jctrans.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdapimin.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdapistd.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdarith.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdatadst.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdatasrc.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdcoefct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdcolor.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jddctmgr.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdhuff.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdinput.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdmainct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdmarker.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdmaster.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdmerge.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdpostct.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdsample.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jdtrans.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jerror.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jfdctflt.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jfdctfst.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jfdctint.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jidctflt.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jidctfst.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jidctint.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jmemmgr.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jmemnobs.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jquant1.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jquant2.o \
						  $(B)/$(RDVA)/$(LIB)/jpeg-8c/jutils.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/png.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngerror.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngget.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngmem.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngpread.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngread.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngrio.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngrtran.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngrutil.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngset.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngtrans.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngwio.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngwrite.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngwtran.o \
						  $(B)/$(RDVA)/$(LIB)/libpng/pngwutil.o \
						  $(B)/$(RDVA)/qcommon/GenericParser2.o \
						  $(B)/$(RDVA)/qcommon/matcomp.o \
						  $(B)/$(RDVA)/qcommon/q_math.o \
						  $(B)/$(RDVA)/qcommon/q_shared.o \
						  $(B)/$(RDVA)/rd-vanilla/G2_API.o \
						  $(B)/$(RDVA)/rd-vanilla/G2_bolts.o \
						  $(B)/$(RDVA)/rd-vanilla/G2_bones.o \
						  $(B)/$(RDVA)/rd-vanilla/G2_misc.o \
						  $(B)/$(RDVA)/rd-vanilla/G2_surfaces.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_animation.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_arioche.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_backend.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_bsp.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_cmds.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_curve.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_font.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_ghoul2.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_image.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_init.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_light.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_main.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_marks.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_mesh.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_model.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_noise.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_quicksprite.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_scene.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_shade_calc.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_shade.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_shader.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_shadows.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_sky.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_subs.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_surface.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_surfacesprites.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_terrain.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_world.o \
						  $(B)/$(RDVA)/rd-vanilla/tr_WorldEffects.o \
						  $(B)/$(RDVA)/sdl/sdl_glimp.o

ifeq ($(PLATFORM),darwin)
	OPENJKDED_CFLAGS += -DMACOS_X
	CGAME_CFLAGS += -DMACOS_X
	OPENJK_CFLAGS += -DMACOS_X
	GAME_CFLAGS += -DMACOS_X
	RDVANILLA_CFLAGS += -DMACOS_X
	UI_CFLAGS += -DMACOS_X
else
	GAME_CFLAGS += -D__linux__
	CGAME_CFLAGS += -D__linux__
	UI_CFLAGS += -D__linux__
	OPENJK_CFLAGS += -D__linux__
	OPENJKDED_CFLAGS += -D__linux__
	RDVANILLA_CFLAGS += -D__linux__
endif

all: makedirs game cgame ui openjk openjkded rd-vanilla

makedirs:
	@if [ ! -d $(B) ];then mkdir $(B);fi
	@if [ ! -d $(B)/$(INST) ];then mkdir $(B)/$(INST);fi
	@if [ ! -d $(B)/$(INST)/$(BASE) ];then mkdir $(B)/$(INST)/$(BASE);fi
	@if [ ! -d $(B)/$(JAMPG) ];then mkdir $(B)/$(JAMPG);fi
	@if [ ! -d $(B)/$(JAMPG)/game ];then mkdir $(B)/$(JAMPG)/game;fi
	@if [ ! -d $(B)/$(JAMPG)/qcommon ];then mkdir $(B)/$(JAMPG)/qcommon;fi
	@if [ ! -d $(B)/$(CG) ];then mkdir $(B)/$(CG);fi
	@if [ ! -d $(B)/$(CG)/cgame ];then mkdir $(B)/$(CG)/cgame;fi
	@if [ ! -d $(B)/$(CG)/game ];then mkdir $(B)/$(CG)/game;fi
	@if [ ! -d $(B)/$(CG)/qcommon ];then mkdir $(B)/$(CG)/qcommon;fi
	@if [ ! -d $(B)/$(CG)/ui ];then mkdir $(B)/$(CG)/ui;fi
	@if [ ! -d $(B)/$(UI) ];then mkdir $(B)/$(UI);fi
	@if [ ! -d $(B)/$(UI)/game ];then mkdir $(B)/$(UI)/game;fi
	@if [ ! -d $(B)/$(UI)/qcommon ];then mkdir $(B)/$(UI)/qcommon;fi
	@if [ ! -d $(B)/$(UI)/ui ];then mkdir $(B)/$(UI)/ui;fi
	@if [ ! -d $(B)/$(OPENJK) ];then mkdir $(B)/$(OPENJK);fi
	@if [ ! -d $(B)/$(OPENJK)/botlib ];then mkdir $(B)/$(OPENJK)/botlib;fi
	@if [ ! -d $(B)/$(OPENJK)/client ];then mkdir $(B)/$(OPENJK)/client;fi
	@if [ ! -d $(B)/$(OPENJK)/icarus ];then mkdir $(B)/$(OPENJK)/icarus;fi
	@if [ ! -d $(B)/$(OPENJK)/$(LIB) ];then mkdir $(B)/$(OPENJK)/$(LIB);fi
	@if [ ! -d $(B)/$(OPENJK)/$(LIB)/jpeg-8c ];then mkdir $(B)/$(OPENJK)/$(LIB)/jpeg-8c;fi
	@if [ ! -d $(B)/$(OPENJK)/mp3code ];then mkdir $(B)/$(OPENJK)/mp3code;fi
	@if [ ! -d $(B)/$(OPENJK)/qcommon ];then mkdir $(B)/$(OPENJK)/qcommon;fi
	@if [ ! -d $(B)/$(OPENJK)/RMG ];then mkdir $(B)/$(OPENJK)/RMG;fi
	@if [ ! -d $(B)/$(OPENJK)/sdl ];then mkdir $(B)/$(OPENJK)/sdl;fi
	@if [ ! -d $(B)/$(OPENJK)/server ];then mkdir $(B)/$(OPENJK)/server;fi
	@if [ ! -d $(B)/$(OPENJK)/server/NPCNav ];then mkdir $(B)/$(OPENJK)/server/NPCNav;fi
	@if [ ! -d $(B)/$(OPENJK)/sys ];then mkdir $(B)/$(OPENJK)/sys;fi
	@if [ ! -d $(B)/$(OPENJK)/$(LIB)/zlib ];then mkdir $(B)/$(OPENJK)/$(LIB)/zlib;fi
	@if [ ! -d $(B)/$(OPENJKDED) ];then mkdir $(B)/$(OPENJKDED);fi
	@if [ ! -d $(B)/$(OPENJKDED)/botlib ];then mkdir $(B)/$(OPENJKDED)/botlib;fi
	@if [ ! -d $(B)/$(OPENJKDED)/icarus ];then mkdir $(B)/$(OPENJKDED)/icarus;fi
	@if [ ! -d $(B)/$(OPENJKDED)/$(LIB) ];then mkdir $(B)/$(OPENJKDED)/$(LIB);fi
	@if [ ! -d $(B)/$(OPENJKDED)/$(LIB)/libpng ];then mkdir $(B)/$(OPENJKDED)/$(LIB)/libpng;fi
	@if [ ! -d $(B)/$(OPENJKDED)/null ];then mkdir $(B)/$(OPENJKDED)/null;fi
	@if [ ! -d $(B)/$(OPENJKDED)/qcommon ];then mkdir $(B)/$(OPENJKDED)/qcommon;fi
	@if [ ! -d $(B)/$(OPENJKDED)/rd-dedicated ];then mkdir $(B)/$(OPENJKDED)/rd-dedicated;fi
	@if [ ! -d $(B)/$(OPENJKDED)/RMG ];then mkdir $(B)/$(OPENJKDED)/RMG;fi
	@if [ ! -d $(B)/$(OPENJKDED)/server ];then mkdir $(B)/$(OPENJKDED)/server;fi
	@if [ ! -d $(B)/$(OPENJKDED)/server/NPCNav ];then mkdir $(B)/$(OPENJKDED)/server/NPCNav;fi
	@if [ ! -d $(B)/$(OPENJKDED)/sys ];then mkdir $(B)/$(OPENJKDED)/sys;fi
	@if [ ! -d $(B)/$(OPENJKDED)/$(LIB)/zlib ];then mkdir $(B)/$(OPENJKDED)/$(LIB)/zlib;fi
	@if [ ! -d $(B)/$(RDVA) ];then mkdir $(B)/$(RDVA);fi
	@if [ ! -d $(B)/$(RDVA)/$(LIB) ];then mkdir $(B)/$(RDVA)/$(LIB);fi
	@if [ ! -d $(B)/$(RDVA)/$(LIB)/jpeg-8c ];then mkdir $(B)/$(RDVA)/$(LIB)/jpeg-8c;fi
	@if [ ! -d $(B)/$(RDVA)/$(LIB)/libpng ];then mkdir $(B)/$(RDVA)/$(LIB)/libpng;fi
	@if [ ! -d $(B)/$(RDVA)/qcommon ];then mkdir $(B)/$(RDVA)/qcommon;fi
	@if [ ! -d $(B)/$(RDVA)/rd-vanilla ];then mkdir $(B)/$(RDVA)/rd-vanilla;fi
	@if [ ! -d $(B)/$(RDVA)/sdl ];then mkdir $(B)/$(RDVA)/sdl;fi

#
# GAME RULES
#

game: $(GAME_NAME)

$(GAME_NAME): $(GAME_OBJS)
	$(CC) -o $(GAME_NAME) $(GAME_OBJS) $(GAME_LDFLAGS)

$(B)/$(JAMPG)/game/%.o: game/%.c
	$(CC) $(GAME_CFLAGS) -c $< -o $@

$(B)/$(JAMPG)/qcommon/q_%.o: qcommon/q_%.c
	$(CC) $(GAME_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

#
# CGAME RULES
#

cgame: $(CGAME_NAME)

$(CGAME_NAME): $(CGAME_OBJS)
	$(CC) -o $(CGAME_NAME) $(CGAME_OBJS) $(CGAME_LDFLAGS)

$(B)/$(CG)/cgame/%.o: cgame/%.c
	$(CC) $(CGAME_CFLAGS) -c $< -o $@

$(B)/$(CG)/game/%NPC.o: game/%NPC.c
	$(CC) $(CGAME_CFLAGS) -c $< -o $@

$(B)/$(CG)/game/bg_%.o: game/bg_%.c
	$(CC) $(CGAME_CFLAGS) -c $< -o $@

$(B)/$(CG)/qcommon/q_%.o: qcommon/q_%.c
	$(CC) $(CGAME_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(CG)/ui/ui_%.o: ui/ui_%.c
	$(CC) $(CGAME_CFLAGS) -c $< -o $@

#
# UI RULES
#

ui: $(UI_NAME)

$(UI_NAME): $(UI_OBJS)
	$(CC) -o $(UI_NAME) $(UI_OBJS) $(UI_LDFLAGS)

$(B)/$(UI)/game/bg_%.o: game/bg_%.c
	$(CC) $(UI_CFLAGS) -c $< -o $@	

$(B)/$(UI)/qcommon/q_%.o: qcommon/q_%.c
	$(CC) $(UI_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(UI)/ui/%.o: ui/%.c
	$(CC) $(UI_CFLAGS) -c $< -o $@

#
# OPENJK RULES
#

openjk: $(OPENJK_NAME)

$(OPENJK_NAME): $(OPENJK_OBJS)
	$(CXX) -o $(OPENJK_NAME) $(OPENJK_OBJS) $(OPENJK_LDFLAGS)

$(B)/$(OPENJK)/botlib/%.o: botlib/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -DBOTLIB -c $< -o $@

$(B)/$(OPENJK)/client/%.o: client/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/icarus/%.o: icarus/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/$(LIB)/jpeg-8c/%.o: ../$(LIB)/jpeg-8c/%.c
	$(CC) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/mp3code/%.o: mp3code/%.c
	$(CC) $(OPENJK_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(OPENJK)/qcommon/%.o: qcommon/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(OPENJK)/RMG/%.o: RMG/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/sdl/%.o: sdl/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/server/%.o: server/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/server/NPCNav/%.o: server/NPCNav/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/sys/%.o: sys/%.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@
	
$(B)/$(OPENJK)/sys/snapvector.o: sys/snapvector.cpp
	$(CXX) $(OPENJK_CFLAGS) -c $< -o $@

$(B)/$(OPENJK)/$(LIB)/zlib/ioapi.o: ../$(LIB)/zlib/ioapi.c
	$(CC) $(OPENJK_CFLAGS) -c $< -o $@

#
# OPENJKDED RULES
#

openjkded: $(OPENJKDED_NAME)

$(OPENJKDED_NAME): $(OPENJKDED_OBJS)
	$(CXX) -o $(OPENJKDED_NAME) $(OPENJKDED_OBJS) $(OPENJKDED_LDFLAGS)

$(B)/$(OPENJKDED)/botlib/%.o: botlib/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -DBOTLIB -c $< -o $@

$(B)/$(OPENJKDED)/icarus/%.o: icarus/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/$(LIB)/libpng/%.o: ../$(LIB)/libpng/%.c
	$(CC) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/null/%.o: null/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/qcommon/%.o: qcommon/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(OPENJKDED)/rd-dedicated/%.o: rd-dedicated/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(OPENJKDED)/RMG/%.o: RMG/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/server/%.o: server/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/server/NPCNav/%.o: server/NPCNav/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/sys/%.o: sys/%.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@
	
$(B)/$(OPENJKDED)/sys/snapvector.o: sys/snapvector.cpp
	$(CXX) $(OPENJKDED_CFLAGS) -c $< -o $@

$(B)/$(OPENJKDED)/$(LIB)/zlib/ioapi.o: ../$(LIB)/zlib/ioapi.c
	$(CC) $(OPENJKDED_CFLAGS) -c $< -o $@

#
# RENDERER RULES
#

rd-vanilla: $(RDVANILLA_NAME)

$(RDVANILLA_NAME): $(RDVANILLA_OBJS)
	$(CXX) -o $(RDVANILLA_NAME) $(RDVANILLA_OBJS) $(RDVANILLA_LDFLAGS)

$(B)/$(RDVA)/$(LIB)/jpeg-8c/%.o: ../$(LIB)/jpeg-8c/%.c
	$(CC) $(RDVANILLA_CFLAGS) -c $< -o $@

$(B)/$(RDVA)/$(LIB)/libpng/%.o: ../$(LIB)/libpng/%.c
	$(CC) $(RDVANILLA_CFLAGS) -c $< -o $@
    
$(B)/$(RDVA)/qcommon/%.o: qcommon/%.cpp
	$(CXX) $(RDVANILLA_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(RDVA)/rd-vanilla/%.o: rd-vanilla/%.cpp
	$(CXX) $(RDVANILLA_CFLAGS) -D_M_IX86 -Did386 -c $< -o $@

$(B)/$(RDVA)/sdl/%.o: sdl/%.cpp
	$(CXX) $(RDVANILLA_CFLAGS) -c $< -o $@

clean:
	rm -rf $(B)

clean_game:
	$(RM) $(GAME_OBJS) \
	$(GAME_NAME)

clean_cgame:
	$(RM) $(CGAME_OBJS) \
	$(CGAME_NAME)

clean_ui:
	$(RM) $(UI_OBJS) \
	$(UI_NAME)

clean_openjk:
	$(RM) $(OPENJK_OBJS) \
	$(OPENJK_NAME)

clean_openjkded:
	$(RM) $(OPENJKDED_OBJS) \
	$(OPENJKDED_NAME)

clean_rd:
	$(RM) $(RDVANILLA_OBJS) \
	$(RDVANILLA_NAME)