CC			= 	gcc
CXX			= 	g++

ENGINE_CFLAGS		+= 	-fpermissive -m32 -w -D_JK2 -D__linux__ -DNDEBUG -D_JK2 -D_JK2MP -DFINAL_BUILD -D_M_IX86 -I. -ggdb3

G_CFLAGS		=	-m32 -I. -D_M_IX86 -DNDEBUG -D__linux__ -DMISSIONPACK -DQAGAME -DJK2AWARDS -D_JK2 -DFINAL_BUILD -ggdb3

G_OBJS			=	game/ai_main.o \
				game/ai_util.o \
				game/ai_wpnav.o \
				game/AnimalNPC.o \
				game/bg_g2_utils.o \
				game/bg_misc.o \
				game/bg_panimate.o \
				game/bg_pmove.o \
				game/bg_saber.o \
				game/bg_saberLoad.o \
				game/bg_saga.o \
				game/bg_slidemove.o \
				game/bg_vehicleLoad.o \
				game/bg_weapons.o \
				game/FighterNPC.o \
				game/g_active.o \
				game/g_arenas.o \
				game/g_bot.o \
				game/g_client.o \
				game/g_cmds.o \
				game/g_combat.o \
				game/g_exphysics.o \
				game/g_ICARUScb.o \
				game/g_items.o \
				game/g_log.o \
				game/g_main.o \
				game/g_mem.o \
				game/g_misc.o \
				game/g_missile.o \
				game/g_mover.o \
				game/g_nav.o \
				game/g_navnew.o \
				game/g_object.o \
				game/g_saga.o \
				game/g_session.o \
				game/g_spawn.o \
				game/g_strap.o \
				game/g_svcmds.o \
				game/g_syscalls.o \
				game/g_target.o \
				game/g_team.o \
				game/g_timer.o \
				game/g_trigger.o \
				game/g_turret.o \
				game/g_turret_G2.o \
				game/g_utils.o \
				game/g_vehicles.o \
				game/g_vehicleTurret.o \
				game/g_weapon.o \
				game/NPC.o \
				game/NPC_AI_Atst.o \
				game/NPC_AI_Default.o \
				game/NPC_AI_Droid.o \
				game/NPC_AI_GalakMech.o \
				game/NPC_AI_Grenadier.o \
				game/NPC_AI_Howler.o \
				game/NPC_AI_ImperialProbe.o \
				game/NPC_AI_Interrogator.o \
				game/NPC_AI_Jedi.o \
				game/NPC_AI_Mark1.o \
				game/NPC_AI_Mark2.o \
				game/NPC_AI_MineMonster.o \
				game/NPC_AI_Rancor.o \
				game/NPC_AI_Remote.o \
				game/NPC_AI_Seeker.o \
				game/NPC_AI_Sentry.o \
				game/NPC_AI_Sniper.o \
				game/NPC_AI_Stormtrooper.o \
				game/NPC_AI_Utils.o \
				game/NPC_AI_Wampa.o \
				game/NPC_behavior.o \
				game/NPC_combat.o \
				game/NPC_goal.o \
				game/NPC_misc.o \
				game/NPC_move.o \
				game/NPC_reactions.o \
				game/NPC_senses.o \
				game/NPC_sounds.o \
				game/NPC_spawn.o \
				game/NPC_stats.o \
				game/NPC_utils.o \
				game/q_math.o \
				game/q_shared.o \
				game/SpeederNPC.o \
				game/tri_coll_test.o \
				game/w_force.o \
				game/w_saber.o \
				game/WalkerNPC.o

CG_CFLAGS		=	-m32 -I. -DNDEBUG -D__linux__ -DMISSIONPACK -DCGAME -D_JK2 -DFINAL_BUILD -ggdb3

CG_OBJS			=	cgame/AnimalNPC.o \
				cgame/bg_g2_utils.o \
				cgame/bg_misc.o \
				cgame/bg_panimate.o \
				cgame/bg_pmove.o \
				cgame/bg_saber.o \
				cgame/bg_saberLoad.o \
				cgame/bg_saga.o \
				cgame/bg_slidemove.o \
				cgame/bg_vehicleLoad.o \
				cgame/bg_weapons.o \
				cgame/cg_consolecmds.o \
				cgame/cg_draw.o \
				cgame/cg_drawtools.o \
				cgame/cg_effects.o \
				cgame/cg_ents.o \
				cgame/cg_event.o \
				cgame/cg_info.o \
				cgame/cg_light.o \
				cgame/cg_localents.o \
				cgame/cg_main.o \
				cgame/cg_marks.o \
				cgame/cg_newDraw.o \
				cgame/cg_players.o \
				cgame/cg_playerstate.o \
				cgame/cg_predict.o \
				cgame/cg_saga.o \
				cgame/cg_scoreboard.o \
				cgame/cg_servercmds.o \
				cgame/cg_snapshot.o \
				cgame/cg_spawn.o \
				cgame/cg_strap.o \
				cgame/cg_syscalls.o \
				cgame/cg_turret.o \
				cgame/cg_view.o \
				cgame/cg_weaponinit.o \
				cgame/cg_weapons.o \
				cgame/FighterNPC.o \
				cgame/fx_blaster.o \
				cgame/fx_bowcaster.o \
				cgame/fx_bryarpistol.o \
				cgame/fx_demp2.o \
				cgame/fx_disruptor.o \
				cgame/fx_flechette.o \
				cgame/fx_force.o \
				cgame/fx_heavyrepeater.o \
				cgame/fx_rocketlauncher.o \
				cgame/q_math.o \
				cgame/q_shared.o \
				cgame/SpeederNPC.o \
				cgame/ui_shared.o \
				cgame/WalkerNPC.o

UI_CFLAGS		=	-m32 -I. -DNDEBUG -D_USRDL -D__linux__ -DUI_EXPORTS -DMISSIONPACK -D_JK2 -DFINAL_BUILD -fPIC -ggdb3
UI_OBJS			=     	ui/bg_misc.o \
				ui/bg_saga.o \
				ui/bg_vehicleLoad.o \
				ui/bg_weapons.o \
				ui/q_math.o \
				ui/q_shared.o \
				ui/ui_atoms.o \
                                ui/ui_force.o \
                                ui/ui_gameinfo.o \
                                ui/ui_main.o \
                                ui/ui_saber.o \
                                ui/ui_shared.o \
                                ui/ui_syscalls.o

ENGINE_OBJS		=   	client/FXExport.o \
			        client/FxPrimitives.o \
			        client/FxScheduler.o \
			        client/FxSystem.o \
			        client/FxTemplate.o \
			        client/FxUtil.o \
			        client/cl_cgame.o \
			        client/cl_cin.o \
			        client/cl_console.o \
			        client/cl_input.o \
			        client/cl_keys.o \
			        client/cl_main.o \
			        client/cl_net_chan.o \
			        client/cl_parse.o \
			        client/cl_scrn.o \
			        client/cl_ui.o \
			        qcommon/CNetProfile.o \
			        qcommon/GenericParser2.o \
			        qcommon/RoffSystem.o \
			        qcommon/cm_draw.o \
			        qcommon/cm_load.o \
			        qcommon/cm_patch.o \
			        qcommon/cm_polylib.o \
			        qcommon/cm_randomterrain.o \
			        qcommon/cm_shader.o \
			        qcommon/cm_terrain.o \
			        qcommon/cm_terrainmap.o \
			        qcommon/cm_test.o \
			        qcommon/cm_trace.o \
			        qcommon/cmd_common.o \
			        qcommon/cmd_pc.o \
			        qcommon/common.o \
			        qcommon/cvar.o \
			        qcommon/exe_headers.o \
			        qcommon/files_common.o \
			        qcommon/files_pc.o \
			        qcommon/hstring.o \
			        qcommon/huffman.o \
			        qcommon/md4.o \
			        qcommon/msg.o \
			        qcommon/net_chan.o \
			        qcommon/q_math.o \
			        qcommon/q_shared.o \
			        qcommon/stringed_ingame.o \
			        qcommon/stringed_interface.o \
			        qcommon/unzip.o \
			        qcommon/vm.o \
			        qcommon/vm_interpreted.o \
			        unix_ded/vm_x86.o \
			        qcommon/z_memman_pc.o \
			        \
			        sdl/sdl_glimp.o \
			        sdl/sdl_snd.o \
			        sdl/sdl_input.o \
			        sys/sys_main.o \
			        sys/sys_unix.o \
			        unix/unix_main.o \
			        sys/sys_net.o \
			        unix/snapvector.o \
			        \
			        server/sv_bot.o \
			        server/sv_ccmds.o \
			        server/sv_client.o \
			        server/sv_game.o \
			        server/sv_init.o \
			        server/sv_main.o \
			        server/sv_net_chan.o \
			        server/sv_snapshot.o \
			        server/sv_world.o \
			        server/NPCNav/gameCallbacks.o \
			        server/NPCNav/navigator.o \
			        client/snd_ambient.o \
			        client/snd_dma.o \
			        client/snd_mem.o \
			        client/snd_mix.o \
			        client/snd_mp3.o \
			        client/snd_music.o \
			        mp3code/cdct.o \
			        mp3code/csbt.o \
			        mp3code/csbtb.o \
			        mp3code/csbtl3.o \
			        mp3code/cup.o \
			        mp3code/cupini.o \
			        mp3code/cupl1.o \
			        mp3code/cupl3.o \
			        mp3code/cwin.o \
			        mp3code/cwinb.o \
			        mp3code/cwinm.o \
			        mp3code/hwin.o \
			        mp3code/l3dq.o \
			        mp3code/l3init.o \
			        mp3code/mdct.o \
			        mp3code/mhead.o \
			        mp3code/msis.o \
			        mp3code/towave.o \
			        mp3code/uph.o \
			        mp3code/upsf.o \
			        mp3code/wavep.o \
			        renderer/matcomp.o \
			        renderer/tr_WorldEffects.o \
			        renderer/tr_animation.o \
			        renderer/tr_arioche.o \
			        renderer/tr_backend.o \
			        renderer/tr_bsp.o \
			        renderer/tr_cmds.o \
			        renderer/tr_curve.o \
			        renderer/tr_font.o \
			        renderer/tr_ghoul2.o \
			        renderer/tr_image.o \
			        renderer/tr_init.o \
			        renderer/tr_light.o \
			        renderer/tr_main.o \
			        renderer/tr_marks.o \
			        renderer/tr_mesh.o \
			        renderer/tr_model.o \
			        renderer/tr_noise.o \
			        renderer/tr_quicksprite.o \
			        renderer/tr_scene.o \
			        renderer/tr_shade.o \
			        renderer/tr_shade_calc.o \
			        renderer/tr_shader.o \
			        renderer/tr_shadows.o \
			        renderer/tr_sky.o \
			        renderer/tr_surface.o \
			        renderer/tr_surfacesprites.o \
			        renderer/tr_terrain.o \
			        renderer/tr_world.o \
			        jpeg-6/jcapimin.o \
			        jpeg-6/jccoefct.o \
			        jpeg-6/jccolor.o \
			        jpeg-6/jcdctmgr.o \
			        jpeg-6/jchuff.o \
			        jpeg-6/jcinit.o \
			        jpeg-6/jcmainct.o \
			        jpeg-6/jcmarker.o \
			        jpeg-6/jcmaster.o \
			        jpeg-6/jcomapi.o \
			        jpeg-6/jcparam.o \
			        jpeg-6/jcphuff.o \
			        jpeg-6/jcprepct.o \
			        jpeg-6/jcsample.o \
			        jpeg-6/jctrans.o \
			        jpeg-6/jdapimin.o \
			        jpeg-6/jdapistd.o \
			        jpeg-6/jdatadst.o \
			        jpeg-6/jdatasrc.o \
			        jpeg-6/jdcoefct.o \
			        jpeg-6/jdcolor.o \
			        jpeg-6/jddctmgr.o \
			        jpeg-6/jdhuff.o \
			        jpeg-6/jdinput.o \
			        jpeg-6/jdmainct.o \
			        jpeg-6/jdmarker.o \
			        jpeg-6/jdmaster.o \
			        jpeg-6/jdpostct.o \
			        jpeg-6/jdsample.o \
			        jpeg-6/jdtrans.o \
			        jpeg-6/jerror.o \
			        jpeg-6/jfdctflt.o \
			        jpeg-6/jidctflt.o \
			        jpeg-6/jmemmgr.o \
			        jpeg-6/jmemnobs.o \
			        jpeg-6/jutils.o \
			        png/png.o \
			        ghoul2/G2_API.o \
			        ghoul2/G2_bolts.o \
			        ghoul2/G2_bones.o \
			        ghoul2/G2_misc.o \
			        ghoul2/G2_surfaces.o \
			        RMG/RM_Area.o \
			        RMG/RM_Instance.o \
			        RMG/RM_InstanceFile.o \
			        RMG/RM_Instance_BSP.o \
			        RMG/RM_Instance_Group.o \
			        RMG/RM_Instance_Random.o \
			        RMG/RM_Instance_Void.o \
			        RMG/RM_Manager.o \
			        RMG/RM_Mission.o \
			        RMG/RM_Objective.o \
			        RMG/RM_Path.o \
			        RMG/RM_Terrain.o \
			        botlib/be_aas_bspq3.o \
			        botlib/be_aas_cluster.o \
			        botlib/be_aas_debug.o \
			        botlib/be_aas_entity.o \
			        botlib/be_aas_file.o \
			        botlib/be_aas_main.o \
			        botlib/be_aas_move.o \
			        botlib/be_aas_optimize.o \
			        botlib/be_aas_reach.o \
			        botlib/be_aas_route.o \
			        botlib/be_aas_routealt.o \
			        botlib/be_aas_sample.o \
			        botlib/be_ai_char.o \
			        botlib/be_ai_chat.o \
			        botlib/be_ai_gen.o \
			        botlib/be_ai_goal.o \
			        botlib/be_ai_move.o \
			        botlib/be_ai_weap.o \
			        botlib/be_ai_weight.o \
			        botlib/be_ea.o \
			        botlib/be_interface.o \
			        botlib/l_crc.o \
			        botlib/l_libvar.o \
			        botlib/l_log.o \
			        botlib/l_memory.o \
			        botlib/l_precomp.o \
			        botlib/l_script.o \
			        botlib/l_struct.o \
			        icarus/BlockStream.o \
			        icarus/GameInterface.o \
			        icarus/Instance.o \
			        icarus/Interface.o \
			        icarus/Memory.o \
			        icarus/Q3_Interface.o \
			        icarus/Q3_Registers.o \
			        icarus/Sequence.o \
			        icarus/Sequencer.o \
			        icarus/TaskManager.o \
			        zlib/ioapi.o \
			        zlib/adler32.o \
			        zlib/compress.o \
			        zlib/crc32.o \
			        zlib/deflate.o \
			        zlib/gzclose.o \
			        zlib/gzlib.o \
			        zlib/gzread.o \
			        zlib/gzwrite.o \
			        zlib/infback.o \
			        zlib/inffast.o \
			        zlib/inflate.o \
			        zlib/inftrees.o \
			        zlib/trees.o \
			        zlib/uncompr.o \
			        zlib/zutil.o

all: engine game cgame ui

game: $(G_OBJS)
	$(CC) -m32 -shared -o jampgamei386.so $(G_OBJS) -lm

game/%.o: game/%.c
	$(CC) $(G_CFLAGS) -fPIC -c $< -o $@

game/q_%.o: qcommon/q_%.c
	$(CC) $(G_CFLAGS) -fPIC -c $< -o $@

cgame: $(CG_OBJS)
	$(CC) -m32 -shared -o cgamei386.so $(CG_OBJS) -lm

cgame/q_%.o: qcommon/q_%.c
	$(CC) $(CG_CFLAGS) -fPIC -c $< -o $@

cgame/%NPC.o: game/%NPC.c
	$(CC) $(CG_CFLAGS) -fPIC -c $< -o $@

cgame/ui_%.o: ui/ui_%.c
	$(CC) $(CG_CFLAGS) -fPIC -c $< -o $@

cgame/bg_%.o: game/bg_%.c
	$(CC) $(CG_CFLAGS) -fPIC -c $< -o $@

cgame/%.o: cgame/%.c
	$(CC) $(CG_CFLAGS) -fPIC -c $< -o $@

ui: $(UI_OBJS)
	$(CC) -m32 -shared -o uii386.so $(UI_OBJS) -lm

ui/q_%.o: qcommon/q_%.c
	$(CC) $(UI_CFLAGS) -fPIC -c $< -o $@

ui/bg_%.o: game/bg_%.c
	$(CC) $(UI_CFLAGS) -fPIC -c $< -o $@

ui/%.o: ui/%.c
	$(CC) $(UI_CFLAGS) -fPIC -c $< -o $@

engine: $(ENGINE_OBJS)
	$(CXX) -m32 -o jamp $(ENGINE_OBJS) `sdl2-config --libs` -lGL -lopenal

zlib/%.o: zlib/%.c
	$(CC) $(ENGINE_CFLAGS) -c $< -o $@

mp3code/%.o: mp3code/%.c
	$(CC) $(ENGINE_CFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(ENGINE_CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(ENGINE_CFLAGS) -c $< -o $@

botlib/%.o: botlib/%.cpp
	$(CXX) $(ENGINE_CFLAGS) -DBOTLIB -c $< -o $@

unix/%.o: unix/%.nasm
	nasm -f elf -o $@  $<

clean:
	$(RM) $(ENGINE_OBJS) $(G_OBJS) $(CG_OBJS) $(UI_OBJS)
