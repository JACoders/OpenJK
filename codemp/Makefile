CC			= gcc
CXX			= g++
CXXFLAGS		+= -fpermissive -m32 -w -D_JK2 -D__linux__ -D_M_IX86 -I. -DUSE_LOCAL_HEADERS -ISDL12/include -DDEDICATED
CFLAGS			+= -fpermissive -m32 -w -D_JK2 -D__linux__ -D_M_IX86 -I. -DUSE_LOCAL_HEADERS -ISDL12/include -DDEDICATED

SRCS			= server/sv_bot.cpp \
			  server/sv_ccmds.cpp \
			  server/sv_client.cpp \
			  server/sv_game.cpp \
			  server/sv_init.cpp \
			  server/sv_main.cpp \
			  server/sv_net_chan.cpp \
			  server/sv_snapshot.cpp \
			  server/sv_world.cpp \
			  qcommon/cm_load.cpp \
			  qcommon/stringed_ingame.cpp \
			  qcommon/stringed_interface.cpp \
			  qcommon/GenericParser2.cpp \
			  qcommon/cm_patch.cpp \
			  qcommon/cm_polylib.cpp \
			  qcommon/cm_test.cpp \
			  qcommon/cm_trace.cpp \
			  qcommon/common.cpp \
			  qcommon/cvar.cpp \
			  qcommon/md4.cpp \
			  qcommon/msg.cpp \
			  qcommon/net_chan.cpp \
			  qcommon/huffman.cpp \
			  qcommon/q_math.cpp \
			  qcommon/q_shared.cpp \
			  qcommon/z_memman_pc.cpp \
			  zlib/ioapi.cpp \
			  qcommon/unzip.cpp \
			  qcommon/vm.cpp \
			  qcommon/vm_interpreted.cpp \
			  qcommon/vm_x86.cpp \
			  \
			  unix/linux_common.cpp \
			  unix/unix_main.cpp \
			  unix/unix_net.cpp \
			  unix/unix_shared.cpp \
			  \
			  ghoul2/G2_API.cpp \
			  ghoul2/G2_bolts.cpp \
			  ghoul2/G2_bones.cpp \
			  ghoul2/G2_misc.cpp \
			  ghoul2/G2_surfaces.cpp \
			  \
			  null/null_client.cpp \
			  null/null_input.cpp \
			  null/null_snddma.cpp \
			  null/null_glimp.cpp \
			  null/null_renderer.cpp \
			  \
			  icarus/BlockStream.cpp \
			  icarus/GameInterface.cpp \
			  icarus/Instance.cpp \
			  icarus/Interface.cpp \
			  icarus/Memory.cpp \
			  icarus/Q3_Interface.cpp \
			  icarus/Q3_Registers.cpp \
			  icarus/Sequence.cpp \
			  icarus/Sequencer.cpp \
			  icarus/TaskManager.cpp \
			  \
			  renderer/tr_image.cpp \
			  renderer/tr_model.cpp \
			  qcommon/RoffSystem.cpp \
			  server/NPCNav/navigator.cpp \
			  server/NPCNav/gameCallbacks.cpp \
			  qcommon/files_common.cpp \
			  qcommon/cmd_common.cpp \
			  qcommon/files_pc.cpp \
			  \
			  RMG/RM_Manager.cpp \
			  RMG/RM_Mission.cpp \
			  RMG/RM_Instance.cpp \
			  RMG/RM_InstanceFile.cpp \
			  RMG/RM_Instance_BSP.cpp \
			  RMG/RM_Instance_Group.cpp \
			  RMG/RM_Instance_Random.cpp \
			  RMG/RM_Instance_Void.cpp \
			  RMG/RM_Objective.cpp \
			  RMG/RM_Terrain.cpp \
			  RMG/RM_Path.cpp \
			  RMG/RM_Area.cpp \
			  \
			  qcommon/cm_terrain.cpp \
			  qcommon/cm_randomterrain.cpp \
			  qcommon/cm_shader.cpp \
#			  \
			  renderer/tr_WorldEffects.cpp \
			  renderer/tr_animation.cpp \
			  renderer/tr_arioche.cpp \
			  renderer/tr_backend.cpp \
			  renderer/tr_bsp.cpp \
			  renderer/tr_cmds.cpp \
			  renderer/tr_curve.cpp \
			  renderer/tr_font.cpp \
			  renderer/tr_ghoul2.cpp \
			  renderer/tr_init.cpp \
			  renderer/tr_light.cpp \
			  renderer/tr_main.cpp \
			  renderer/tr_marks.cpp \
			  renderer/tr_mesh.cpp \
			  renderer/tr_noise.cpp \
			  renderer/tr_quicksprite.cpp \
			  renderer/tr_scene.cpp \
			  renderer/tr_shade.cpp \
			  renderer/tr_shade_calc.cpp \
			  renderer/tr_shader.cpp \
			  renderer/tr_shadows.cpp \
			  renderer/tr_sky.cpp \
			  renderer/tr_surface.cpp \
			  renderer/tr_surfacesprites.cpp \
			  renderer/tr_terrain.cpp \
			  renderer/tr_world.cpp \
			  \
			  client/FXExport.cpp \
			  client/FxPrimitives.cpp \
			  client/FxScheduler.cpp \
			  client/FxSystem.cpp \
			  client/FxTemplate.cpp \
			  client/FxUtil.cpp \
			  client/cl_cgame.cpp \
			  client/cl_cin.cpp \
			  client/cl_console.cpp \
			  client/cl_input.cpp \
			  client/cl_keys.cpp \
			  client/cl_main.cpp \
			  client/cl_net_chan.cpp \
			  client/cl_parse.cpp \
			  client/cl_scrn.cpp \
			  client/cl_ui.cpp \
			  qcommon/CNetProfile.cpp \
			  qcommon/cm_draw.cpp \
			  qcommon/cm_terrainmap.cpp \
			  qcommon/cmd_common.cpp \
			  qcommon/cmd_pc.cpp \
			  qcommon/exe_headers.cpp \
			  qcommon/hstring.cpp \
			  qcommon/unzip.cpp \
			  \
			  client/snd_ambient.cpp \
			  client/snd_dma.cpp \
			  client/snd_mem.cpp \
			  client/snd_mix.cpp \
			  client/snd_mp3.cpp \
			  client/snd_music.cpp \
			  \
			  null/null_main.cpp \


OBJS			= $(SRCS:.cpp=.o)

BOTLIBSRCS		=  botlib/be_aas_bspq3.cpp \
			  botlib/be_aas_cluster.cpp \
			  botlib/be_aas_debug.cpp \
			  botlib/be_aas_entity.cpp \
			  botlib/be_aas_file.cpp \
			  botlib/be_aas_main.cpp \
			  botlib/be_aas_move.cpp \
			  botlib/be_aas_optimize.cpp \
			  botlib/be_aas_reach.cpp \
			  botlib/be_aas_route.cpp \
			  botlib/be_aas_routealt.cpp \
			  botlib/be_aas_sample.cpp \
			  botlib/be_ai_char.cpp \
			  botlib/be_ai_chat.cpp \
			  botlib/be_ai_gen.cpp \
			  botlib/be_ai_goal.cpp \
			  botlib/be_ai_move.cpp \
			  botlib/be_ai_weap.cpp \
			  botlib/be_ai_weight.cpp \
			  botlib/be_ea.cpp \
			  botlib/be_interface.cpp \
			  botlib/l_crc.cpp \
			  botlib/l_libvar.cpp \
			  botlib/l_log.cpp \
			  botlib/l_memory.cpp \
			  botlib/l_precomp.cpp \
			  botlib/l_script.cpp \
			  botlib/l_struct.cpp

BOTLIBOBJS		= $(BOTLIBSRCS:.cpp=.o)

BOTLIBCFLAGS		+= -DBOTLIB -D_JK2 -m32 -fpermissive -w -I. -c

MP3SRC			= mp3code/cdct.c \
			  mp3code/csbt.c \
			  mp3code/csbtb.c \
			  mp3code/csbtl3.c \
			  mp3code/cup.c \
			  mp3code/cupini.c \
			  mp3code/cupl1.c \
			  mp3code/cupl3.c \
			  mp3code/cwin.c \
			  mp3code/cwinb.c \
			  mp3code/cwinm.c \
			  mp3code/hwin.c \
			  mp3code/l3dq.c \
			  mp3code/l3init.c \
			  mp3code/mdct.c \
			  mp3code/mhead.c \
			  mp3code/msis.c \
			  mp3code/towave.c \
			  mp3code/uph.c \
			  mp3code/upsf.c \
			  mp3code/wavep.c \
			  renderer/matcomp.c

MP3OBJS			= $(MP3SRCS:.c=.o)

all: $(OBJS) $(MP3OBJS) botlib
	$(CXX) -m32 -o jamp $(OBJS) $(MP3OBJS) -lX11 -lXxf86dga -lXxf86vm -lopenal -lz

botlib/%.o: botlib/%.cpp
	$(CXX) -o $@ $(BOTLIBCFLAGS) $<

botlib: $(BOTLIBOBJS)
	ar rcs libbotlib.a $(BOTLIBOBJS)
	ranlib libbotlib.a

clean:
	$(RM) $(OBJS) $(MP3OBJS) $(BOTLIBOBJS)
