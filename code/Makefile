PLATFORM=$(shell uname|tr A-Z a-z)

CC			= gcc
CXX			= g++
B			= build

ifeq ($(PLATFORM),darwin)
	DLL_EXT		= dylib
	BINARY_FMT	= macho
else
	DLL_EXT		= so
	BINARY_FMT	= elf
endif

GAME_NAME		= $(B)/jagamei386.$(DLL_EXT)
GAME_CFLAGS		= -m32 -I.  -fpermissive -D_M_IX86 -DNDEBUG -DMISSIONPACK -DJK2AWARDS -DFINAL_BUILD -D_NO_PCH
GAME_LDFLAGS		= -m32 -shared -lm
GAME_OBJS		= $(B)/game/AI_Animal.o \
			  $(B)/game/AI_AssassinDroid.o \
			  $(B)/game/AI_Atst.o \
			  $(B)/game/AI_BobaFett.o \
			  $(B)/game/AI_Civilian.o \
			  $(B)/game/AI_Default.o \
			  $(B)/game/AI_Droid.o \
			  $(B)/game/AI_GalakMech.o \
			  $(B)/game/AI_Glider.o \
			  $(B)/game/AI_Grenadier.o \
			  $(B)/game/AI_HazardTrooper.o \
			  $(B)/game/AI_Howler.o \
			  $(B)/game/AI_ImperialProbe.o \
			  $(B)/game/AI_Interrogator.o \
			  $(B)/game/AI_Jedi.o \
			  $(B)/game/AI_Mark1.o \
			  $(B)/game/AI_Mark2.o \
			  $(B)/game/AI_MineMonster.o \
			  $(B)/game/AI_Rancor.o \
			  $(B)/game/AI_Remote.o \
			  $(B)/game/AI_RocketTrooper.o \
			  $(B)/game/AI_SaberDroid.o \
			  $(B)/game/AI_SandCreature.o \
			  $(B)/game/AI_Seeker.o \
			  $(B)/game/AI_Sentry.o \
			  $(B)/game/AI_Sniper.o \
			  $(B)/game/AI_Stormtrooper.o \
			  $(B)/game/AI_Tusken.o \
			  $(B)/game/AI_Utils.o \
			  $(B)/game/AI_Wampa.o \
			  $(B)/game/g_active.o \
			  $(B)/game/g_breakable.o \
			  $(B)/game/g_camera.o \
			  $(B)/game/g_client.o \
			  $(B)/game/g_cmds.o \
			  $(B)/game/g_combat.o \
			  $(B)/game/g_emplaced.o \
			  $(B)/game/g_functions.o \
			  $(B)/game/g_fx.o \
			  $(B)/game/g_headers.o \
			  $(B)/game/g_inventory.o \
			  $(B)/game/g_itemLoad.o \
			  $(B)/game/g_items.o \
			  $(B)/game/g_main.o \
			  $(B)/game/g_mem.o \
			  $(B)/game/g_misc.o \
			  $(B)/game/g_misc_model.o \
			  $(B)/game/g_missile.o \
			  $(B)/game/g_mover.o \
			  $(B)/game/g_nav.o \
			  $(B)/game/g_navigator.o \
			  $(B)/game/g_navnew.o \
			  $(B)/game/g_object.o \
			  $(B)/game/g_objectives.o \
			  $(B)/game/g_rail.o \
			  $(B)/game/g_ref.o \
			  $(B)/game/g_roff.o \
			  $(B)/game/g_savegame.o \
			  $(B)/game/g_session.o \
			  $(B)/game/g_spawn.o \
			  $(B)/game/g_svcmds.o \
			  $(B)/game/g_target.o \
			  $(B)/game/G_Timer.o \
			  $(B)/game/g_trigger.o \
			  $(B)/game/g_turret.o \
			  $(B)/game/g_usable.o \
			  $(B)/game/g_utils.o \
			  $(B)/game/g_weapon.o \
			  $(B)/game/g_weaponLoad.o \
			  $(B)/game/NPC.o \
			  $(B)/game/NPC_behavior.o \
			  $(B)/game/NPC_combat.o \
			  $(B)/game/NPC_goal.o \
			  $(B)/game/NPC_misc.o \
			  $(B)/game/NPC_move.o \
			  $(B)/game/NPC_reactions.o \
			  $(B)/game/NPC_senses.o \
			  $(B)/game/NPC_sounds.o \
			  $(B)/game/NPC_spawn.o \
			  $(B)/game/NPC_stats.o \
			  $(B)/game/NPC_utils.o \
			  $(B)/game/q_math.o \
			  $(B)/game/genericparser2.o \
			  $(B)/game/q_shared.o \
			  $(B)/game/wp_saber.o \
			  $(B)/game/wp_saberLoad.o \
			  $(B)/icarus/BlockStream.o \
			  $(B)/icarus/IcarusImplementation.o \
			  $(B)/game/Q3_Interface.o \
			  $(B)/icarus/Sequence.o \
			  $(B)/icarus/Sequencer.o \
			  $(B)/icarus/TaskManager.o \
			  $(B)/game/bg_misc.o \
			  $(B)/game/bg_pangles.o \
			  $(B)/game/bg_panimate.o \
			  $(B)/game/bg_pmove.o \
			  $(B)/game/bg_slidemove.o \
			  $(B)/cgame/cg_camera.o \
			  $(B)/cgame/cg_consolecmds.o \
			  $(B)/cgame/cg_credits.o \
			  $(B)/cgame/cg_draw.o \
			  $(B)/cgame/cg_drawtools.o \
			  $(B)/cgame/cg_effects.o \
			  $(B)/cgame/cg_ents.o \
			  $(B)/cgame/cg_event.o \
			  $(B)/cgame/cg_headers.o \
			  $(B)/cgame/cg_info.o \
			  $(B)/cgame/cg_lights.o \
			  $(B)/cgame/cg_localents.o \
			  $(B)/cgame/cg_main.o \
			  $(B)/cgame/cg_marks.o \
			  $(B)/cgame/cg_players.o \
			  $(B)/cgame/cg_playerstate.o \
			  $(B)/cgame/cg_predict.o \
			  $(B)/cgame/cg_scoreboard.o \
			  $(B)/cgame/cg_servercmds.o \
			  $(B)/cgame/cg_snapshot.o \
			  $(B)/cgame/cg_text.o \
			  $(B)/cgame/cg_view.o \
			  $(B)/cgame/cg_weapons.o \
			  $(B)/cgame/FX_ATSTMain.o \
			  $(B)/cgame/FX_Blaster.o \
			  $(B)/cgame/FX_Bowcaster.o \
			  $(B)/cgame/FX_BryarPistol.o \
			  $(B)/cgame/FX_Concussion.o \
			  $(B)/cgame/FX_DEMP2.o \
			  $(B)/cgame/FX_Disruptor.o \
			  $(B)/cgame/FX_Emplaced.o \
			  $(B)/cgame/FX_Flechette.o \
			  $(B)/cgame/FX_HeavyRepeater.o \
			  $(B)/cgame/FX_NoghriShot.o \
			  $(B)/cgame/FX_RocketLauncher.o \
			  $(B)/cgame/FX_TuskenShot.o \
			  $(B)/cgame/FxPrimitives.o \
			  $(B)/cgame/FxScheduler.o \
			  $(B)/cgame/FxSystem.o \
			  $(B)/cgame/FxTemplate.o \
			  $(B)/cgame/FxUtil.o \
			  $(B)/Ratl/ratl.o \
			  $(B)/Ravl/CBounds.o \
			  $(B)/Ravl/CVec.o \
			  $(B)/Rufl/hfile.o \
			  $(B)/Rufl/hstring.o \
			  $(B)/game/AnimalNPC.o \
			  $(B)/game/bg_vehicleLoad.o \
			  $(B)/game/FighterNPC.o \
			  $(B)/game/g_vehicles.o \
			  $(B)/game/SpeederNPC.o \
			  $(B)/game/WalkerNPC.o \
			  $(B)/cgame/cg_syscalls.o \
			  $(B)/ui/gameinfo.o \
			  $(B)/qcommon/tri_coll_test.o \
			  $(B)/game/wp_atst.o \
			  $(B)/game/wp_blaster_pistol.o \
			  $(B)/game/wp_blaster_rifle.o \
			  $(B)/game/wp_bot_laser.o \
			  $(B)/game/wp_bowcaster.o \
			  $(B)/game/wp_concussion.o \
			  $(B)/game/wp_demp2.o \
			  $(B)/game/wp_det_pack.o \
			  $(B)/game/wp_disruptor.o \
			  $(B)/game/wp_emplaced_gun.o \
			  $(B)/game/wp_flechette.o \
			  $(B)/game/wp_melee.o \
			  $(B)/game/wp_noghri_stick.o \
			  $(B)/game/wp_repeater.o \
			  $(B)/game/wp_rocket_launcher.o \
			  $(B)/game/wp_stun_baton.o \
			  $(B)/game/wp_thermal.o \
			  $(B)/game/wp_trip_mine.o \
			  $(B)/game/wp_tusken.o

ENGINE_NAME		= $(B)/jasp
ENGINE_CFLAGS		+= -m32 -fpermissive -w -DNDEBUG -DFINAL_BUILD -D_M_IX86 -I. `sdl2-config --cflags` -D_JK2EXE
ENGINE_LDFLAGS		= -m32 `sdl2-config --libs`
ifeq ($(PLATFORM),darwin)
	ENGINE_LDFLAGS		+= -framework OpenGL -framework OpenAL
else
	ENGINE_LDFLAGS		+= -lGL -lopenal
endif
ENGINE_OBJS		= $(B)/engine/client/cl_cgame.o \
			  $(B)/engine/client/cl_cin.o \
			  $(B)/engine/client/cl_console.o \
			  $(B)/engine/client/cl_input.o \
			  $(B)/engine/client/cl_keys.o \
			  $(B)/engine/client/cl_main.o \
			  $(B)/engine/client/cl_parse.o \
			  $(B)/engine/client/cl_scrn.o \
			  $(B)/engine/client/cl_ui.o \
			  $(B)/engine/client/cl_mp3.o \
			  $(B)/engine/qcommon/cm_draw.o \
			  $(B)/engine/qcommon/cm_load.o \
			  $(B)/engine/qcommon/cm_patch.o \
			  $(B)/engine/qcommon/cm_polylib.o \
			  $(B)/engine/qcommon/cm_randomterrain.o \
			  $(B)/engine/qcommon/cm_shader.o \
			  $(B)/engine/qcommon/cm_terrain.o \
			  $(B)/engine/qcommon/cm_terrainmap.o \
			  $(B)/engine/qcommon/cm_test.o \
			  $(B)/engine/qcommon/cm_trace.o \
			  $(B)/engine/qcommon/cmd.o \
			  $(B)/engine/qcommon/common.o \
			  $(B)/engine/qcommon/cvar.o \
			  $(B)/engine/qcommon/files_common.o \
			  $(B)/engine/qcommon/files_pc.o \
			  $(B)/engine/qcommon/hstring.o \
			  $(B)/engine/qcommon/md4.o \
			  $(B)/engine/qcommon/msg.o \
			  $(B)/engine/qcommon/net_chan.o \
			  $(B)/engine/game/q_math.o \
			  $(B)/engine/game/q_shared.o \
			  $(B)/engine/qcommon/stringed_ingame.o \
			  $(B)/engine/qcommon/stringed_interface.o \
			  $(B)/engine/qcommon/strip.o \
			  $(B)/engine/qcommon/unzip.o \
			  $(B)/engine/qcommon/z_memman_pc.o \
			  $(B)/engine/server/sv_ccmds.o \
			  $(B)/engine/server/sv_client.o \
			  $(B)/engine/server/sv_game.o \
			  $(B)/engine/server/sv_init.o \
			  $(B)/engine/server/sv_main.o \
			  $(B)/engine/server/sv_savegame.o \
			  $(B)/engine/server/sv_snapshot.o \
			  $(B)/engine/server/sv_world.o \
			  $(B)/engine/server/exe_headers.o \
			  $(B)/engine/client/snd_ambient.o \
			  $(B)/engine/client/snd_dma.o \
			  $(B)/engine/client/snd_mem.o \
			  $(B)/engine/client/snd_mix.o \
			  $(B)/engine/client/snd_music.o \
			  $(B)/engine/mp3code/cdct.o \
			  $(B)/engine/mp3code/csbt.o \
			  $(B)/engine/mp3code/csbtb.o \
			  $(B)/engine/mp3code/csbtl3.o \
			  $(B)/engine/mp3code/cup.o \
			  $(B)/engine/mp3code/cupini.o \
			  $(B)/engine/mp3code/cupl1.o \
			  $(B)/engine/mp3code/cupl3.o \
			  $(B)/engine/mp3code/cwin.o \
			  $(B)/engine/mp3code/cwinb.o \
			  $(B)/engine/mp3code/cwinm.o \
			  $(B)/engine/mp3code/hwin.o \
			  $(B)/engine/mp3code/l3dq.o \
			  $(B)/engine/mp3code/l3init.o \
			  $(B)/engine/mp3code/mdct.o \
			  $(B)/engine/mp3code/mhead.o \
			  $(B)/engine/mp3code/msis.o \
			  $(B)/engine/mp3code/towave.o \
			  $(B)/engine/mp3code/uph.o \
			  $(B)/engine/mp3code/upsf.o \
			  $(B)/engine/mp3code/wavep.o \
			  $(B)/engine/renderer/matcomp.o \
			  $(B)/engine/renderer/tr_WorldEffects.o \
			  $(B)/engine/renderer/tr_arioche.o \
			  $(B)/engine/renderer/tr_backend.o \
			  $(B)/engine/renderer/tr_bsp.o \
			  $(B)/engine/renderer/tr_cmds.o \
			  $(B)/engine/renderer/tr_draw.o \
			  $(B)/engine/renderer/tr_curve.o \
			  $(B)/engine/renderer/tr_font.o \
			  $(B)/engine/renderer/tr_ghoul2.o \
			  $(B)/engine/renderer/tr_image.o \
			  $(B)/engine/renderer/tr_init.o \
			  $(B)/engine/renderer/tr_light.o \
			  $(B)/engine/renderer/tr_main.o \
			  $(B)/engine/renderer/tr_marks.o \
			  $(B)/engine/renderer/tr_mesh.o \
			  $(B)/engine/renderer/tr_model.o \
			  $(B)/engine/renderer/tr_noise.o \
			  $(B)/engine/renderer/tr_quicksprite.o \
			  $(B)/engine/renderer/tr_scene.o \
			  $(B)/engine/renderer/tr_shade.o \
			  $(B)/engine/renderer/tr_shade_calc.o \
			  $(B)/engine/renderer/tr_shader.o \
			  $(B)/engine/renderer/tr_shadows.o \
			  $(B)/engine/renderer/tr_sky.o \
			  $(B)/engine/renderer/tr_surface.o \
			  $(B)/engine/renderer/tr_surfacesprites.o \
			  $(B)/engine/renderer/tr_terrain.o \
			  $(B)/engine/renderer/tr_world.o \
			  $(B)/engine/jpeg-6/jcapimin.o \
			  $(B)/engine/jpeg-6/jccoefct.o \
			  $(B)/engine/jpeg-6/jccolor.o \
			  $(B)/engine/jpeg-6/jcdctmgr.o \
			  $(B)/engine/jpeg-6/jchuff.o \
			  $(B)/engine/jpeg-6/jcinit.o \
			  $(B)/engine/jpeg-6/jcmainct.o \
			  $(B)/engine/jpeg-6/jcmarker.o \
			  $(B)/engine/jpeg-6/jcmaster.o \
			  $(B)/engine/jpeg-6/jcomapi.o \
			  $(B)/engine/jpeg-6/jcparam.o \
			  $(B)/engine/jpeg-6/jcphuff.o \
			  $(B)/engine/jpeg-6/jcprepct.o \
			  $(B)/engine/jpeg-6/jcsample.o \
			  $(B)/engine/jpeg-6/jctrans.o \
			  $(B)/engine/jpeg-6/jdapimin.o \
			  $(B)/engine/jpeg-6/jdapistd.o \
			  $(B)/engine/jpeg-6/jdatadst.o \
			  $(B)/engine/jpeg-6/jdatasrc.o \
			  $(B)/engine/jpeg-6/jdcoefct.o \
			  $(B)/engine/jpeg-6/jdcolor.o \
			  $(B)/engine/jpeg-6/jddctmgr.o \
			  $(B)/engine/jpeg-6/jdhuff.o \
			  $(B)/engine/jpeg-6/jdinput.o \
			  $(B)/engine/jpeg-6/jdmainct.o \
			  $(B)/engine/jpeg-6/jdmarker.o \
			  $(B)/engine/jpeg-6/jdmaster.o \
			  $(B)/engine/jpeg-6/jdpostct.o \
			  $(B)/engine/jpeg-6/jdsample.o \
			  $(B)/engine/jpeg-6/jdtrans.o \
			  $(B)/engine/jpeg-6/jerror.o \
			  $(B)/engine/jpeg-6/jfdctflt.o \
			  $(B)/engine/jpeg-6/jidctflt.o \
			  $(B)/engine/jpeg-6/jmemmgr.o \
			  $(B)/engine/jpeg-6/jmemnobs.o \
			  $(B)/engine/jpeg-6/jutils.o \
			  $(B)/engine/png/png.o \
			  $(B)/engine/ghoul2/G2_API.o \
			  $(B)/engine/ghoul2/G2_bolts.o \
			  $(B)/engine/ghoul2/G2_bones.o \
			  $(B)/engine/ghoul2/G2_misc.o \
			  $(B)/engine/ghoul2/G2_surfaces.o \
			  $(B)/engine/RMG/RM_Area.o \
			  $(B)/engine/RMG/RM_Instance.o \
			  $(B)/engine/RMG/RM_InstanceFile.o \
			  $(B)/engine/RMG/RM_Instance_BSP.o \
			  $(B)/engine/RMG/RM_Instance_Group.o \
			  $(B)/engine/RMG/RM_Instance_Random.o \
			  $(B)/engine/RMG/RM_Instance_Void.o \
			  $(B)/engine/RMG/RM_Manager.o \
			  $(B)/engine/RMG/RM_Mission.o \
			  $(B)/engine/RMG/RM_Objective.o \
			  $(B)/engine/RMG/RM_Path.o \
			  $(B)/engine/RMG/RM_Terrain.o \
			  $(B)/engine/unix/unix_main.o \
			  $(B)/engine/client/vmachine.o \
			  $(B)/engine/ui/ui_atoms.o \
			  $(B)/engine/ui/ui_connect.o \
			  $(B)/engine/ui/ui_debug.o \
			  $(B)/engine/ui/ui_main.o \
			  $(B)/engine/ui/ui_saber.o \
			  $(B)/engine/ui/ui_shared.o \
			  $(B)/engine/ui/ui_syscalls.o \
              $(B)/engine/sdl/sdl_glimp.o \
              $(B)/engine/sdl/sdl_snd.o \
              $(B)/engine/sdl/sdl_input.o \
              $(B)/engine/sys/sys_main.o \
              $(B)/engine/sys/sys_unix.o \
              $(B)/engine/game/genericparser2.o \
              $(B)/engine/renderer/tr_stl.o \
              $(B)/engine/renderer/tr_jpeg_interface.o \
              $(B)/engine/zlib32/inflate.o \
              $(B)/engine/zlib32/zipcommon.o \
              $(B)/engine/zlib32/deflate.o
              
ifeq ($(PLATFORM),darwin)
	ENGINE_CFLAGS += -DMACOS_X -fasm-blocks
	GAME_CFLAGS += -DMACOS_X
else
	ENGINE_CFLAGS += -masm=intel
	GAME_CFLAGS += -D__linux__
endif

all: makedirs engine game

makedirs:
	@if [ ! -d $(B) ];then mkdir $(B);fi
	@if [ ! -d $(B)/engine ];then mkdir $(B)/engine;fi
	@if [ ! -d $(B)/engine/botlib ];then mkdir $(B)/engine/botlib;fi
	@if [ ! -d $(B)/engine/client ];then mkdir $(B)/engine/client;fi
	@if [ ! -d $(B)/engine/ghoul2 ];then mkdir $(B)/engine/ghoul2;fi
	@if [ ! -d $(B)/engine/icarus ];then mkdir $(B)/engine/icarus;fi
	@if [ ! -d $(B)/engine/jpeg-6 ];then mkdir $(B)/engine/jpeg-6;fi
	@if [ ! -d $(B)/engine/mp3code ];then mkdir $(B)/engine/mp3code;fi
	@if [ ! -d $(B)/engine/png ];then mkdir $(B)/engine/png;fi
	@if [ ! -d $(B)/engine/qcommon ];then mkdir $(B)/engine/qcommon;fi
	@if [ ! -d $(B)/engine/RMG ];then mkdir $(B)/engine/RMG;fi
	@if [ ! -d $(B)/engine/renderer ];then mkdir $(B)/engine/renderer;fi
	@if [ ! -d $(B)/engine/sdl ];then mkdir $(B)/engine/sdl;fi
	@if [ ! -d $(B)/engine/server ];then mkdir $(B)/engine/server;fi
	@if [ ! -d $(B)/engine/game ];then mkdir $(B)/engine/game;fi
	@if [ ! -d $(B)/engine/sys ];then mkdir $(B)/engine/sys;fi
	@if [ ! -d $(B)/engine/ui ];then mkdir $(B)/engine/ui;fi
	@if [ ! -d $(B)/engine/unix ];then mkdir $(B)/engine/unix;fi
	@if [ ! -d $(B)/engine/zlib32 ];then mkdir $(B)/engine/zlib32;fi
	@if [ ! -d $(B)/cgame ];then mkdir $(B)/cgame;fi
	@if [ ! -d $(B)/game ];then mkdir $(B)/game;fi
	@if [ ! -d $(B)/ui ];then mkdir $(B)/ui;fi
	@if [ ! -d $(B)/icarus ];then mkdir $(B)/icarus;fi
	@if [ ! -d $(B)/Ratl ];then mkdir $(B)/Ratl;fi
	@if [ ! -d $(B)/Ravl ];then mkdir $(B)/Ravl;fi
	@if [ ! -d $(B)/Rufl ];then mkdir $(B)/Rufl;fi
	@if [ ! -d $(B)/qcommon ];then mkdir $(B)/qcommon;fi

#
# GAME RULES
#

game: $(GAME_NAME)

$(GAME_NAME): $(GAME_OBJS)
	$(CXX) -o $(GAME_NAME) $(GAME_OBJS) $(GAME_LDFLAGS)

$(B)/game/q_%.o: qcommon/q_%.cpp
	$(CC) $(GAME_CFLAGS) -fPIC -c $< -o $@

$(B)/%.o: %.cpp
	$(CC) $(GAME_CFLAGS) -fPIC -c $< -o $@


#
# ENGINE RULES
#

engine: $(ENGINE_NAME)

$(ENGINE_NAME): $(ENGINE_OBJS)
	$(CXX) -o $(ENGINE_NAME) $(ENGINE_OBJS) $(ENGINE_LDFLAGS)

$(B)/engine/zlib/%.o: zlib/%.c
	$(CC) $(ENGINE_CFLAGS) -c $< -o $@

$(B)/engine/jpeg-6/%.o: jpeg-6/%.c
	$(CC) $(ENGINE_CFLAGS) -c $< -o $@

$(B)/engine/mp3code/%.o: mp3code/%.c
	$(CC) $(ENGINE_CFLAGS) -c $< -o $@

$(B)/engine/botlib/%.o: botlib/%.cpp
	$(CXX) $(ENGINE_CFLAGS) -DBOTLIB -c $< -o $@

$(B)/engine/unix/%.o: unix/%.nasm
	nasm -f $(BINARY_FMT) -o $@  $<

$(B)/engine/%.o: %.c
	$(CXX) $(ENGINE_CFLAGS) -c $< -o $@

$(B)/engine/%.o: %.cpp
	$(CXX) $(ENGINE_CFLAGS) -c $< -o $@


clean:
	$(RM) $(ENGINE_OBJS) $(GAME_OBJS) \
	$(ENGINE_NAME) $(GAME_NAME)
