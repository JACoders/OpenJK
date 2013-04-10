#
# GNU Make required
#

PLATFORM=$(shell uname|tr A-Z a-z)
PLATFORM_RELEASE=$(shell uname -r)

###
### These paths are where you probably want to change things
###

# Where we are building from (where the source code should be!)
MOUNT_DIR=../


# Where we are building to, libMesaVoodooGL.so.3.3 should be here, etc.
# the demo pk3 file should be here in demoq3/pak0.pk3 or baseq3/pak0.pk3
BDIR=$(MOUNT_DIR)/../run


# Build name
# BUILD_NAME=$(BUILD_NAME)
BUILD_NAME=jamp



#############################################################################
##
## You shouldn't have to touch anything below here
##
#############################################################################

BASEQ3_DIR=$(BDIR)/base

BD=debug$(ARCH)$(GLIBC)
BR=release$(ARCH)$(GLIBC)
CDIR=$(MOUNT_DIR)/client
SDIR=$(MOUNT_DIR)/server
RDIR=$(MOUNT_DIR)/renderer
CMDIR=$(MOUNT_DIR)/qcommon
UDIR=$(MOUNT_DIR)/unix
GDIR=$(MOUNT_DIR)/game
CGDIR=$(MOUNT_DIR)/cgame
BAIDIR=$(GDIR)
BLIBDIR=$(MOUNT_DIR)/botlib
NDIR=$(MOUNT_DIR)/null
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
FTDIR=$(MOUNT_DIR)/ft2
JPDIR=$(MOUNT_DIR)/jpeg-6
SPLNDIR=$(MOUNT_DIR)/splines
GHOUL2DIR=$(MOUNT_DIR)/ghoul2
IDIR=$(MOUNT_DIR)/icarus
ZLIB=$(MOUNT_DIR)/zlib
RMGDIR=$(MOUNT_DIR)/RMG

#############################################################################
# SETUP AND BUILD -- LINUX
#############################################################################

## Defaults
DLL_ONLY=false

  ifneq (,$(findstring libc6,$(shell if [ -e /lib/libc.so.6* ];then echo libc6;fi)))
    GLIBC=-glibc
  else
    GLIBC=
  endif #libc6 test

  MESADIR=../Mesa/
  ARCH=i386
  RPMARCH=i386
  VENDOR=unknown
  DLL_ONLY=false

  # bk001205: no mo'  -I/usr/include/glide, no FX
  # bk001205: no mo'  -Dstricmp=strcasecmp, see q_shared.h
  #BASE_CFLAGS =  -pipe -fsigned-char -x c++ -D_JK2 -D_M_IX86  -I/home/drews/STLport-4.5.3/stlport -I/opt/intel/compiler50/ia32/include
  #BASE_CFLAGS =  -pipe -fsigned-char -I.. -D_JK2 -D_M_IX86
  BASE_CFLAGS =  -D_JK2 -D_M_IX86 -w -I../ -fpermissive -m32
  # rcg010216: DLL_ONLY for PPC
  ifeq ($(strip $(DLL_ONLY)),true)
    BASE_CFLAGS += -DDLL_ONLY
  endif

  ifeq ($(PLATFORM),darwin) #TODO: This should not be mixed in with linux stuff
    BASE_CFLAGS += -DMACOS_X
  endif

  #GL_CFLAGS = -I$(MESADIR)/include -I/usr/X11R6/include
  GL_CFAGS = -I/usr/X11R6/include 

  # bk001204 - need -O for -Wall for uninitialized
  # bk001205 - took out -O to get assertions (NDEBUG)
  # bk001206 - MALLOC_CHECK in addition to ZONE_DEBUG
  # TTimo 03/30/2001 temporary took out -Werror for initial merge
  DEBUG_CFLAGS=$(BASE_CFLAGS) -g
#  DEBUG_CFLAGS=$(BASE_CFLAGS) -g  -Wall -Werror -O
  ifeq ($(ARCH),axp)
    CC=pgcc
    RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O2 -unroll
  else
    ifeq ($(ARCH),ppc)
      NEWPGCC=/loki/global/ppc/bin/gcc
      CC=$(NEWPGCC)
      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O2 -pipe -unroll 
    else
      #NEWPGCC=/usr/local/gcc-2.95.2/bin/gcc # bk001205
      #NEWPGCC=/loki/global/x86/bin/gcc
      #NEWPGCC=/usr/bin/gcc 
      #NEWPGCC=/usr/local/bin/gcc
      CC=gcc
      CXX=g++
# TTimo: legacy RELEASE_CFLAGS
# NOTE: the -fomit-frame-pointer option leads to an unstable binary on my test box if it was built on the main box
#   but building on the Mdk 7.2 baseline seems to work
# -O2 -unroll
      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O2 -unroll -DFINAL_BUILD
# TTimo: use this for building on P3 gcc 2.95.3 libc2.2 for all targets (experimental! -fomit-fram-pointer removed)
#      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce
    endif
  endif

  LIBEXT=a

ifeq ($(PLATFORM),darwin) #TODO: This should not be mixed in with linux stuff
  SHLIBEXT=dylib
else
  SHLIBEXT=so
endif
  SHLIBCFLAGS=-D_JK2 -DJK2AWARDS -DQAGAME -DMISSIONPACK -m32
  SHLIBLDFLAGS=-m32 -shared $(LDFLAGS)

  ARFLAGS=ar rv
  RANLIB=ranlib

  THREAD_LDFLAGS=-lpthread
  #LDFLAGS=/opt/sxl/lib/sxlgcc3.a -lpthread -ldl -lm -lstdc++ -static -Wl --gc-sections
  LDFLAGS= -ldl -lm
  GLLDFLAGS=-L/usr/X11R6/lib -L$(MESADIR)/lib -lX11 -lXext -lXxf86dga -lXxf86vm

  TARGETS=\
	  $(B)/$(PLATFORM)jampded \
	  $(B)/baseq3/jampgame$(ARCH).$(SHLIBEXT) 

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_CXX=$(CXX) $(CFLAGS) -o $@ -c $<
DO_SMP_CC=$(CC) $(CFLAGS) -DSMP -o $@ -c $<
DO_BOT_CC=$(CC) $(CFLAGS) -use_msasm -DBOTLIB  -o $@ -c $<   # $(SHLIBCFLAGS) # bk001212
DO_BOT_CPP=$(CXX) $(CFLAGS) -use_msasm -DBOTLIB  -o $@ -c $<   # $(SHLIBCFLAGS) # bk001212
DO_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -use_msasm -o $@ -c $<
DO_SHLIB_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_AS=$(CC) $(CFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<
ifeq ($(PLATFORM),darwin) #TODO: This should not be mixed in with linux stuff
  DO_NASM=nasm -f macho -o $@  $<
else
  DO_NASM=nasm -f elf -o $@  $<
endif
DO_DED_CC=$(CC) -DDEDICATED -DC_ONLY $(CFLAGS) -use_msasm -o $@ -c $<
DO_DED_CC2=$(CC) -D__linux2__ -DDEDICATED -DC_ONLY $(CFLAGS) -o $@ -c $<
DO_DED_CPP=$(CXX) -DDEDICATED -DC_ONLY $(CFLAGS) -o $@ -c $<

#DO_LCC=$(LCC) -o $@ -S -Wf-target=bytecode -Wf-g -DQ3_VM -I$(CGDIR) -I$(GDIR)  -I$(UIDIR)  $< 

#### DEFAULT TARGET
default:build_debug

debug: build_debug
release: build_release

build_debug:
	$(MAKE)  targets  B=$(BD) CFLAGS="$(DEBUG_CFLAGS)"

build_release:
	$(MAKE)  targets  B=$(BR) CFLAGS="$(RELEASE_CFLAGS)"

#Build both debug and release builds
all:build_debug build_release


targets:makedirs $(TARGETS)


makedirs:
	@if [ ! -d $(B) ];then mkdir $(B);fi
#	@if [ ! -d $(B)/client ];then mkdir $(B)/client;fi
	@if [ ! -d $(B)/ded ];then mkdir $(B)/ded;fi
	@if [ ! -d $(B)/zlib ];then mkdir $(B)/zlib;fi
#	@if [ ! -d $(B)/ref ];then mkdir $(B)/ref;fi
#	@if [ ! -d $(B)/ft2 ];then mkdir $(B)/ft2;fi
	@if [ ! -d $(B)/baseq3 ];then mkdir $(B)/baseq3;fi
#	@if [ ! -d $(B)/baseq3/cgame ];then mkdir $(B)/baseq3/cgame;fi
	@if [ ! -d $(B)/baseq3/game ];then mkdir $(B)/baseq3/game;fi
#	@if [ ! -d $(B)/baseq3/ui ];then mkdir $(B)/baseq3/ui;fi
#	@if [ ! -d $(B)/baseq3/vm ];then mkdir $(B)/baseq3/vm;fi
#	@if [ ! -d $(B)/missionpack ];then mkdir $(B)/missionpack;fi
#	@if [ ! -d $(B)/missionpack/cgame ];then mkdir $(B)/missionpack/cgame;fi
#	@if [ ! -d $(B)/missionpack/game ];then mkdir $(B)/missionpack/game;fi
#	@if [ ! -d $(B)/missionpack/ui ];then mkdir $(B)/missionpack/ui;fi
#	@if [ ! -d $(B)/missionpack/vm ];then mkdir $(B)/missionpack/vm;fi
#	@if [ ! -d $(B)/q3static ];then mkdir $(B)/q3static;fi


#############################################################################
# DEDICATED SERVER                        
#############################################################################

Q3DOBJ = \
	$(B)/ded/sv_bot.o \
	$(B)/ded/sv_client.o \
	$(B)/ded/sv_ccmds.o \
	$(B)/ded/sv_game.o \
	$(B)/ded/sv_init.o \
	$(B)/ded/sv_main.o \
	$(B)/ded/sv_net_chan.o \
	$(B)/ded/sv_snapshot.o \
	$(B)/ded/sv_world.o \
	\
	$(B)/ded/cm_load.o \
	$(B)/ded/stringed_ingame.o \
	$(B)/ded/stringed_interface.o \
	$(B)/ded/GenericParser2.o \
	$(B)/ded/cm_patch.o \
	$(B)/ded/cm_polylib.o \
	$(B)/ded/cm_test.o \
	$(B)/ded/cm_trace.o \
	$(B)/ded/common.o \
	$(B)/ded/cvar.o \
	$(B)/ded/md4.o \
	$(B)/ded/msg.o \
	$(B)/ded/net_chan.o \
	$(B)/ded/huffman.o \
	\
	$(B)/ded/q_math.o \
	$(B)/ded/q_shared.o \
	$(B)/ded/z_memman_pc.o \
	\
	\
	$(B)/ded/ioapi.o \
	$(B)/ded/unzip.o \
	$(B)/ded/vm.o \
	$(B)/ded/vm_interpreted.o \
	\
	$(B)/ded/be_aas_bspq3.o \
	$(B)/ded/be_aas_cluster.o \
	$(B)/ded/be_aas_debug.o \
	$(B)/ded/be_aas_entity.o \
	$(B)/ded/be_aas_file.o \
	$(B)/ded/be_aas_main.o \
	$(B)/ded/be_aas_move.o \
	$(B)/ded/be_aas_optimize.o \
	$(B)/ded/be_aas_reach.o \
	$(B)/ded/be_aas_route.o \
	$(B)/ded/be_aas_routealt.o \
	$(B)/ded/be_aas_sample.o \
	$(B)/ded/be_ai_char.o \
	$(B)/ded/be_ai_chat.o \
	$(B)/ded/be_ai_gen.o \
	$(B)/ded/be_ai_goal.o \
	$(B)/ded/be_ai_move.o \
	$(B)/ded/be_ai_weap.o \
	$(B)/ded/be_ai_weight.o \
	$(B)/ded/be_ea.o \
	$(B)/ded/be_interface.o \
	$(B)/ded/l_crc.o \
	$(B)/ded/l_libvar.o \
	$(B)/ded/l_log.o \
	$(B)/ded/l_memory.o \
	$(B)/ded/l_precomp.o \
	$(B)/ded/l_script.o \
	$(B)/ded/l_struct.o \
	\
	$(B)/ded/linux_common.o \
	$(B)/ded/unix_main.o \
	$(B)/ded/unix_net.o \
	$(B)/ded/unix_shared.o \
	\
	$(B)/ded/G2_API.o \
	$(B)/ded/G2_bolts.o \
	$(B)/ded/G2_surfaces.o \
	$(B)/ded/G2_misc.o \
	\
	$(B)/ded/null_client.o \
	$(B)/ded/null_input.o \
	$(B)/ded/null_snddma.o \
	$(B)/ded/null_glimp.o \
	$(B)/ded/null_renderer.o \
	\
	$(B)/ded/BlockStream.o \
	$(B)/ded/GameInterface.o \
	$(B)/ded/Instance.o \
	$(B)/ded/Interface.o \
	$(B)/ded/Memory.o \
	$(B)/ded/Q3_Interface.o \
	$(B)/ded/Q3_Registers.o \
	$(B)/ded/Sequence.o \
	$(B)/ded/Sequencer.o \
	$(B)/ded/TaskManager.o \
	\
	$(B)/ded/tr_model.o \
	$(B)/ded/tr_image.o \
	$(B)/ded/RoffSystem.o \
	$(B)/ded/tr_ghoul2.o \
	$(B)/ded/matcomp.o \
	$(B)/ded/tr_init.o \
	$(B)/ded/tr_main.o \
	$(B)/ded/tr_backend.o \
	$(B)/ded/tr_mesh.o \
	$(B)/ded/G2_bones.o \
	$(B)/ded/tr_shader.o \
	$(B)/ded/cmd_pc.o \
	$(B)/ded/navigator.o \
	$(B)/ded/gameCallbacks.o \
	$(B)/ded/files_common.o \
	$(B)/ded/cmd_common.o \
	$(B)/ded/files_pc.o \
	$(B)/ded/RM_Manager.o \
	$(B)/ded/RM_Mission.o \
	$(B)/ded/RM_Instance.o \
	$(B)/ded/RM_InstanceFile.o \
	$(B)/ded/RM_Instance_BSP.o \
	$(B)/ded/RM_Instance_Group.o \
	$(B)/ded/RM_Instance_Void.o \
	$(B)/ded/RM_Instance_Random.o \
	$(B)/ded/RM_Objective.o \
	$(B)/ded/RM_Terrain.o \
	$(B)/ded/RM_Path.o \
	$(B)/ded/RM_Area.o \
	$(B)/ded/cm_terrain.o \
	$(B)/ded/cm_randomterrain.o \
	$(B)/ded/cm_shader.o \
	\
	$(B)/zlib/adler32.o \
	$(B)/zlib/compress.o \
	$(B)/zlib/crc32.o \
	$(B)/zlib/deflate.o \
	$(B)/zlib/gzclose.o \
	$(B)/zlib/gzlib.o \
	$(B)/zlib/gzread.o \
	$(B)/zlib/gzwrite.o \
	$(B)/zlib/infback.o \
	$(B)/zlib/inffast.o \
	$(B)/zlib/inflate.o \
	$(B)/zlib/inftrees.o \
	$(B)/zlib/trees.o \
	$(B)/zlib/uncompr.o \
	$(B)/zlib/zutil.o

#	$(B)/ded/tr_terrain.o \
#	\
#	$(B)/ded/null_main.o \
#	$(B)/ded/null_net.o \
#	$(B)/ded/tr_cmds.o \
#	$(B)/ded/tr_animation.o \
#	$(B)/ded/tr_bsp.o \
#	$(B)/ded/tr_curve.o \
#	$(B)/ded/tr_flares.o \
#	$(B)/ded/tr_font.o \
#	$(B)/ded/tr_light.o \
#	$(B)/ded/tr_marks.o \
#	$(B)/ded/tr_noise.o \
#	$(B)/ded/tr_quicksprite.o \
#	$(B)/ded/tr_scene.o \
#	$(B)/ded/tr_shade_calc.o \
#	$(B)/ded/tr_shade.o \
#	$(B)/ded/tr_shadows.o \
#	$(B)/ded/tr_surfacesprites.o \
#	$(B)/ded/tr_world.o \
#	$(B)/ded/tr_worldeffects.o \
#	\
#	$(B)/ded/linux_glimp.o \
#	$(B)/ded/tr_image.o 
#	$(B)/ded/linux_qgl.o 
#	$(B)/ded/snapvector.o \
#	$(B)/ded/tr_main.o
#	$(B)/ded/tr_sky.o


#	$(B)/ded/inflate.o \
#	$(B)/ded/deflate.o \
#	$(B)/ded/zipcommon.o \

#ifeq ($(ARCH),i386)
  Q3DOBJ += $(B)/ded/vm_x86.o $(B)/ded/ftol.o $(B)/ded/snapvector.o
#endif

ifeq ($(ARCH),ppc)
  ifeq ($(DLL_ONLY),false)
    Q3DOBJ += $(B)/ded/vm_ppc.o
  endif
endif

$(B)/$(PLATFORM)jampded : $(Q3DOBJ)
	$(CXX) -m32 -o $@ $(Q3DOBJ) $(LDFLAGS)

$(B)/ded/sv_bot.o : $(SDIR)/sv_bot.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_client.o : $(SDIR)/sv_client.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_ccmds.o : $(SDIR)/sv_ccmds.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_game.o : $(SDIR)/sv_game.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_init.o : $(SDIR)/sv_init.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_main.o : $(SDIR)/sv_main.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_net_chan.o : $(SDIR)/sv_net_chan.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_snapshot.o : $(SDIR)/sv_snapshot.cpp; $(DO_DED_CPP) 
$(B)/ded/sv_world.o : $(SDIR)/sv_world.cpp; $(DO_DED_CPP) 
$(B)/ded/gameCallbacks.o : $(SDIR)/NPCNav/gameCallbacks.cpp; $(DO_DED_CPP) 
$(B)/ded/navigator.o: $(SDIR)/NPCNav/navigator.cpp; $(DO_DED_CPP) 

$(B)/ded/cm_load.o : $(CMDIR)/cm_load.cpp; $(DO_DED_CPP) 
$(B)/ded/stringed_ingame.o : $(CMDIR)/stringed_ingame.cpp; $(DO_DED_CPP) 
$(B)/ded/stringed_interface.o : $(CMDIR)/stringed_interface.cpp; $(DO_DED_CPP) 
$(B)/ded/GenericParser2.o : $(CMDIR)/GenericParser2.cpp; $(DO_DED_CPP) 
$(B)/ded/cm_shader.o : $(CMDIR)/cm_shader.cpp; $(DO_DED_CPP) 
$(B)/ded/cm_polylib.o : $(CMDIR)/cm_polylib.cpp; $(DO_DED_CPP) 
$(B)/ded/cm_test.o : $(CMDIR)/cm_test.cpp; $(DO_DED_CPP) 
$(B)/ded/cm_trace.o : $(CMDIR)/cm_trace.cpp; $(DO_DED_CPP) 
$(B)/ded/cm_patch.o : $(CMDIR)/cm_patch.cpp; $(DO_DED_CPP) 
$(B)/ded/common.o : $(CMDIR)/common.cpp; $(DO_DED_CPP) 
$(B)/ded/cvar.o : $(CMDIR)/cvar.cpp; $(DO_DED_CPP) 
$(B)/ded/files.o : $(CMDIR)/files.cpp; $(DO_DED_CPP) 
$(B)/ded/md4.o : $(CMDIR)/md4.cpp; $(DO_DED_CPP) 
$(B)/ded/msg.o : $(CMDIR)/msg.cpp; $(DO_DED_CPP) 
$(B)/ded/net_chan.o : $(CMDIR)/net_chan.cpp; $(DO_DED_CPP) 
$(B)/ded/huffman.o : $(CMDIR)/huffman.cpp; $(DO_DED_CPP)
$(B)/ded/q_shared.o : $(CMDIR)/q_shared.cpp; $(DO_DED_CPP)
$(B)/ded/q_math.o : $(CMDIR)/q_math.cpp; $(DO_DED_CPP)
$(B)/ded/z_memman_pc.o : $(CMDIR)/z_memman_pc.cpp; $(DO_DED_CPP)
$(B)/ded/files_common.o : $(CMDIR)/files_common.cpp; $(DO_DED_CPP)
$(B)/ded/cmd_common.o : $(CMDIR)/cmd_common.cpp; $(DO_DED_CPP)
$(B)/ded/files_pc.o : $(CMDIR)/files_pc.cpp; $(DO_DED_CPP)

$(B)/ded/RM_Manager.o : $(RMGDIR)/RM_Manager.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Mission.o : $(RMGDIR)/RM_Mission.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Instance.o: $(RMGDIR)/RM_Instance.cpp; $(DO_DED_CPP)
$(B)/ded/RM_InstanceFile.o : $(RMGDIR)/RM_InstanceFile.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Instance_Void.o : $(RMGDIR)/RM_Instance_Void.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Instance_Random.o : $(RMGDIR)/RM_Instance_Random.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Instance_Group.o : $(RMGDIR)/RM_Instance_Group.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Instance_BSP.o : $(RMGDIR)/RM_Instance_BSP.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Objective.o : $(RMGDIR)/RM_Objective.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Path.o : $(RMGDIR)/RM_Path.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Area.o : $(RMGDIR)/RM_Area.cpp; $(DO_DED_CPP)
$(B)/ded/RM_Terrain.o : $(RMGDIR)/RM_Terrain.cpp; $(DO_DED_CPP)
$(B)/ded/tr_terrain.o : $(RDIR)/tr_terrain.cpp; $(DO_DED_CPP)
$(B)/ded/cm_terrain.o : $(CMDIR)/cm_terrain.cpp; $(DO_DED_CPP)
$(B)/ded/cm_randomterrain.o : $(CMDIR)/cm_randomterrain.cpp; $(DO_DED_CPP)

$(B)/ded/G2_API.o : $(GHOUL2DIR)/G2_API.cpp; $(DO_DED_CPP)
$(B)/ded/G2_bolts.o : $(GHOUL2DIR)/G2_bolts.cpp; $(DO_DED_CPP)
$(B)/ded/G2_bones.o : $(GHOUL2DIR)/G2_bones.cpp; $(DO_DED_CPP)
$(B)/ded/G2_misc.o : $(GHOUL2DIR)/G2_misc.cpp; $(DO_DED_CPP)
$(B)/ded/G2_surfaces.o : $(GHOUL2DIR)/G2_surfaces.cpp; $(DO_DED_CPP)

$(B)/ded/RoffSystem.o : $(CMDIR)/RoffSystem.cpp; $(DO_DED_CPP)

$(B)/ded/inflate.o : $(ZLIB)/inflate.c; $(DO_DED_CPP)
$(B)/ded/deflate.o : $(ZLIB)/deflate.c; $(DO_DED_CPP)
$(B)/ded/zipcommon.o : $(ZLIB)/zipcommon.cpp; $(DO_DED_CPP)

$(B)/zlib/%.o : $(ZLIB)/%.c; $(DO_DED_CC)

$(B)/ded/tr_model.o : $(RDIR)/tr_model.cpp; $(DO_DED_CPP)
$(B)/ded/tr_image.o : $(RDIR)/tr_image.cpp; $(DO_DED_CPP)
$(B)/ded/tr_ghoul2.o : $(RDIR)/tr_ghoul2.cpp; $(DO_DED_CPP)
$(B)/ded/tr_shader.o : $(RDIR)/tr_shader.cpp; $(DO_DED_CPP)
$(B)/ded/tr_sky.o : $(RDIR)/tr_shader.cpp; $(DO_DED_CPP)
$(B)/ded/tr_cmds.o : $(RDIR)/tr_cmds.cpp; $(DO_DED_CPP)
$(B)/ded/tr_backend.o : $(RDIR)/tr_backend.cpp; $(DO_DED_CPP)
$(B)/ded/tr_animation.o : $(RDIR)/tr_animation.cpp; $(DO_DED_CPP)
$(B)/ded/tr_bsp.o : $(RDIR)/tr_bsp.cpp; $(DO_DED_CPP)
$(B)/ded/tr_curve.o : $(RDIR)/tr_curve.cpp; $(DO_DED_CPP)
$(B)/ded/tr_flares.o : $(RDIR)/tr_flares.cpp; $(DO_DED_CPP)
$(B)/ded/tr_font.o : $(RDIR)/tr_font.cpp; $(DO_DED_CPP)
$(B)/ded/tr_init.o : $(RDIR)/tr_init.cpp; $(DO_DED_CPP)
$(B)/ded/tr_light.o : $(RDIR)/tr_light.cpp; $(DO_DED_CPP)
$(B)/ded/tr_main.o : $(RDIR)/tr_main.cpp; $(DO_DED_CPP)
$(B)/ded/tr_marks.o : $(RDIR)/tr_marks.cpp; $(DO_DED_CPP)
$(B)/ded/tr_mesh.o : $(RDIR)/tr_mesh.cpp; $(DO_DED_CPP)
$(B)/ded/tr_noise.o : $(RDIR)/tr_noise.cpp; $(DO_DED_CPP)
$(B)/ded/tr_quicksprite.o : $(RDIR)/tr_quicksprite.cpp; $(DO_DED_CPP)
$(B)/ded/tr_scene.o : $(RDIR)/tr_scene.cpp; $(DO_DED_CPP)
$(B)/ded/tr_shade.o : $(RDIR)/tr_shade.cpp; $(DO_DED_CPP)
$(B)/ded/tr_shade_calc.o : $(RDIR)/tr_shade_calc.cpp; $(DO_DED_CPP)
$(B)/ded/tr_shadows.o : $(RDIR)/tr_shadows.cpp; $(DO_DED_CPP)
$(B)/ded/tr_surface.o : $(RDIR)/tr_surface.cpp; $(DO_DED_CPP)
$(B)/ded/tr_surfacesprites.o : $(RDIR)/tr_surfacesprites.cpp; $(DO_DED_CPP)
$(B)/ded/tr_world.o : $(RDIR)/tr_world.cpp; $(DO_DED_CPP)
$(B)/ded/tr_worldeffects.o : $(RDIR)/tr_worldeffects.cpp; $(DO_DED_CPP)
$(B)/ded/matcomp.o : $(RDIR)/matcomp.c; $(DO_DED_CPP)


$(B)/ded/be_aas_bspq3.o : $(BLIBDIR)/be_aas_bspq3.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_cluster.o : $(BLIBDIR)/be_aas_cluster.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_debug.o : $(BLIBDIR)/be_aas_debug.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_entity.o : $(BLIBDIR)/be_aas_entity.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_file.o : $(BLIBDIR)/be_aas_file.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_main.o : $(BLIBDIR)/be_aas_main.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_move.o : $(BLIBDIR)/be_aas_move.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_optimize.o : $(BLIBDIR)/be_aas_optimize.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_reach.o : $(BLIBDIR)/be_aas_reach.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_route.o : $(BLIBDIR)/be_aas_route.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_routealt.o : $(BLIBDIR)/be_aas_routealt.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_aas_sample.o : $(BLIBDIR)/be_aas_sample.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_char.o : $(BLIBDIR)/be_ai_char.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_chat.o : $(BLIBDIR)/be_ai_chat.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_gen.o : $(BLIBDIR)/be_ai_gen.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_goal.o : $(BLIBDIR)/be_ai_goal.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_move.o : $(BLIBDIR)/be_ai_move.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_weap.o : $(BLIBDIR)/be_ai_weap.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ai_weight.o : $(BLIBDIR)/be_ai_weight.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_ea.o : $(BLIBDIR)/be_ea.cpp; $(DO_BOT_CPP) 
$(B)/ded/be_interface.o : $(BLIBDIR)/be_interface.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_crc.o : $(BLIBDIR)/l_crc.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_libvar.o : $(BLIBDIR)/l_libvar.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_log.o : $(BLIBDIR)/l_log.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_memory.o : $(BLIBDIR)/l_memory.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_precomp.o : $(BLIBDIR)/l_precomp.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_script.o : $(BLIBDIR)/l_script.cpp; $(DO_BOT_CPP) 
$(B)/ded/l_struct.o : $(BLIBDIR)/l_struct.cpp; $(DO_BOT_CPP) 

$(B)/ded/linux_common.o : $(UDIR)/linux_common.c; $(DO_DED_CPP)
$(B)/ded/linux_glimp.o : $(UDIR)/linux_glimp.c; $(DO_DED_CC)
$(B)/ded/unix_main.o : $(UDIR)/unix_main.cpp; $(DO_DED_CPP) 
$(B)/ded/unix_net.o : $(UDIR)/unix_net.cpp; $(DO_DED_CPP) 
$(B)/ded/unix_shared.o : $(UDIR)/unix_shared.cpp; $(DO_DED_CPP) 
$(B)/ded/linux_qgl.o : $(UDIR)/linux_qgl.c; $(DO_DED_CC)
$(B)/ded/null_client.o : $(NDIR)/null_client.cpp; $(DO_DED_CPP) 
$(B)/ded/null_input.o : $(NDIR)/null_input.cpp; $(DO_DED_CPP) 
$(B)/ded/null_snddma.o : $(NDIR)/null_snddma.cpp; $(DO_DED_CPP) 
$(B)/ded/null_glimp.o : $(NDIR)/null_glimp.cpp; $(DO_DED_CPP)
$(B)/ded/null_main.o : $(NDIR)/null_main.cpp; $(DO_DED_CPP)
$(B)/ded/null_net.o : $(NDIR)/null_net.cpp; $(DO_DED_CPP)
$(B)/ded/null_renderer.o : $(NDIR)/null_renderer.cpp; $(DO_DED_CPP)
$(B)/ded/ioapi.o : $(ZLIB)/ioapi.c; $(DO_DED_CC)
$(B)/ded/unzip.o : $(CMDIR)/unzip.cpp; $(DO_DED_CPP) 
$(B)/ded/vm.o : $(CMDIR)/vm.cpp; $(DO_DED_CPP) 
$(B)/ded/vm_interpreted.o : $(CMDIR)/vm_interpreted.cpp; $(DO_DED_CPP) 
$(B)/ded/cmd_pc.o : $(CMDIR)/cmd_pc.cpp; $(DO_DED_CPP)

$(B)/ded/BlockStream.o : $(IDIR)/BlockStream.cpp; $(DO_DED_CPP)
$(B)/ded/GameInterface.o : $(IDIR)/GameInterface.cpp; $(DO_DED_CPP)
$(B)/ded/Instance.o : $(IDIR)/Instance.cpp; $(DO_DED_CPP)
$(B)/ded/Interface.o : $(IDIR)/Interface.cpp; $(DO_DED_CPP)
$(B)/ded/Memory.o : $(IDIR)/Memory.cpp; $(DO_DED_CPP)
$(B)/ded/Q3_Interface.o : $(IDIR)/Q3_Interface.cpp; $(DO_DED_CPP)
$(B)/ded/Q3_Registers.o : $(IDIR)/Q3_Registers.cpp; $(DO_DED_CPP)
$(B)/ded/Sequence.o: $(IDIR)/Sequence.cpp; $(DO_DED_CPP)
$(B)/ded/Sequencer.o : $(IDIR)/Sequencer.cpp; $(DO_DED_CPP)
$(B)/ded/TaskManager.o: $(IDIR)/TaskManager.cpp; $(DO_DED_CPP)

#ifeq ($(ARCH),i386)
$(B)/ded/vm_x86.o : $(UDIR)/vm_x86.cpp; $(DO_DED_CPP) 
$(B)/ded/ftol.o : $(UDIR)/ftol.nasm; $(DO_NASM)
$(B)/ded/snapvector.o : $(UDIR)/snapvector.nasm; $(DO_NASM) 
#endif

ifeq ($(ARCH),ppc)
ifeq ($(DLL_ONLY),false)
$(B)/ded/vm_ppc.o : $(CMDIR)/vm_ppc.c; $(DO_DED_CC)
endif
endif


#############################################################################
## QVM
#############################################################################

$(B)/baseq3/vm/cgame.qvm:
	cd $(CGDIR) && ./cgame.sh
	mv /tmp/quake3/baseq3/vm/cgame.qvm $@

$(B)/baseq3/vm/ui.qvm:
	cd $(Q3UIDIR) && ./q3_ui.sh
	mv /tmp/quake3/baseq3/vm/ui.qvm $@

$(B)/baseq3/vm/qagame.qvm:
	cd $(GDIR) && ./game.sh
	mv /tmp/quake3/baseq3/vm/qagame.qvm $@

$(B)/missionpack/vm/cgame.qvm:
	cd $(CGDIR) && ./cgame_ta.sh
	mv /tmp/quake3/missionpack/vm/cgame.qvm $@

$(B)/missionpack/vm/qagame.qvm:
	cd $(GDIR) && ./game_ta.sh
	mv /tmp/quake3/missionpack/vm/qagame.qvm $@

$(B)/missionpack/vm/ui.qvm:
	cd $(UIDIR) && ./ui.sh
	mv /tmp/quake3/missionpack/vm/ui.qvm $@



#############################################################################
## BASEQ3 CGAME
#############################################################################

Q3CGOBJ = \
	$(B)/baseq3/cgame/bg_misc.o \
	$(B)/baseq3/cgame/bg_pmove.o \
	$(B)/baseq3/cgame/bg_slidemove.o \
	$(B)/baseq3/cgame/cg_consolecmds.o \
	$(B)/baseq3/cgame/cg_draw.o \
	$(B)/baseq3/cgame/cg_drawtools.o \
	$(B)/baseq3/cgame/cg_effects.o \
	$(B)/baseq3/cgame/cg_ents.o \
	$(B)/baseq3/cgame/cg_event.o \
	$(B)/baseq3/cgame/cg_info.o \
	$(B)/baseq3/cgame/cg_localents.o \
	$(B)/baseq3/cgame/cg_main.o \
	$(B)/baseq3/cgame/cg_marks.o \
	$(B)/baseq3/cgame/cg_players.o \
	$(B)/baseq3/cgame/cg_playerstate.o \
	$(B)/baseq3/cgame/cg_predict.o \
	$(B)/baseq3/cgame/cg_scoreboard.o \
	$(B)/baseq3/cgame/cg_servercmds.o \
	$(B)/baseq3/cgame/cg_snapshot.o \
	$(B)/baseq3/cgame/cg_syscalls.o \
	$(B)/baseq3/cgame/cg_view.o \
	$(B)/baseq3/cgame/cg_weapons.o \
	$(B)/baseq3/cgame/q_math.o \
	$(B)/baseq3/cgame/q_shared.o

$(B)/baseq3/cgame$(ARCH).$(SHLIBEXT) : $(Q3CGOBJ)
	$(CC)  $(SHLIBLDFLAGS) -o $@ $(Q3CGOBJ)

$(B)/baseq3/cgame/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)         
$(B)/baseq3/cgame/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_consolecmds.o : $(CGDIR)/cg_consolecmds.c; $(DO_SHLIB_CC)       
$(B)/baseq3/cgame/cg_draw.o : $(CGDIR)/cg_draw.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_drawtools.o : $(CGDIR)/cg_drawtools.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_effects.o : $(CGDIR)/cg_effects.c; $(DO_SHLIB_CC)           
$(B)/baseq3/cgame/cg_ents.o : $(CGDIR)/cg_ents.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/cg_event.o : $(CGDIR)/cg_event.c; $(DO_SHLIB_CC)            
$(B)/baseq3/cgame/cg_info.o : $(CGDIR)/cg_info.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/cg_localents.o : $(CGDIR)/cg_localents.c; $(DO_SHLIB_CC)           
$(B)/baseq3/cgame/cg_main.o : $(CGDIR)/cg_main.c; $(DO_SHLIB_CC)                   
$(B)/baseq3/cgame/cg_marks.o : $(CGDIR)/cg_marks.c; $(DO_SHLIB_CC)                 
$(B)/baseq3/cgame/cg_players.o : $(CGDIR)/cg_players.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_playerstate.o : $(CGDIR)/cg_playerstate.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_predict.o : $(CGDIR)/cg_predict.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_scoreboard.o : $(CGDIR)/cg_scoreboard.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_servercmds.o : $(CGDIR)/cg_servercmds.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_snapshot.o : $(CGDIR)/cg_snapshot.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_syscalls.o : $(CGDIR)/cg_syscalls.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_view.o : $(CGDIR)/cg_view.c; $(DO_SHLIB_CC)                     
$(B)/baseq3/cgame/cg_weapons.o : $(CGDIR)/cg_weapons.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC)                       
$(B)/baseq3/cgame/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC)                     

#############################################################################
## BASEQ3 GAME
#############################################################################

Q3GOBJ = \
	$(B)/baseq3/game/g_main.o \
	$(B)/baseq3/game/AnimalNPC.o \
	$(B)/baseq3/game/FighterNPC.o \
	$(B)/baseq3/game/NPC.o \
	$(B)/baseq3/game/NPC_AI_Atst.o \
	$(B)/baseq3/game/NPC_AI_Default.o \
	$(B)/baseq3/game/NPC_AI_Droid.o \
	$(B)/baseq3/game/NPC_AI_GalakMech.o \
	$(B)/baseq3/game/NPC_AI_Grenadier.o \
	$(B)/baseq3/game/NPC_AI_Howler.o \
	$(B)/baseq3/game/NPC_AI_ImperialProbe.o \
	$(B)/baseq3/game/NPC_AI_Interrogator.o \
	$(B)/baseq3/game/NPC_AI_Jedi.o \
	$(B)/baseq3/game/NPC_AI_Mark1.o \
	$(B)/baseq3/game/NPC_AI_Mark2.o \
	$(B)/baseq3/game/NPC_AI_MineMonster.o \
	$(B)/baseq3/game/NPC_AI_Rancor.o \
	$(B)/baseq3/game/NPC_AI_Remote.o \
	$(B)/baseq3/game/NPC_AI_Seeker.o \
	$(B)/baseq3/game/NPC_AI_Sentry.o \
	$(B)/baseq3/game/NPC_AI_Sniper.o \
	$(B)/baseq3/game/NPC_AI_Stormtrooper.o \
	$(B)/baseq3/game/NPC_AI_Utils.o \
	$(B)/baseq3/game/NPC_AI_Wampa.o \
	$(B)/baseq3/game/NPC_behavior.o \
	$(B)/baseq3/game/NPC_combat.o \
	$(B)/baseq3/game/NPC_goal.o \
	$(B)/baseq3/game/NPC_misc.o \
	$(B)/baseq3/game/NPC_move.o \
	$(B)/baseq3/game/NPC_reactions.o \
	$(B)/baseq3/game/NPC_senses.o \
	$(B)/baseq3/game/NPC_sounds.o \
	$(B)/baseq3/game/NPC_spawn.o \
	$(B)/baseq3/game/NPC_stats.o \
	$(B)/baseq3/game/NPC_utils.o \
	$(B)/baseq3/game/SpeederNPC.o \
	$(B)/baseq3/game/WalkerNPC.o \
	$(B)/baseq3/game/ai_main.o \
	$(B)/baseq3/game/ai_util.o \
	$(B)/baseq3/game/ai_wpnav.o \
	$(B)/baseq3/game/bg_g2_utils.o \
	$(B)/baseq3/game/bg_misc.o \
	$(B)/baseq3/game/bg_panimate.o \
	$(B)/baseq3/game/bg_pmove.o \
	$(B)/baseq3/game/bg_saber.o \
	$(B)/baseq3/game/bg_saberLoad.o \
	$(B)/baseq3/game/bg_saga.o \
	$(B)/baseq3/game/bg_slidemove.o \
	$(B)/baseq3/game/bg_vehicleLoad.o \
	$(B)/baseq3/game/bg_weapons.o \
	$(B)/baseq3/game/g_ICARUScb.o \
	$(B)/baseq3/game/g_active.o \
	$(B)/baseq3/game/g_arenas.o \
	$(B)/baseq3/game/g_bot.o \
	$(B)/baseq3/game/g_client.o \
	$(B)/baseq3/game/g_cmds.o \
	$(B)/baseq3/game/g_combat.o \
	$(B)/baseq3/game/g_exphysics.o \
	$(B)/baseq3/game/g_items.o \
	$(B)/baseq3/game/g_log.o \
	$(B)/baseq3/game/g_mem.o \
	$(B)/baseq3/game/g_misc.o \
	$(B)/baseq3/game/g_missile.o \
	$(B)/baseq3/game/g_mover.o \
	$(B)/baseq3/game/g_nav.o \
	$(B)/baseq3/game/g_navnew.o \
	$(B)/baseq3/game/g_object.o \
	$(B)/baseq3/game/g_saga.o \
	$(B)/baseq3/game/g_session.o \
	$(B)/baseq3/game/g_spawn.o \
	$(B)/baseq3/game/g_strap.o \
	$(B)/baseq3/game/g_svcmds.o \
	$(B)/baseq3/game/g_syscalls.o \
	$(B)/baseq3/game/g_target.o \
	$(B)/baseq3/game/g_team.o \
	$(B)/baseq3/game/g_timer.o \
	$(B)/baseq3/game/g_trigger.o \
	$(B)/baseq3/game/g_turret.o \
	$(B)/baseq3/game/g_turret_G2.o \
	$(B)/baseq3/game/g_utils.o \
	$(B)/baseq3/game/g_vehicleTurret.o \
	$(B)/baseq3/game/g_vehicles.o \
	$(B)/baseq3/game/g_weapon.o \
	$(B)/baseq3/game/tri_coll_test.o \
	$(B)/baseq3/game/w_force.o \
	$(B)/baseq3/game/w_saber.o \
	\
	$(B)/baseq3/game/q_math.o \
	$(B)/baseq3/game/q_shared.o

$(B)/baseq3/jampgame$(ARCH).$(SHLIBEXT) : $(Q3GOBJ)
	$(CC)  $(SHLIBLDFLAGS) -o $@ $(Q3GOBJ)

$(B)/baseq3/game/ai_main.o : $(GDIR)/ai_main.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/ai_util.o : $(GDIR)/ai_util.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/ai_wpnav.o : $(GDIR)/ai_wpnav.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/AnimalNPC.o : $(GDIR)/AnimalNPC.c; $(DO_SHLIB_CC)   
$(B)/baseq3/game/FighterNPC.o : $(GDIR)/FighterNPC.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC.o : $(GDIR)/NPC.c; $(DO_SHLIB_CC)      
$(B)/baseq3/game/NPC_AI_Atst.o : $(GDIR)/NPC_AI_Atst.c; $(DO_SHLIB_CC)  
$(B)/baseq3/game/NPC_AI_Default.o : $(GDIR)/NPC_AI_Default.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/NPC_AI_Droid.o : $(GDIR)/NPC_AI_Droid.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/NPC_AI_GalakMech.o : $(GDIR)/NPC_AI_GalakMech.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/NPC_AI_Grenadier.o : $(GDIR)/NPC_AI_Grenadier.c; $(DO_SHLIB_CC)    
$(B)/baseq3/game/NPC_AI_Howler.o : $(GDIR)/NPC_AI_Howler.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_ImperialProbe.o : $(GDIR)/NPC_AI_ImperialProbe.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Interrogator.o : $(GDIR)/NPC_AI_Interrogator.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Jedi.o : $(GDIR)/NPC_AI_Jedi.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Mark1.o : $(GDIR)/NPC_AI_Mark1.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Mark2.o : $(GDIR)/NPC_AI_Mark2.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_MineMonster.o : $(GDIR)/NPC_AI_MineMonster.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Rancor.o : $(GDIR)/NPC_AI_Rancor.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Remote.o : $(GDIR)/NPC_AI_Remote.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Seeker.o : $(GDIR)/NPC_AI_Seeker.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Sentry.o : $(GDIR)/NPC_AI_Sentry.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Sniper.o : $(GDIR)/NPC_AI_Sniper.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Stormtrooper.o : $(GDIR)/NPC_AI_Stormtrooper.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Utils.o : $(GDIR)/NPC_AI_Utils.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_AI_Wampa.o : $(GDIR)/NPC_AI_Wampa.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_behavior.o : $(GDIR)/NPC_behavior.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_combat.o : $(GDIR)/NPC_combat.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_goal.o : $(GDIR)/NPC_goal.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_misc.o : $(GDIR)/NPC_misc.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_reactions.o : $(GDIR)/NPC_reactions.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_senses.o : $(GDIR)/NPC_senses.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_sounds.o : $(GDIR)/NPC_sounds.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_spawn.o : $(GDIR)/NPC_spawn.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_stats.o : $(GDIR)/NPC_stats.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_utils.o : $(GDIR)/NPC_utils.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/SpeederNPC.o : $(GDIR)/SpeederNPC.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/WalkerNPC.o : $(GDIR)/WalkerNPC.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/NPC_move.o : $(GDIR)/NPC_move.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_g2_utils.o : $(GDIR)/bg_g2_utils.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_lib.o : $(GDIR)/bg_lib.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_panimate.o : $(GDIR)/bg_panimate.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_saber.o : $(GDIR)/bg_saber.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_saberLoad.o : $(GDIR)/bg_saberLoad.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_saga.o : $(GDIR)/bg_saga.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_vehicleLoad.o : $(GDIR)/bg_vehicleLoad.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/bg_weapons.o : $(GDIR)/bg_weapons.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_ICARUScb.o : $(GDIR)/g_ICARUScb.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_active.o : $(GDIR)/g_active.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_arenas.o : $(GDIR)/g_arenas.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_bot.o : $(GDIR)/g_bot.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/g_client.o : $(GDIR)/g_client.c; $(DO_SHLIB_CC)      
$(B)/baseq3/game/g_cmds.o : $(GDIR)/g_cmds.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_combat.o : $(GDIR)/g_combat.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_exphysics.o : $(GDIR)/g_exphysics.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_items.o : $(GDIR)/g_items.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_log.o : $(GDIR)/g_log.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_main.o : $(GDIR)/g_main.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/g_mem.o : $(GDIR)/g_mem.c; $(DO_SHLIB_CC)               
$(B)/baseq3/game/g_misc.o : $(GDIR)/g_misc.c; $(DO_SHLIB_CC)             
$(B)/baseq3/game/g_missile.o : $(GDIR)/g_missile.c; $(DO_SHLIB_CC)        
$(B)/baseq3/game/g_mover.o : $(GDIR)/g_mover.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_nav.o : $(GDIR)/g_nav.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_navnew.o : $(GDIR)/g_navnew.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_object.o : $(GDIR)/g_object.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_saga.o : $(GDIR)/g_saga.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_session.o : $(GDIR)/g_session.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_spawn.o : $(GDIR)/g_spawn.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_strap.o : $(GDIR)/g_strap.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_svcmds.o : $(GDIR)/g_svcmds.c; $(DO_SHLIB_CC)         
$(B)/baseq3/game/g_syscalls.o : $(GDIR)/g_syscalls.c; $(DO_SHLIB_CC)    
$(B)/baseq3/game/g_target.o : $(GDIR)/g_target.c; $(DO_SHLIB_CC)        
$(B)/baseq3/game/g_team.o : $(GDIR)/g_team.c; $(DO_SHLIB_CC)           
$(B)/baseq3/game/g_timer.o : $(GDIR)/g_timer.c; $(DO_SHLIB_CC)           
$(B)/baseq3/game/g_trigger.o : $(GDIR)/g_trigger.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_turret.o : $(GDIR)/g_turret.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_turret_G2.o : $(GDIR)/g_turret_G2.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_utils.o : $(GDIR)/g_utils.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_vehicleTurret.o : $(GDIR)/g_vehicleTurret.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_vehicles.o : $(GDIR)/g_vehicles.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_weapon.o : $(GDIR)/g_weapon.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/tri_coll_test.o : $(GDIR)/tri_coll_test.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/w_force.o : $(GDIR)/w_force.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/w_saber.o : $(GDIR)/w_saber.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/q_math.o : $(CMDIR)/q_math.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/q_shared.o : $(CMDIR)/q_shared.c; $(DO_SHLIB_CC)       


#############################################################################
## BASEQ3 UI
#############################################################################

Q3UIOBJ = \
        $(B)/baseq3/ui/bg_misc.o \
        $(B)/baseq3/ui/ui_addbots.o \
        $(B)/baseq3/ui/ui_atoms.o \
        $(B)/baseq3/ui/ui_cdkey.o \
        $(B)/baseq3/ui/ui_cinematics.o \
        $(B)/baseq3/ui/ui_confirm.o \
        $(B)/baseq3/ui/ui_connect.o \
        $(B)/baseq3/ui/ui_controls2.o \
        $(B)/baseq3/ui/ui_credits.o \
        $(B)/baseq3/ui/ui_demo2.o \
        $(B)/baseq3/ui/ui_display.o \
        $(B)/baseq3/ui/ui_gameinfo.o \
        $(B)/baseq3/ui/ui_ingame.o \
        $(B)/baseq3/ui/ui_loadconfig.o \
        $(B)/baseq3/ui/ui_main.o \
        $(B)/baseq3/ui/ui_menu.o \
        $(B)/baseq3/ui/ui_mfield.o \
        $(B)/baseq3/ui/ui_mods.o \
        $(B)/baseq3/ui/ui_network.o \
        $(B)/baseq3/ui/ui_options.o \
        $(B)/baseq3/ui/ui_playermodel.o \
        $(B)/baseq3/ui/ui_players.o \
        $(B)/baseq3/ui/ui_playersettings.o \
        $(B)/baseq3/ui/ui_preferences.o \
        $(B)/baseq3/ui/ui_qmenu.o \
        $(B)/baseq3/ui/ui_removebots.o \
        $(B)/baseq3/ui/ui_saveconfig.o \
        $(B)/baseq3/ui/ui_serverinfo.o \
        $(B)/baseq3/ui/ui_servers2.o \
        $(B)/baseq3/ui/ui_setup.o \
        $(B)/baseq3/ui/ui_sound.o \
        $(B)/baseq3/ui/ui_sparena.o \
        $(B)/baseq3/ui/ui_specifyserver.o \
        $(B)/baseq3/ui/ui_splevel.o \
        $(B)/baseq3/ui/ui_sppostgame.o \
        $(B)/baseq3/ui/ui_spskill.o \
        $(B)/baseq3/ui/ui_startserver.o \
        $(B)/baseq3/ui/ui_syscalls.o \
        $(B)/baseq3/ui/ui_team.o \
        $(B)/baseq3/ui/ui_teamorders.o \
        $(B)/baseq3/ui/ui_video.o \
        \
        $(B)/baseq3/ui/q_math.o \
        $(B)/baseq3/ui/q_shared.o

$(B)/baseq3/ui$(ARCH).$(SHLIBEXT) : $(Q3UIOBJ)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3UIOBJ)

$(B)/baseq3/ui/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_addbots.o : $(Q3UIDIR)/ui_addbots.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_atoms.o : $(Q3UIDIR)/ui_atoms.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_cinematics.o : $(Q3UIDIR)/ui_cinematics.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_cdkey.o : $(Q3UIDIR)/ui_cdkey.c; $(DO_SHLIB_CC)
$(B)/baseq3/ui/ui_confirm.o : $(Q3UIDIR)/ui_confirm.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_connect.o : $(Q3UIDIR)/ui_connect.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_controls2.o : $(Q3UIDIR)/ui_controls2.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_credits.o : $(Q3UIDIR)/ui_credits.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_demo2.o : $(Q3UIDIR)/ui_demo2.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_display.o : $(Q3UIDIR)/ui_display.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_gameinfo.o : $(Q3UIDIR)/ui_gameinfo.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_ingame.o : $(Q3UIDIR)/ui_ingame.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_loadconfig.o : $(Q3UIDIR)/ui_loadconfig.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_main.o : $(Q3UIDIR)/ui_main.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_menu.o : $(Q3UIDIR)/ui_menu.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_mfield.o : $(Q3UIDIR)/ui_mfield.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_mods.o : $(Q3UIDIR)/ui_mods.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_network.o : $(Q3UIDIR)/ui_network.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_options.o : $(Q3UIDIR)/ui_options.c; $(DO_SHLIB_CC)
$(B)/baseq3/ui/ui_playermodel.o : $(Q3UIDIR)/ui_playermodel.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_players.o : $(Q3UIDIR)/ui_players.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_playersettings.o : $(Q3UIDIR)/ui_playersettings.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_preferences.o : $(Q3UIDIR)/ui_preferences.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_qmenu.o : $(Q3UIDIR)/ui_qmenu.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_quit.o : $(Q3UIDIR)/ui_quit.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_removebots.o : $(Q3UIDIR)/ui_removebots.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_saveconfig.o : $(Q3UIDIR)/ui_saveconfig.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_serverinfo.o : $(Q3UIDIR)/ui_serverinfo.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_servers2.o : $(Q3UIDIR)/ui_servers2.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_setup.o : $(Q3UIDIR)/ui_setup.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_sound.o : $(Q3UIDIR)/ui_sound.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_sparena.o : $(Q3UIDIR)/ui_sparena.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_specifyserver.o : $(Q3UIDIR)/ui_specifyserver.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_splevel.o : $(Q3UIDIR)/ui_splevel.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_sppostgame.o : $(Q3UIDIR)/ui_sppostgame.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_spskill.o : $(Q3UIDIR)/ui_spskill.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_startserver.o : $(Q3UIDIR)/ui_startserver.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_team.o : $(Q3UIDIR)/ui_team.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_teamorders.o : $(Q3UIDIR)/ui_teamorders.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_syscalls.o : $(Q3UIDIR)/ui_syscalls.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/ui_video.o : $(Q3UIDIR)/ui_video.c; $(DO_SHLIB_CC) 

# bk001205 - these wre the only SHLIB compiles in 1.17
$(B)/baseq3/ui/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC) 
$(B)/baseq3/ui/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC) 


#############################################################################
## Q3 STATIC (DEBUG) BUILD
#############################################################################

Q3SOBJ = \
	$(B)/q3static/cl_cgame.o \
	$(B)/q3static/cl_cin.o \
	$(B)/q3static/cl_console.o \
	$(B)/q3static/cl_input.o \
	$(B)/q3static/cl_keys.o \
	$(B)/q3static/cl_main.o \
	$(B)/q3static/cl_net_chan.o \
	$(B)/q3static/cl_parse.o \
	$(B)/q3static/cl_scrn.o \
	$(B)/q3static/cl_ui.o \
	\
	$(B)/q3static/cm_load.o \
	$(B)/q3static/cm_patch.o \
	$(B)/q3static/cm_polylib.o \
	$(B)/q3static/cm_test.o \
	$(B)/q3static/cm_trace.o \
	\
	$(B)/q3static/cmd.o \
	$(B)/q3static/common.o \
	$(B)/q3static/cvar.o \
	$(B)/q3static/files.o \
	$(B)/q3static/md4.o \
	$(B)/q3static/msg.o \
	$(B)/q3static/net_chan.o \
	\
	$(B)/q3static/snd_adpcm.o \
	$(B)/q3static/snd_dma.o \
	$(B)/q3static/snd_mem.o \
	$(B)/q3static/snd_mix.o \
	$(B)/q3static/snd_wavelet.o \
	\
	$(B)/q3static/sv_bot.o \
	$(B)/q3static/sv_ccmds.o \
	$(B)/q3static/sv_client.o \
	$(B)/q3static/sv_game.o \
	$(B)/q3static/sv_init.o \
	$(B)/q3static/sv_main.o \
	$(B)/q3static/sv_net_chan.o \
	$(B)/q3static/sv_snapshot.o \
	$(B)/q3static/sv_world.o \
	\
	$(B)/q3static/unzip.o \
	$(B)/q3static/vm.o \
	$(B)/q3static/vm_interpreted.o \
	\
	$(B)/q3static/be_aas_bspq3.o \
	$(B)/q3static/be_aas_cluster.o \
	$(B)/q3static/be_aas_debug.o \
	$(B)/q3static/be_aas_entity.o \
	$(B)/q3static/be_aas_file.o \
	$(B)/q3static/be_aas_main.o \
	$(B)/q3static/be_aas_move.o \
	$(B)/q3static/be_aas_optimize.o \
	$(B)/q3static/be_aas_reach.o \
	$(B)/q3static/be_aas_route.o \
	$(B)/q3static/be_aas_routealt.o \
	$(B)/q3static/be_aas_sample.o \
	$(B)/q3static/be_ai_char.o \
	$(B)/q3static/be_ai_chat.o \
	$(B)/q3static/be_ai_gen.o \
	$(B)/q3static/be_ai_goal.o \
	$(B)/q3static/be_ai_move.o \
	$(B)/q3static/be_ai_weap.o \
	$(B)/q3static/be_ai_weight.o \
	$(B)/q3static/be_ea.o \
	$(B)/q3static/be_interface.o \
	$(B)/q3static/l_crc.o \
	$(B)/q3static/l_libvar.o \
	$(B)/q3static/l_log.o \
	$(B)/q3static/l_memory.o \
	$(B)/q3static/l_precomp.o \
	$(B)/q3static/l_script.o \
	$(B)/q3static/l_struct.o \
	\
	$(B)/q3static/jcapimin.o \
	$(B)/q3static/jchuff.o   \
	$(B)/q3static/jcinit.o \
	$(B)/q3static/jccoefct.o  \
	$(B)/q3static/jccolor.o \
	$(B)/q3static/jfdctflt.o \
	$(B)/q3static/jcdctmgr.o \
	$(B)/q3static/jcphuff.o \
	$(B)/q3static/jcmainct.o \
	$(B)/q3static/jcmarker.o \
	$(B)/q3static/jcmaster.o \
	$(B)/q3static/jcomapi.o \
	$(B)/q3static/jcparam.o \
	$(B)/q3static/jcprepct.o \
	$(B)/q3static/jcsample.o \
	$(B)/q3static/jdapimin.o \
	$(B)/q3static/jdapistd.o \
	$(B)/q3static/jdatasrc.o \
	$(B)/q3static/jdcoefct.o \
	$(B)/q3static/jdcolor.o \
	$(B)/q3static/jddctmgr.o \
	$(B)/q3static/jdhuff.o \
	$(B)/q3static/jdinput.o \
	$(B)/q3static/jdmainct.o \
	$(B)/q3static/jdmarker.o \
	$(B)/q3static/jdmaster.o \
	$(B)/q3static/jdpostct.o \
	$(B)/q3static/jdsample.o \
	$(B)/q3static/jdtrans.o \
	$(B)/q3static/jerror.o \
	$(B)/q3static/jidctflt.o \
	$(B)/q3static/jmemmgr.o \
	$(B)/q3static/jmemnobs.o \
	$(B)/q3static/jutils.o \
	\
	$(B)/q3static/tr_animation.o \
	$(B)/q3static/tr_backend.o \
	$(B)/q3static/tr_bsp.o \
	$(B)/q3static/tr_cmds.o \
	$(B)/q3static/tr_curve.o \
	$(B)/q3static/tr_flares.o \
	$(B)/q3static/tr_font.o \
	$(B)/q3static/tr_image.o \
	$(B)/q3static/tr_init.o \
	$(B)/q3static/tr_light.o \
	$(B)/q3static/tr_main.o \
	$(B)/q3static/tr_marks.o \
	$(B)/q3static/tr_mesh.o \
	$(B)/q3static/tr_model.o \
	$(B)/q3static/tr_noise.o \
	$(B)/q3static/tr_scene.o \
	$(B)/q3static/tr_shade.o \
	$(B)/q3static/tr_shade_calc.o \
	$(B)/q3static/tr_shader.o \
	$(B)/q3static/tr_shadows.o \
	$(B)/q3static/tr_sky.o \
	$(B)/q3static/tr_surface.o \
	$(B)/q3static/tr_world.o \
	\
	$(B)/q3static/unix_main.o \
	$(B)/q3static/unix_net.o \
	$(B)/q3static/unix_shared.o \
	\
	$(B)/q3static/ahoptim.o  \
	$(B)/q3static/autohint.o \
	$(B)/q3static/ftbase.o   \
	$(B)/q3static/ftdebug.o  \
	$(B)/q3static/ftglyph.o  \
	$(B)/q3static/ftinit.o   \
	$(B)/q3static/ftmm.o     \
	$(B)/q3static/ftsystem.o \
	$(B)/q3static/raster1.o  \
	$(B)/q3static/sfnt.o     \
	$(B)/q3static/sfobjs.o   \
	$(B)/q3static/smooth.o   \
	$(B)/q3static/truetype.o \
	\
	$(B)/q3static/linux_qgl.o \
	$(B)/q3static/linux_glimp.o \
	$(B)/q3static/linux_joystick.o \
	$(B)/q3static/linux_snd.o \
	$(B)/q3static/snd_mixa.o \
	$(B)/q3static/matha.o

ifeq ($(ARCH),i386)
  Q3SOBJ += $(B)/q3static/vm_x86.o
endif

ifeq ($(ARCH),ppc)
  ifeq ($(DLL_ONLY),false)
    Q3SOBJ += $(B)/q3static/vm_ppc.o
  endif
endif


$(B)/q3static/cl_cgame.o : $(CDIR)/cl_cgame.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_cin.o : $(CDIR)/cl_cin.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_console.o : $(CDIR)/cl_console.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_input.o : $(CDIR)/cl_input.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_keys.o : $(CDIR)/cl_keys.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_main.o : $(CDIR)/cl_main.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_net_chan.o : $(CDIR)/cl_net_chan.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_parse.o : $(CDIR)/cl_parse.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/cl_scrn.o : $(CDIR)/cl_scrn.c; $(DO_CC) -DQ3_STATIC    
$(B)/q3static/cl_ui.o : $(CDIR)/cl_ui.c; $(DO_CC) -DQ3_STATIC         
$(B)/q3static/snd_adpcm.o : $(CDIR)/snd_adpcm.c; $(DO_CC) -DQ3_STATIC   
$(B)/q3static/snd_dma.o : $(CDIR)/snd_dma.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/snd_mem.o : $(CDIR)/snd_mem.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/snd_mix.o : $(CDIR)/snd_mix.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/snd_wavelet.o : $(CDIR)/snd_wavelet.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/sv_bot.o : $(SDIR)/sv_bot.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/sv_client.o : $(SDIR)/sv_client.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/sv_ccmds.o : $(SDIR)/sv_ccmds.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/sv_game.o : $(SDIR)/sv_game.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/sv_init.o : $(SDIR)/sv_init.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/sv_main.o : $(SDIR)/sv_main.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/sv_net_chan.o : $(SDIR)/sv_net_chan.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/sv_snapshot.o : $(SDIR)/sv_snapshot.c; $(DO_CC) -DQ3_STATIC      
$(B)/q3static/sv_world.o : $(SDIR)/sv_world.c; $(DO_CC) -DQ3_STATIC         
$(B)/q3static/cm_trace.o : $(CMDIR)/cm_trace.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/cm_load.o : $(CMDIR)/cm_load.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cm_test.o : $(CMDIR)/cm_test.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cm_patch.o : $(CMDIR)/cm_patch.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/cm_polylib.o : $(CMDIR)/cm_polylib.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/cmd.o : $(CMDIR)/cmd.c; $(DO_CC) -DQ3_STATIC                   
$(B)/q3static/common.o : $(CMDIR)/common.c; $(DO_CC) -DQ3_STATIC               
$(B)/q3static/cvar.o : $(CMDIR)/cvar.c; $(DO_CC) -DQ3_STATIC                
$(B)/q3static/files.o : $(CMDIR)/files.c; $(DO_CC) -DQ3_STATIC              
$(B)/q3static/md4.o : $(CMDIR)/md4.c; $(DO_CC) -DQ3_STATIC                   
$(B)/q3static/msg.o : $(CMDIR)/msg.c; $(DO_CC) -DQ3_STATIC                    
$(B)/q3static/net_chan.o : $(CMDIR)/net_chan.c; $(DO_CC) -DQ3_STATIC

$(B)/q3static/be_aas_bspq3.o : $(BLIBDIR)/be_aas_bspq3.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_cluster.o : $(BLIBDIR)/be_aas_cluster.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_debug.o : $(BLIBDIR)/be_aas_debug.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_entity.o : $(BLIBDIR)/be_aas_entity.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_file.o : $(BLIBDIR)/be_aas_file.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_main.o : $(BLIBDIR)/be_aas_main.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_move.o : $(BLIBDIR)/be_aas_move.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_optimize.o : $(BLIBDIR)/be_aas_optimize.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_reach.o : $(BLIBDIR)/be_aas_reach.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_route.o : $(BLIBDIR)/be_aas_route.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_routealt.o : $(BLIBDIR)/be_aas_routealt.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_aas_sample.o : $(BLIBDIR)/be_aas_sample.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_char.o : $(BLIBDIR)/be_ai_char.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_chat.o : $(BLIBDIR)/be_ai_chat.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_gen.o : $(BLIBDIR)/be_ai_gen.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_goal.o : $(BLIBDIR)/be_ai_goal.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_move.o : $(BLIBDIR)/be_ai_move.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_weap.o : $(BLIBDIR)/be_ai_weap.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ai_weight.o : $(BLIBDIR)/be_ai_weight.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_ea.o : $(BLIBDIR)/be_ea.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/be_interface.o : $(BLIBDIR)/be_interface.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_crc.o : $(BLIBDIR)/l_crc.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_libvar.o : $(BLIBDIR)/l_libvar.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_log.o : $(BLIBDIR)/l_log.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_memory.o : $(BLIBDIR)/l_memory.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_precomp.o : $(BLIBDIR)/l_precomp.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_script.o : $(BLIBDIR)/l_script.c; $(DO_BOT_CC) -DQ3_STATIC 
$(B)/q3static/l_struct.o : $(BLIBDIR)/l_struct.c; $(DO_BOT_CC) -DQ3_STATIC 

$(B)/q3static/jcapimin.o : $(JPDIR)/jcapimin.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jchuff.o : $(JPDIR)/jchuff.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcinit.o : $(JPDIR)/jcinit.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jccoefct.o : $(JPDIR)/jccoefct.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jccolor.o : $(JPDIR)/jccolor.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jfdctflt.o : $(JPDIR)/jfdctflt.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcdctmgr.o : $(JPDIR)/jcdctmgr.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcmainct.o : $(JPDIR)/jcmainct.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcmarker.o : $(JPDIR)/jcmarker.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcmaster.o : $(JPDIR)/jcmaster.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcomapi.o : $(JPDIR)/jcomapi.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/jcparam.o : $(JPDIR)/jcparam.c;  $(DO_CC) -DQ3_STATIC
$(B)/q3static/jcprepct.o : $(JPDIR)/jcprepct.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/jcsample.o : $(JPDIR)/jcsample.c; $(DO_CC) -DQ3_STATIC 

$(B)/q3static/jdapimin.o : $(JPDIR)/jdapimin.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdapistd.o : $(JPDIR)/jdapistd.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdatasrc.o : $(JPDIR)/jdatasrc.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdcoefct.o : $(JPDIR)/jdcoefct.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdcolor.o : $(JPDIR)/jdcolor.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jcphuff.o : $(JPDIR)/jcphuff.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jddctmgr.o : $(JPDIR)/jddctmgr.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdhuff.o : $(JPDIR)/jdhuff.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdinput.o : $(JPDIR)/jdinput.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdmainct.o : $(JPDIR)/jdmainct.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdmarker.o : $(JPDIR)/jdmarker.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdmaster.o : $(JPDIR)/jdmaster.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdpostct.o : $(JPDIR)/jdpostct.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdsample.o : $(JPDIR)/jdsample.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jdtrans.o : $(JPDIR)/jdtrans.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jerror.o : $(JPDIR)/jerror.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jidctflt.o : $(JPDIR)/jidctflt.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jmemmgr.o : $(JPDIR)/jmemmgr.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jmemnobs.o : $(JPDIR)/jmemnobs.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/jutils.o : $(JPDIR)/jutils.c; $(DO_CC) -DQ3_STATIC 

$(B)/q3static/tr_bsp.o : $(RDIR)/tr_bsp.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_animation.o : $(RDIR)/tr_animation.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_backend.o : $(RDIR)/tr_backend.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_cmds.o : $(RDIR)/tr_cmds.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_curve.o : $(RDIR)/tr_curve.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_flares.o : $(RDIR)/tr_flares.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/tr_font.o : $(RDIR)/tr_font.c; $(DO_CC) -DQ3_STATIC  
$(B)/q3static/tr_image.o : $(RDIR)/tr_image.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_init.o : $(RDIR)/tr_init.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_light.o : $(RDIR)/tr_light.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_main.o : $(RDIR)/tr_main.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_marks.o : $(RDIR)/tr_marks.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_mesh.o : $(RDIR)/tr_mesh.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_model.o : $(RDIR)/tr_model.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_noise.o : $(RDIR)/tr_noise.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_scene.o : $(RDIR)/tr_scene.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_shade.o : $(RDIR)/tr_shade.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_shader.o : $(RDIR)/tr_shader.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_shade_calc.o : $(RDIR)/tr_shade_calc.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_shadows.o : $(RDIR)/tr_shadows.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_sky.o : $(RDIR)/tr_sky.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_smp.o : $(RDIR)/tr_smp.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_stripify.o : $(RDIR)/tr_stripify.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_subdivide.o : $(RDIR)/tr_subdivide.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_surface.o : $(RDIR)/tr_surface.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/tr_world.o : $(RDIR)/tr_world.c; $(DO_CC) -DQ3_STATIC 

$(B)/q3static/unix_qgl.o : $(UDIR)/unix_qgl.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/unix_main.o : $(UDIR)/unix_main.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/unix_net.o : $(UDIR)/unix_net.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/unix_shared.o : $(UDIR)/unix_shared.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/linux_glimp.o : $(UDIR)/linux_glimp.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/linux_joystick.o : $(UDIR)/linux_joystick.c; $(DO_CC) -DQ3_STATIC  
$(B)/q3static/linux_qgl.o : $(UDIR)/linux_qgl.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/linux_input.o : $(UDIR)/linux_input.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/linux_snd.o : $(UDIR)/linux_snd.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/snd_mixa.o : $(UDIR)/snd_mixa.s; $(DO_AS) 
$(B)/q3static/matha.o : $(UDIR)/matha.s; $(DO_AS) 
$(B)/q3static/unzip.o : $(CMDIR)/unzip.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/vm.o : $(CMDIR)/vm.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/vm_interpreted.o : $(CMDIR)/vm_interpreted.c; $(DO_CC) -DQ3_STATIC 

ifeq ($(ARCH),i386)
  $(B)/q3static/vm_x86.o : $(CMDIR)/vm_x86.c; $(DO_CC) -DQ3_STATIC
endif

ifeq ($(ARCH),ppc)
ifeq ($(DLL_ONLY),false)
$(B)/q3static/vm_ppc.o : $(CMDIR)/vm_ppc.c; $(DO_CC) -DQ3_STATIC
endif
endif

$(B)/q3static/ahoptim.o  : $(FTDIR)/ahoptim.c;   $(DO_CC) -DQ3_STATIC
$(B)/q3static/autohint.o : $(FTDIR)/autohint.c;  $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftbase.o   : $(FTDIR)/ftbase.c;    $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftdebug.o  : $(FTDIR)/ftdebug.c;   $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftglyph.o  : $(FTDIR)/ftglyph.c;   $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftinit.o   : $(FTDIR)/ftinit.c;    $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftmm.o     : $(FTDIR)/ftmm.c;      $(DO_CC) -DQ3_STATIC
$(B)/q3static/ftsystem.o : $(FTDIR)/ftsystem.c;  $(DO_CC) -DQ3_STATIC
$(B)/q3static/raster1.o  : $(FTDIR)/raster1.c;   $(DO_CC) -DQ3_STATIC -DFT_FLAT_COMPILE
$(B)/q3static/sfnt.o     : $(FTDIR)/sfnt.c;      $(DO_CC) -DQ3_STATIC
$(B)/q3static/sfobjs.o   : $(FTDIR)/sfobjs.c;    $(DO_CC) -DQ3_STATIC
$(B)/q3static/smooth.o   : $(FTDIR)/smooth.c;    $(DO_CC) -DQ3_STATIC -DFT_FLAT_COMPILE
$(B)/q3static/truetype.o : $(FTDIR)/truetype.c;  $(DO_CC) -DQ3_STATIC

## add BASEQ3 CGAME
Q3SOBJ += \
	$(B)/q3static/cg_consolecmds.o \
	$(B)/q3static/cg_draw.o \
	$(B)/q3static/cg_drawtools.o \
	$(B)/q3static/cg_effects.o \
	$(B)/q3static/cg_ents.o \
	$(B)/q3static/cg_event.o \
	$(B)/q3static/cg_info.o \
	$(B)/q3static/cg_localents.o \
	$(B)/q3static/cg_main.o \
	$(B)/q3static/cg_marks.o \
	$(B)/q3static/cg_players.o \
	$(B)/q3static/cg_playerstate.o \
	$(B)/q3static/cg_predict.o \
	$(B)/q3static/cg_scoreboard.o \
	$(B)/q3static/cg_servercmds.o \
	$(B)/q3static/cg_snapshot.o \
	$(B)/q3static/cg_syscalls.o \
	$(B)/q3static/cg_view.o \
	$(B)/q3static/cg_weapons.o

$(B)/q3static/cg_consolecmds.o : $(CGDIR)/cg_consolecmds.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/cg_draw.o : $(CGDIR)/cg_draw.c; $(DO_CC) -DQ3_STATIC             
$(B)/q3static/cg_drawtools.o : $(CGDIR)/cg_drawtools.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_effects.o : $(CGDIR)/cg_effects.c; $(DO_CC) -DQ3_STATIC           
$(B)/q3static/cg_ents.o : $(CGDIR)/cg_ents.c; $(DO_CC) -DQ3_STATIC               
$(B)/q3static/cg_event.o : $(CGDIR)/cg_event.c; $(DO_CC) -DQ3_STATIC            
$(B)/q3static/cg_info.o : $(CGDIR)/cg_info.c; $(DO_CC) -DQ3_STATIC               
$(B)/q3static/cg_localents.o : $(CGDIR)/cg_localents.c; $(DO_CC) -DQ3_STATIC           
$(B)/q3static/cg_main.o : $(CGDIR)/cg_main.c; $(DO_CC) -DQ3_STATIC                   
$(B)/q3static/cg_marks.o : $(CGDIR)/cg_marks.c; $(DO_CC) -DQ3_STATIC                 
$(B)/q3static/cg_players.o : $(CGDIR)/cg_players.c; $(DO_CC) -DQ3_STATIC             
$(B)/q3static/cg_playerstate.o : $(CGDIR)/cg_playerstate.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_predict.o : $(CGDIR)/cg_predict.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_scoreboard.o : $(CGDIR)/cg_scoreboard.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_servercmds.o : $(CGDIR)/cg_servercmds.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_snapshot.o : $(CGDIR)/cg_snapshot.c; $(DO_CC) -DQ3_STATIC             
$(B)/q3static/cg_syscalls.o : $(CGDIR)/cg_syscalls.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/cg_view.o : $(CGDIR)/cg_view.c; $(DO_CC) -DQ3_STATIC                     
$(B)/q3static/cg_weapons.o : $(CGDIR)/cg_weapons.c; $(DO_CC) -DQ3_STATIC 

## add BASEQ3 GAME
Q3SOBJ += \
	$(B)/q3static/ai_chat.o \
	$(B)/q3static/ai_cmd.o \
	$(B)/q3static/ai_dmnet.o \
	$(B)/q3static/ai_dmq3.o \
	$(B)/q3static/ai_main.o \
	$(B)/q3static/ai_team.o \
	$(B)/q3static/ai_vcmd.o \
	$(B)/q3static/g_active.o \
	$(B)/q3static/g_arenas.o \
	$(B)/q3static/g_bot.o \
	$(B)/q3static/g_client.o \
	$(B)/q3static/g_cmds.o \
	$(B)/q3static/g_combat.o \
	$(B)/q3static/g_items.o \
	$(B)/q3static/g_main.o \
	$(B)/q3static/g_mem.o \
	$(B)/q3static/g_misc.o \
	$(B)/q3static/g_missile.o \
	$(B)/q3static/g_mover.o \
	$(B)/q3static/g_session.o \
	$(B)/q3static/g_spawn.o \
	$(B)/q3static/g_svcmds.o \
	$(B)/q3static/g_target.o \
	$(B)/q3static/g_team.o \
	$(B)/q3static/g_trigger.o \
	$(B)/q3static/g_utils.o \
	$(B)/q3static/g_weapon.o \
	\
	$(B)/q3static/g_syscalls.o

$(B)/q3static/ai_chat.o : $(GDIR)/ai_chat.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/ai_cmd.o : $(GDIR)/ai_cmd.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/ai_dmnet.o : $(GDIR)/ai_dmnet.c; $(DO_CC) -DQ3_STATIC   
$(B)/q3static/ai_dmq3.o : $(GDIR)/ai_dmq3.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/ai_main.o : $(GDIR)/ai_main.c; $(DO_CC) -DQ3_STATIC      
$(B)/q3static/ai_team.o : $(GDIR)/ai_team.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/ai_vcmd.o : $(GDIR)/ai_vcmd.c; $(DO_CC) -DQ3_STATIC     
$(B)/q3static/g_active.o : $(GDIR)/g_active.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/g_arenas.o : $(GDIR)/g_arenas.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/g_bot.o : $(GDIR)/g_bot.c; $(DO_CC) -DQ3_STATIC            
$(B)/q3static/g_client.o : $(GDIR)/g_client.c; $(DO_CC) -DQ3_STATIC      
$(B)/q3static/g_cmds.o : $(GDIR)/g_cmds.c; $(DO_CC) -DQ3_STATIC          
$(B)/q3static/g_combat.o : $(GDIR)/g_combat.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/g_items.o : $(GDIR)/g_items.c; $(DO_CC) -DQ3_STATIC          
$(B)/q3static/g_main.o : $(GDIR)/g_main.c; $(DO_CC) -DQ3_STATIC            
$(B)/q3static/g_mem.o : $(GDIR)/g_mem.c; $(DO_CC) -DQ3_STATIC               
$(B)/q3static/g_misc.o : $(GDIR)/g_misc.c; $(DO_CC) -DQ3_STATIC             
$(B)/q3static/g_missile.o : $(GDIR)/g_missile.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/g_mover.o : $(GDIR)/g_mover.c; $(DO_CC) -DQ3_STATIC          
$(B)/q3static/g_session.o : $(GDIR)/g_session.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/g_spawn.o : $(GDIR)/g_spawn.c; $(DO_CC) -DQ3_STATIC          
$(B)/q3static/g_svcmds.o : $(GDIR)/g_svcmds.c; $(DO_CC) -DQ3_STATIC         
$(B)/q3static/g_syscalls.o : $(GDIR)/g_syscalls.c; $(DO_CC) -DQ3_STATIC    
$(B)/q3static/g_target.o : $(GDIR)/g_target.c; $(DO_CC) -DQ3_STATIC        
$(B)/q3static/g_team.o : $(GDIR)/g_team.c; $(DO_CC) -DQ3_STATIC           
$(B)/q3static/g_trigger.o : $(GDIR)/g_trigger.c; $(DO_CC) -DQ3_STATIC       
$(B)/q3static/g_utils.o : $(GDIR)/g_utils.c; $(DO_CC) -DQ3_STATIC          
$(B)/q3static/g_weapon.o : $(GDIR)/g_weapon.c; $(DO_CC) -DQ3_STATIC             

## add BASEQ3 UI
Q3SOBJ += \
        $(B)/q3static/ui_addbots.o \
        $(B)/q3static/ui_atoms.o \
        $(B)/q3static/ui_cdkey.o \
        $(B)/q3static/ui_cinematics.o \
        $(B)/q3static/ui_confirm.o \
        $(B)/q3static/ui_connect.o \
        $(B)/q3static/ui_controls2.o \
        $(B)/q3static/ui_credits.o \
        $(B)/q3static/ui_demo2.o \
        $(B)/q3static/ui_display.o \
        $(B)/q3static/ui_gameinfo.o \
        $(B)/q3static/ui_ingame.o \
        $(B)/q3static/ui_loadconfig.o \
        $(B)/q3static/ui_main.o \
        $(B)/q3static/ui_menu.o \
        $(B)/q3static/ui_mfield.o \
        $(B)/q3static/ui_mods.o \
        $(B)/q3static/ui_network.o \
        $(B)/q3static/ui_options.o \
        $(B)/q3static/ui_playermodel.o \
        $(B)/q3static/ui_players.o \
        $(B)/q3static/ui_playersettings.o \
        $(B)/q3static/ui_preferences.o \
        $(B)/q3static/ui_qmenu.o \
        $(B)/q3static/ui_removebots.o \
        $(B)/q3static/ui_saveconfig.o \
        $(B)/q3static/ui_serverinfo.o \
        $(B)/q3static/ui_servers2.o \
        $(B)/q3static/ui_setup.o \
        $(B)/q3static/ui_sound.o \
        $(B)/q3static/ui_sparena.o \
        $(B)/q3static/ui_specifyserver.o \
        $(B)/q3static/ui_splevel.o \
        $(B)/q3static/ui_sppostgame.o \
        $(B)/q3static/ui_spskill.o \
        $(B)/q3static/ui_startserver.o \
        $(B)/q3static/ui_team.o \
        $(B)/q3static/ui_teamorders.o \
        $(B)/q3static/ui_video.o \
	\
	$(B)/q3static/ui_syscalls.o

$(B)/q3static/ui_addbots.o : $(Q3UIDIR)/ui_addbots.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_atoms.o : $(Q3UIDIR)/ui_atoms.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_cinematics.o : $(Q3UIDIR)/ui_cinematics.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_cdkey.o : $(Q3UIDIR)/ui_cdkey.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/ui_confirm.o : $(Q3UIDIR)/ui_confirm.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_connect.o : $(Q3UIDIR)/ui_connect.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_controls2.o : $(Q3UIDIR)/ui_controls2.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_credits.o : $(Q3UIDIR)/ui_credits.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_demo2.o : $(Q3UIDIR)/ui_demo2.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_display.o : $(Q3UIDIR)/ui_display.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_gameinfo.o : $(Q3UIDIR)/ui_gameinfo.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_ingame.o : $(Q3UIDIR)/ui_ingame.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_loadconfig.o : $(Q3UIDIR)/ui_loadconfig.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_main.o : $(Q3UIDIR)/ui_main.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_menu.o : $(Q3UIDIR)/ui_menu.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_mfield.o : $(Q3UIDIR)/ui_mfield.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_mods.o : $(Q3UIDIR)/ui_mods.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_network.o : $(Q3UIDIR)/ui_network.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_options.o : $(Q3UIDIR)/ui_options.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/ui_playermodel.o : $(Q3UIDIR)/ui_playermodel.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_players.o : $(Q3UIDIR)/ui_players.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_playersettings.o : $(Q3UIDIR)/ui_playersettings.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_preferences.o : $(Q3UIDIR)/ui_preferences.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_qmenu.o : $(Q3UIDIR)/ui_qmenu.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_quit.o : $(Q3UIDIR)/ui_quit.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_removebots.o : $(Q3UIDIR)/ui_removebots.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_saveconfig.o : $(Q3UIDIR)/ui_saveconfig.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_serverinfo.o : $(Q3UIDIR)/ui_serverinfo.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_servers2.o : $(Q3UIDIR)/ui_servers2.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_setup.o : $(Q3UIDIR)/ui_setup.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_sound.o : $(Q3UIDIR)/ui_sound.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_sparena.o : $(Q3UIDIR)/ui_sparena.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_specifyserver.o : $(Q3UIDIR)/ui_specifyserver.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_splevel.o : $(Q3UIDIR)/ui_splevel.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_sppostgame.o : $(Q3UIDIR)/ui_sppostgame.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_spskill.o : $(Q3UIDIR)/ui_spskill.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_startserver.o : $(Q3UIDIR)/ui_startserver.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_team.o : $(Q3UIDIR)/ui_team.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_teamorders.o : $(Q3UIDIR)/ui_teamorders.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_syscalls.o : $(Q3UIDIR)/ui_syscalls.c; $(DO_CC) -DQ3_STATIC 
$(B)/q3static/ui_video.o : $(Q3UIDIR)/ui_video.c; $(DO_CC) -DQ3_STATIC 


## add shared files
Q3SOBJ += \
	$(B)/q3static/bg_misc.o \
	$(B)/q3static/bg_pmove.o \
	$(B)/q3static/bg_slidemove.o \
        $(B)/q3static/q_math.o \
        $(B)/q3static/q_shared.o

## shared files
$(B)/q3static/q_math.o : $(GDIR)/q_math.c; $(DO_CC) -DQ3_STATIC               
$(B)/q3static/q_shared.o : $(GDIR)/q_shared.c; $(DO_CC) -DQ3_STATIC      
$(B)/q3static/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_CC) -DQ3_STATIC
$(B)/q3static/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_CC) -DQ3_STATIC  
$(B)/q3static/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_CC) -DQ3_STATIC    



$(B)/$(PLATFORM)q3static : $(Q3SOBJ) 
	$(CC) $(CFLAGS) -o $@ $(Q3SOBJ)  $(GLLDFLAGS) $(LDFLAGS) 


#############################################################################
# RPM
#############################################################################

TMPDIR=/var/tmp
TARDIR=$(TMPDIR)/$(BUILD_NAME)
TARFILE = $(BUILD_NAME)-$(VERSION)-$(RPM_RELEASE).$(ARCH).tar

tar:
	if [ ! -d archives ];then mkdir archives;chmod 755 archives;fi
	$(MAKE) copyfiles COPYDIR=$(TARDIR)
	cd $(TARDIR); tar cvf $(TARFILE) * && gzip -9 $(TARFILE)
	mv $(TARDIR)/$(TARFILE).gz archives/.
	rm -rf $(TARDIR)

# Make RPMs.  You need to be root to make this work
RPMROOT=/usr/src/redhat
RPM = rpm
RPMFLAGS = -bb
INSTALLDIR = /usr/local/games/$(BUILD_NAME)
RPMDIR = $(TMPDIR)/$(BUILD_NAME)-$(VERSION)
DESTDIR= $(RPMDIR)/$(INSTALLDIR)

rpm: $(BUILD_NAME).spec
	touch $(RPMROOT)/SOURCES/$(BUILD_NAME)-$(VERSION).tar.gz
	if [ ! -d archives ];then mkdir archives;fi
	$(MAKE) copyfiles COPYDIR=$(DESTDIR)
	cp $(UDIR)/quake3.gif $(RPMROOT)/SOURCES/.
	cp $(BUILD_NAME).spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) $(BUILD_NAME).spec
	rm -rf $(RPMDIR)
	mv $(RPMROOT)/RPMS/$(RPMARCH)/$(BUILD_NAME)-$(VERSION)-$(RPM_RELEASE).$(RPMARCH).rpm archives/$(BUILD_NAME)-$(VERSION)-$(RPM_RELEASE).$(RPMARCH).rpm 

copyfiles:
	-mkdirhier $(COPYDIR)
	cp $(BR)/linuxquake3 $(COPYDIR)/quake3.x86
	strip $(COPYDIR)/quake3.x86
	chmod 755 $(COPYDIR)/quake3.x86
	cp $(BR)/linuxq3ded $(COPYDIR)/q3ded
	strip $(COPYDIR)/q3ded
	chmod 755 $(COPYDIR)/q3ded
	cp $(BDIR)/libMesaVoodooGL.so.3.2 $(COPYDIR)/.
	chmod 755 $(COPYDIR)/libMesaVoodooGL.so.3.2
	( cd $(COPYDIR); ln -s libMesaVoodooGL.so.3.2 libMesaVoodooGL.so )
	cp $(BDIR)/Quake_III_Arena_FAQ.html $(COPYDIR)/.
	chmod 644 $(COPYDIR)/Quake_III_Arena_FAQ.html
	mkdir $(COPYDIR)/baseq3
	cp $(BASEQ3_DIR)/pak2.pk3 $(COPYDIR)/baseq3/.
	chmod 644 $(COPYDIR)/baseq3/pak2.pk3

$(BUILD_NAME).spec : $(UDIR)/$(BUILD_NAME).spec.sh Makefile
	sh $< $(VERSION) $(RPM_RELEASE) $(ARCH) $(INSTALLDIR) > $@

#############################################################################
# MISC
#############################################################################

# TTimo: FIXME: doesn't clean the binary and .so

clean:clean-debug clean-release

clean2: clean-bins
	rm -f $(Q3OBJ) $(Q3POBJ) $(Q3POBJ_SMP) $(Q3DOBJ) $(MPGOBJ) $(Q3GOBJ) $(Q3CGOBJ) $(MPCGOBJ) $(Q3UIOBJ) $(MPUIOBJ)
	rm -f $(CGDIR)/vm/*.asm
	rm -f $(GDIR)/vm/*.asm
	rm -f $(UIDIR)/vm/*.asm
	rm -f $(Q3UIDIR)/vm/*.asm

# TTimo: linuxq3ded linuxquake3 linuxquake3-smp .. hardcoded the names .. maybe not such a good thing
# FIXME: also, removing the *.so is crappy .. I just want to avoid rm -rf debugi386-glibc to save the symlinks to pk3's for testing
clean-bins:
	if [ -d $(B) ];then (find $(B) -name '*.so' -exec rm {} \;)fi
	rm -f $(B)/linuxq3ded
	rm -f $(B)/linuxquake3
	rm -f $(B)/linuxquake3-smp
	rm -f $(B)/baseq3/vm/cgame.qvm
	rm -f $(B)/baseq3/vm/ui.qvm
	rm -f $(B)/baseq3/vm/qagame.qvm
	rm -f $(B)/missionpack/vm/cgame.qvm
	rm -f $(B)/missionpack/vm/qagame.qvm
	rm -f $(B)/missionpack/vm/ui.qvm

clean-debug:
	$(MAKE) clean2 B=$(BD) CFLAGS="$(DEBUG_CFLAGS)"

clean-release:
	$(MAKE) clean2 B=$(BR) CFLAGS="$(DEBUG_CFLAGS)"

#
# TTimo: the make tar and make rpm don't really fit my needs
# writing custom hack

DATADIR = /home/timo/Id/Q3SetupMedia/quake3

setup: release $(B)/linuxq3a-$(VERSION).tar.gz

$(B)/linuxq3a-$(VERSION).tar.gz:
	rm -rf setup
	mkdir setup
	cp -R $(DATADIR) setup
	cp release$(ARCH)-glibc/linuxquake3 setup/quake3/quake3.x86
	strip setup/quake3/quake3.x86
	cp release$(ARCH)-glibc/linuxq3ded setup/quake3/q3ded
	strip setup/quake3/q3ded
	cd setup; tar cvzf linuxq3a-$(VERSION).tar.gz quake3
# TTimo: the .so where distributed with 1.27g because of a VM problem that made them break on linux
#	cp release$(ARCH)-glibc/baseq3/cgame$(ARCH).so setup/quake3/baseq3
#	cp release$(ARCH)-glibc/baseq3/qagame$(ARCH).so setup/quake3/baseq3
#	cp release$(ARCH)-glibc/baseq3/ui$(ARCH).so setup/quake3/baseq3
#	cp release$(ARCH)-glibc/missionpack/cgame$(ARCH).so setup/quake3/missionpack
#	cp release$(ARCH)-glibc/missionpack/qagame$(ARCH).so setup/quake3/missionpack
#	cp release$(ARCH)-glibc/missionpack/ui$(ARCH).so setup/quake3/missionpack

# don't redistribute this one!	
setup-debug: debug $(B)/linuxq3a-$(VERSION)-DEBUG.tar.gz

$(B)/linuxq3a-$(VERSION)-DEBUG.tar.gz:
	rm -rf setup-debug
	mkdir setup-debug
	cp -R $(DATADIR) setup-debug
	cp debug$(ARCH)-glibc/linuxquake3 setup-debug/quake3/quake3.x86
	cp debug$(ARCH)-glibc/linuxq3ded setup-debug/quake3/q3ded
	cp debug$(ARCH)-glibc/baseq3/cgame$(ARCH).so setup-debug/quake3/baseq3
	cp debug$(ARCH)-glibc/baseq3/qagame$(ARCH).so setup-debug/quake3/baseq3
	cp debug$(ARCH)-glibc/baseq3/ui$(ARCH).so setup-debug/quake3/baseq3
	cp debug$(ARCH)-glibc/missionpack/cgame$(ARCH).so setup-debug/quake3/missionpack
	cp debug$(ARCH)-glibc/missionpack/qagame$(ARCH).so setup-debug/quake3/missionpack
	cp debug$(ARCH)-glibc/missionpack/ui$(ARCH).so setup-debug/quake3/missionpack
	cd setup-debug; tar cvzf linuxq3a-$(VERSION)-DEBUG.tar.gz quake3

## temps
# NOTE: not referenced

dcp:
	cp debug$(ARCH)-glibc/linuxquake3 $(TESTDIR)/quake3-$(VERSION)-debug.$(ARCH)
	cp debug$(ARCH)-glibc/linuxq3ded $(TESTDIR)/q3ded-$(VERSION)-debug.$(ARCH)
	cp debug$(ARCH)-glibc/baseq3/cgame$(ARCH).so $(TESTDIR)/baseq3/cgame$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/baseq3/qagame$(ARCH).so $(TESTDIR)/baseq3/qagame$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/baseq3/ui$(ARCH).so $(TESTDIR)/baseq3/ui$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/missionpack/cgame$(ARCH).so $(TESTDIR)/missionpack/cgame$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/missionpack/qagame$(ARCH).so $(TESTDIR)/missionpack/qagame$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/missionpack/ui$(ARCH).so $(TESTDIR)/missionpack/ui$(ARCH)-debug.so
	cp debug$(ARCH)-glibc/baseq3/vm/*.qvm $(TESTDIR)/baseq3/vm/
	cp debug$(ARCH)-glibc/missionpack/vm/*.qvm $(TESTDIR)/missionpack/vm/

rcp:
	cp release$(ARCH)-glibc/linuxquake3 $(TESTDIR)/quake3-$(VERSION).$(ARCH)
	cp release$(ARCH)-glibc/linuxq3ded $(TESTDIR)/q3ded-$(VERSION).$(ARCH)
	cp release$(ARCH)-glibc/baseq3/cgame$(ARCH).so $(TESTDIR)/baseq3/cgame$(ARCH).so
	cp release$(ARCH)-glibc/baseq3/qagame$(ARCH).so $(TESTDIR)/baseq3/qagame$(ARCH).so
	cp release$(ARCH)-glibc/baseq3/ui$(ARCH).so $(TESTDIR)/baseq3/ui$(ARCH).so
	cp release$(ARCH)-glibc/missionpack/cgame$(ARCH).so $(TESTDIR)/missionpack/cgame$(ARCH).so
	cp release$(ARCH)-glibc/missionpack/qagame$(ARCH).so $(TESTDIR)/missionpack/qagame$(ARCH).so
	cp release$(ARCH)-glibc/missionpack/ui$(ARCH).so $(TESTDIR)/missionpack/ui$(ARCH).so
	cp release$(ARCH)-glibc/baseq3/vm/*.qvm $(TESTDIR)/baseq3/vm/
	cp release$(ARCH)-glibc/missionpack/vm/*.qvm $(TESTDIR)/missionpack/vm/
