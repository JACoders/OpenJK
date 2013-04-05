CC			= clang
CXX			= clang++
CXXFLAGS		= -fpermissive -m32 -w -D_JK2MP -D_JK2 -DBOTLIB -D__linux__
CFLAGS		= -fpermissive -m32 -w -D_JK2MP -D_JK2 -DBOTLIB -D__linux__

BOTLIB_SRCS		= botlib/be_aas_bspq3.cpp \
			  botlib/be_aas_cluster.cpp \
			  botlib/be_aas_debug.cpp \
			  botlib/be_aas_entity.cpp \
			  botlib/be_aas_file.cpp \
			  botlib/be_aas_main.cpp \
			  botlib/be_aas_move.cpp \
			  botlib/be_aas_optimize.cpp \
			  botlib/be_aas_reach.cpp \
			  botlib/be_aas_routealt.cpp \
			  botlib/be_aas_route.cpp \
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

BOTLIB_OBJS		= $(BOTLIB_SRCS:.cpp=.o)

CLIENT_SRCS		= client/cl_cgame.cpp \
			  client/cl_cin.cpp \
			  client/cl_console.cpp \
			  client/cl_input.cpp \
			  client/cl_keys.cpp \
			  client/cl_main.cpp \
			  client/cl_net_chan.cpp \
			  client/cl_parse.cpp \
			  client/cl_scrn.cpp \
			  client/cl_ui.cpp \
			  client/FXExport.cpp \
			  client/FxPrimitives.cpp \
			  client/FxScheduler.cpp \
			  client/FxSystem.cpp \
			  client/FxTemplate.cpp \
			  client/FxUtil.cpp \
			  client/snd_ambient.cpp \
			  client/snd_dma.cpp \
			  client/snd_mem.cpp \
			  client/snd_mix.cpp \
			  client/snd_mp3.cpp \
			  client/snd_music.cpp

CLIENT_OBJS		= $(CLIENT_SRCS:.cpp=.o)

COMMON_SRCS		= qcommon/cmd_common.cpp \
			  qcommon/cmd_pc.cpp \
			  qcommon/cm_draw.cpp \
			  qcommon/cm_load.cpp \
			  qcommon/cm_patch.cpp \
			  qcommon/cm_polylib.cpp \
			  qcommon/cm_randomterrain.cpp \
			  qcommon/cm_shader.cpp \
			  qcommon/cm_terrain.cpp \
			  qcommon/cm_terrainmap.cpp \
			  qcommon/cm_test.cpp \
			  qcommon/cm_trace.cpp \
			  qcommon/CNetProfile.cpp \
			  qcommon/common.cpp \
			  qcommon/cvar.cpp \
			  qcommon/exe_headers.cpp \
			  qcommon/files_common.cpp \
			  qcommon/files_pc.cpp \
			  qcommon/GenericParser2.cpp \
			  qcommon/hstring.cpp \
			  qcommon/huffman.cpp \
			  qcommon/md4.cpp \
			  qcommon/msg.cpp \
			  qcommon/net_chan.cpp \
			  qcommon/q_math.cpp \
			  qcommon/q_shared.cpp \
			  qcommon/RoffSystem.cpp \
			  qcommon/stringed_ingame.cpp \
			  qcommon/stringed_interface.cpp \
			  qcommon/unzip.cpp \
			  qcommon/vm.cpp \
			  qcommon/vm_interpreted.cpp \
			  qcommon/vm_x86.cpp \
			  qcommon/z_memman_pc.cpp \

COMMON_OBJS		= $(COMMON_SRCS:.cpp=.o)

COMMON_C_SRCS		= game/q_math.c

COMMON_C_OBJS		= $(COMMON_C_SRCS:.c=.o)

GHOUL2_SRCS		= ghoul2/G2_API.cpp\
			  ghoul2/G2_bolts.cpp \
			  ghoul2/G2_bones.cpp \
			  ghoul2/G2_misc.cpp \
			  ghoul2/G2_surfaces.cpp

GHOUL2_OBJS		= $(GHOUL2_SRCS:.cpp=.o)

ICARUS_SRCS		= icarus/BlockStream.cpp \
			  icarus/GameInterface.cpp \
			  icarus/Instance.cpp \
			  icarus/Interface.cpp \
			  icarus/Interpreter.cpp \
			  icarus/Memory.cpp \
			  icarus/Q3_Interface.cpp \
			  icarus/Q3_Registers.cpp \
			  icarus/Sequence.cpp \
			  icarus/Sequencer.cpp \
			  icarus/TaskManager.cpp \
			  icarus/Tokenizer.cpp

ICARUS_OBJS		= $(ICARUS_SRCS:.cpp=.o)

SERVER_SRCS		= server/sv_bot.cpp \
			  server/sv_ccmds.cpp \
			  server/sv_client.cpp \
			  server/sv_game.cpp \
			  server/sv_init.cpp \
			  server/sv_main.cpp \
			  server/sv_net_chan.cpp \
			  server/sv_snapshot.cpp \
			  server/sv_world.cpp \
			  server/NPCNav/gameCallbacks.cpp \
			  server/NPCNav/navigator.cpp

SERVER_OBJS		= $(SERVER_SRCS:.cpp=.o)

UNIX_SRCS		= unix/linux_qgl.c \
			  unix/unix_main.c \
			  unix/unix_net.c \
			  unix/unix_shared.cpp

UNIX_OBJS		= $(UNIX_SRCS:.c=.o) $(UNIX_SRCS:.cpp=.o)

RENDERER_SRCS		= renderer/tr_animation.cpp \
			  renderer/tr_arioche.cpp \
			  renderer/tr_backend.cpp \
			  renderer/tr_bsp.cpp \
			  renderer/tr_cmds.cpp \
			  renderer/tr_curve.cpp \
			  renderer/tr_flares.cpp \
			  renderer/tr_font.cpp \
			  renderer/tr_ghoul2.cpp \
			  renderer/tr_image.cpp \
			  renderer/tr_init.cpp \
			  renderer/tr_light.cpp \
			  renderer/tr_main.cpp \
			  renderer/tr_marks.cpp \
			  renderer/tr_mesh.cpp \
			  renderer/tr_model.cpp \
			  renderer/tr_noise.cpp \
			  renderer/tr_quicksprite.cpp \
			  renderer/tr_scene.cpp \
			  renderer/tr_shade_calc.cpp \
			  renderer/tr_shade.cpp \
			  renderer/tr_shader.cpp \
			  renderer/tr_shadows.cpp \
			  renderer/tr_sky.cpp \
			  renderer/tr_surface.cpp \
			  renderer/tr_surfacesprites.cpp \
			  renderer/tr_terrain.cpp \
			  renderer/tr_world.cpp \
			  renderer/tr_WorldEffects.cpp

RENDERER_OBJS		= $(RENDERER_SRCS:.cpp=.o)

ZLIB_SRCS		= zlib/adler32.c \
			  zlib/deflate.c \
			  zlib/infblock.c \
			  zlib/infcodes.c \
			  zlib/inffast.c \
			  zlib/inflate.c \
			  zlib/inftrees.c \
			  zlib/infutil.c \
			  zlib/trees.c \
			  zlib/zutil.c

ZLIB_OBJS		= $(ZLIB_SRCS:.c=.o)

NULL_RENDERER_SRCS	= null/null_client.cpp \
			  null/null_glimp.cpp \
			  null/null_input.cpp \
			  null/null_main.cpp \
			  null/null_net.cpp \
			  null/null_renderer.cpp \
			  null/null_snddma.cpp

NULL_RENDERER_OBJS	= $(NULL_RENDERER_SRCS:.cpp=.o)

all: botlib common server client renderer ghoul2 icarus zlib null_renderer
	$(CXX) -o jamp -m32 $(BOTLIB_OBJS) $(COMMON_OBJS) $(COMMON_C_OBJS) $(SERVER_OBJS) $(GHOUL2_OBJS) $(RENDERER_OBJS) $(ICARUS_OBJS) $(ZLIB_OBJS) $(NULL_RENDERER_OBJS)

botlib: $(BOTLIB_OBJS)
client: $(CLIENT_OBJS)
common: $(COMMON_OBJS) $(COMMON_C_OBJS)
server: $(SERVER_OBJS)
renderer: $(RENDERER_OBJS)
unix: $(UNIX_OBJS)
ghoul2: $(GHOUL2_OBJS)
icarus: $(ICARUS_OBJS)
zlib: $(ZLIB_OBJS)
null_renderer: $(NULL_RENDERER_OBJS)

clean:
	$(RM) $(BOTLIB_OBJS) $(COMMON_OBJS) $(COMMON_C_OBJS) $(SERVER_OBJS) $(GHOUL2_OBJS) $(RENDERER_OBJS) $(CLIENT_OBJS) $(UNIX_OBJS) $(ICARUS_OBJS) $(ZLIB_OBJS) $(NULL_RENDERER_OBJS)
