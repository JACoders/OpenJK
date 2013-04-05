#
# Quake3 Unix Makefile
#
# Currently build for the following:
#	Linux i386 (full client)
#	Linux Alpha (dedicated server only)
#	SGI IRIX (full client)
#
# Nov '98 by Zoid <zoid@idsoftware.com>
#
# GNU Make required
#

PLATFORM=$(shell uname|tr '[:upper:]' '[:lower:]')
PLATFORM_RELEASE=$(shell uname -r)

###
### These paths are where you probably want to change things
###

# Where we are building to, libMesaVoodooGL.so.3.1 should be here, etc.
# the demo pk3 file should be here in demoq3/pak0.pk3 or baseq3/pak0.pk3
BUILD_DIR=/home/zoid/Quake3

# Where we are building from (where the source code should be!)
MOUNT_DIR=/devel/Quake3/q3code

#############################################################################
##
## You shouldn't have to touch anything below here
##
#############################################################################

DEMO_PAK=$(BUILD_DIR)/demoq3/pak0.pk3

BUILD_DEBUG_DIR=debug$(ARCH)$(GLIBC)
BUILD_RELEASE_DIR=release$(ARCH)$(GLIBC)
CLIENT_DIR=$(MOUNT_DIR)/client
SERVER_DIR=$(MOUNT_DIR)/server
REF_DIR=$(MOUNT_DIR)/renderer
COMMON_DIR=$(MOUNT_DIR)/qcommon
UNIX_DIR=$(MOUNT_DIR)/unix
GAME_DIR=$(MOUNT_DIR)/game
CGAME_DIR=$(MOUNT_DIR)/cgame
NULL_DIR=$(MOUNT_DIR)/null

#used for linux i386 builds
MESA_DIR=/usr/local/src/Mesa-3.0

VERSION=1.05

VERSION_FN=$(VERSION)$(GLIBC)
RPM_RELEASE=9

#############################################################################
# SETUP AND BUILD
#############################################################################

ifeq ($(PLATFORM),irix)

ARCH=mips  #default to MIPS
VENDOR=sgi
GLIBC=  #libc is irrelevant

CC=cc
BASE_CFLAGS=-Dstricmp=strcasecmp -Xcpluscomm -woff 1185 -mips3 \
	-nostdinc -I. -I$(ROOT)/usr/include
RELEASE_CFLAGS=$(BASE_CFLAGS) -O3
DEBUG_CFLAGS=$(BASE_CFLAGS) -g

SHLIBEXT=so
SHLIBCFLAGS=
SHLIBLDFLAGS=-shared

LDFLAGS=-ldl -lm
GLLDFLAGS=-L/usr/X11/lib -lGL -lX11 -lXext -lm

TARGETS=$(BUILDDIR)/sgiquake3 \
	$(BUILDDIR)/qagame$(ARCH).$(SHLIBEXT) \
	$(BUILDDIR)/cgame$(ARCH).$(SHLIBEXT)

else
ifeq ($(PLATFORM),linux)

ifneq (,$(findstring libc6,$(shell if [ -e /lib/libc.so.6* ];then echo libc6;fi)))
GLIBC=-glibc
else
GLIBC=
endif #libc6 test


ifneq (,$(findstring alpha,$(shell uname -m)))
ARCH=axp
RPMARCH=alpha
VENDOR=dec
else #default to i386
ARCH=i386
RPMARCH=i386
VENDOR=unknown
endif #alpha test

BASE_CFLAGS=-Dstricmp=strcasecmp -I$(MESA_DIR)/include -I/usr/include/glide \
	-DREF_HARD_LINKED -pipe

DEBUG_CFLAGS=$(BASE_CFLAGS) -g
ifeq ($(ARCH),axp)
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations
else
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O3 -m486 -fomit-frame-pointer -pipe -ffast-math -fexpensive-optimizations -malign-loops=2 -malign-jumps=2 -malign-functions=2
endif

CC=gcc

SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

LDFLAGS=-ldl -lm
GLLDFLAGS=-L/usr/X11R6/lib -L$(MESA_DIR)/lib -lX11 -lXext -lXxf86dga -lXxf86vm
GL_SVGA_LDFLAGS=-L/usr/X11R6/lib -L$(MESA_DIR)/lib -lX11 -lXext -lvga

ifeq ($(ARCH),axp)
TARGETS=$(BUILDDIR)/linuxq3ded $(BUILDDIR)/qagame$(ARCH).$(SHLIBEXT)
else
# $(BUILDDIR)/linuxquake3.vga
TARGETS=\
	$(BUILDDIR)/linuxquake3 \
	$(BUILDDIR)/qagame$(ARCH).$(SHLIBEXT) \
	$(BUILDDIR)/cgame$(ARCH).$(SHLIBEXT)
#$(BUILDDIR)/quake3
endif


else #generic

CC=cc
BASE_CFLAGS=-Dstricmp=strcasecmp
DEBUG_CFLAGS=$(BASE_CFLAGS) -g
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O

SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

LDFLAGS=-ldl -lm

TARGETS=$(BUILDDIR)/q3ded $(BUILDDIR)/qagame$(ARCH).$(SHLIBEXT)

endif #Linux
endif #IRIX

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_SHLIB_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_AS=$(CC) $(CFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<

#### DEFAULT TARGET
# default: build_release

build_debug:
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)"

build_release:
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS)"

#Build both debug and release builds
all: build_debug build_release

targets: makedirs $(TARGETS)

makedirs:
	@if [ ! -d $(BUILDDIR) ];then mkdir $(BUILDDIR);fi
	@if [ ! -d $(BUILDDIR)/client ];then mkdir $(BUILDDIR)/client;fi
	@if [ ! -d $(BUILDDIR)/ded ];then mkdir $(BUILDDIR)/ded;fi
	@if [ ! -d $(BUILDDIR)/ref ];then mkdir $(BUILDDIR)/ref;fi
	@if [ ! -d $(BUILDDIR)/game ];then mkdir $(BUILDDIR)/game;fi
	@if [ ! -d $(BUILDDIR)/cgame ];then mkdir $(BUILDDIR)/cgame;fi

#############################################################################
# CLIENT/SERVER
#############################################################################

QUAKE3_OBJS = \
	$(BUILDDIR)/client/cl_cgame.o \
	$(BUILDDIR)/client/cl_cin.o \
	$(BUILDDIR)/client/cl_console.o \
	$(BUILDDIR)/client/cl_input.o \
	$(BUILDDIR)/client/cl_keys.o \
	$(BUILDDIR)/client/cl_main.o \
	$(BUILDDIR)/client/cl_parse.o \
	$(BUILDDIR)/client/cl_scrn.o \
	$(BUILDDIR)/client/snd_dma.o \
	$(BUILDDIR)/client/snd_mem.o \
	$(BUILDDIR)/client/snd_mix.o \
	$(BUILDDIR)/client/sv_bot.o \
	$(BUILDDIR)/client/sv_client.o \
	$(BUILDDIR)/client/sv_ccmds.o \
	$(BUILDDIR)/client/sv_game.o \
	$(BUILDDIR)/client/sv_init.o \
	$(BUILDDIR)/client/sv_main.o \
	$(BUILDDIR)/client/sv_snapshot.o \
	$(BUILDDIR)/client/sv_world.o \
	$(BUILDDIR)/client/ui_arena.o \
	$(BUILDDIR)/client/ui_connect.o \
	$(BUILDDIR)/client/ui_controls.o \
	$(BUILDDIR)/client/ui_demo.o \
	$(BUILDDIR)/client/ui_maps.o \
	$(BUILDDIR)/client/ui_menu.o \
	$(BUILDDIR)/client/ui_network.o \
	$(BUILDDIR)/client/ui_preferences.o \
	$(BUILDDIR)/client/ui_qmenu.o \
	$(BUILDDIR)/client/ui_servers.o \
	$(BUILDDIR)/client/ui_startserver.o \
	$(BUILDDIR)/client/ui_video.o \
	\
	$(BUILDDIR)/client/cm_load.o \
	$(BUILDDIR)/client/cm_patch.o \
	$(BUILDDIR)/client/cm_polylib.o \
	$(BUILDDIR)/client/cm_tag.o \
	$(BUILDDIR)/client/cm_test.o \
	$(BUILDDIR)/client/cm_trace.o \
	$(BUILDDIR)/client/cmd.o \
	$(BUILDDIR)/client/common.o \
	$(BUILDDIR)/client/cvar.o \
	$(BUILDDIR)/client/files.o \
	$(BUILDDIR)/client/gameinfo.o \
	$(BUILDDIR)/client/md4.o \
	$(BUILDDIR)/client/msg.o \
	$(BUILDDIR)/client/net_chan.o \
	$(BUILDDIR)/client/vm.o \
	\
	$(BUILDDIR)/client/q_shared.o \
	$(BUILDDIR)/client/q_math.o \
	\
	$(BUILDDIR)/client/tr_backend.o \
	$(BUILDDIR)/client/tr_bsp.o \
	$(BUILDDIR)/client/tr_calc.o \
	$(BUILDDIR)/client/tr_calc_3dnow.o \
	$(BUILDDIR)/client/tr_calc_c.o \
	$(BUILDDIR)/client/tr_calc_kni.o \
	$(BUILDDIR)/client/tr_cmds.o \
	$(BUILDDIR)/client/tr_curve.o \
	$(BUILDDIR)/client/tr_draw.o \
	$(BUILDDIR)/client/tr_flares.o \
	$(BUILDDIR)/client/tr_image.o \
	$(BUILDDIR)/client/tr_init.o \
	$(BUILDDIR)/client/tr_light.o \
	$(BUILDDIR)/client/tr_main.o \
	$(BUILDDIR)/client/tr_mesh.o \
	$(BUILDDIR)/client/tr_misc.o \
	$(BUILDDIR)/client/tr_model.o \
	$(BUILDDIR)/client/tr_noise.o \
	$(BUILDDIR)/client/tr_scene.o \
	$(BUILDDIR)/client/tr_shade.o \
	$(BUILDDIR)/client/tr_shader.o \
	$(BUILDDIR)/client/tr_shade_calc.o \
	$(BUILDDIR)/client/tr_shadows.o \
	$(BUILDDIR)/client/tr_smp.o \
	$(BUILDDIR)/client/tr_sky.o \
	$(BUILDDIR)/client/tr_surf.o \
	$(BUILDDIR)/client/tr_world.o \
	\
	$(BUILDDIR)/client/unix_main.o \
	$(BUILDDIR)/client/unix_net.o \
	$(BUILDDIR)/client/unix_shared.o

#platform specific objects
ifeq ($(PLATFORM),irix)
	QUAKE3_PLATOBJ=\
		$(BUILDDIR)/client/irix_qgl.o \
		$(BUILDDIR)/client/irix_glimp.o \
		$(BUILDDIR)/client/irix_snd.o
else
ifeq ($(PLATFORM),linux)
ifeq ($(ARCH),axp)
	QUAKE3_PLATOBJ=
else
	QUAKE3_PLATOBJ=\
		$(BUILDDIR)/client/linux_qgl.o \
		$(BUILDDIR)/client/linux_glimp.o \
		$(BUILDDIR)/client/linux_snd.o \
		$(BUILDDIR)/client/snd_mixa.o \
		$(BUILDDIR)/client/matha.o \
		$(BUILDDIR)/client/sys_dosa.o
	QUAKE3_VGA=\
		$(BUILDDIR)/client/linux_qgl.o \
		$(BUILDDIR)/client/linux_glimp_vga.o \
		$(BUILDDIR)/client/linux_input_vga.o \
		$(BUILDDIR)/client/linux_snd.o \
		$(BUILDDIR)/client/snd_mixa.o \
		$(BUILDDIR)/client/matha.o \
		$(BUILDDIR)/client/sys_dosa.o

endif
endif #Linux
endif #IRIX


$(BUILDDIR)/linuxquake3 : $(QUAKE3_OBJS) $(QUAKE3_PLATOBJ)
	$(CC) $(CFLAGS) -o $@ $(QUAKE3_OBJS) $(QUAKE3_PLATOBJ) $(GLLDFLAGS) $(LDFLAGS) 

ifeq ($(PLATFORM),linux)
ifeq ($(ARCH),i386)
$(BUILDDIR)/linuxquake3.vga : $(QUAKE3_OBJS) $(QUAKE3_VGA)
	$(CC) $(CFLAGS) -o $@ $(QUAKE3_OBJS) $(QUAKE3_VGA) $(GL_SVGA_LDFLAGS) $(LDFLAGS) 
endif
endif

$(BUILDDIR)/client/cl_cgame.o :			$(CLIENT_DIR)/cl_cgame.c
	$(DO_CC)

$(BUILDDIR)/client/cl_cin.o :			$(CLIENT_DIR)/cl_cin.c
	$(DO_CC)

$(BUILDDIR)/client/cl_console.o :       $(CLIENT_DIR)/cl_console.c
	$(DO_CC)

$(BUILDDIR)/client/cl_input.o :         $(CLIENT_DIR)/cl_input.c
	$(DO_CC)

$(BUILDDIR)/client/cl_keys.o :          $(CLIENT_DIR)/cl_keys.c
	$(DO_CC)

$(BUILDDIR)/client/cl_main.o :          $(CLIENT_DIR)/cl_main.c
	$(DO_CC)

$(BUILDDIR)/client/cl_parse.o :         $(CLIENT_DIR)/cl_parse.c
	$(DO_CC)

$(BUILDDIR)/client/cl_scrn.o :          $(CLIENT_DIR)/cl_scrn.c
	$(DO_CC)

$(BUILDDIR)/client/snd_dma.o :          $(CLIENT_DIR)/snd_dma.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mem.o :          $(CLIENT_DIR)/snd_mem.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mix.o :          $(CLIENT_DIR)/snd_mix.c
	$(DO_CC)

$(BUILDDIR)/client/sv_bot.o :           $(SERVER_DIR)/sv_bot.c
	$(DO_CC)

$(BUILDDIR)/client/sv_client.o :           $(SERVER_DIR)/sv_client.c
	$(DO_CC)

$(BUILDDIR)/client/sv_ccmds.o :         $(SERVER_DIR)/sv_ccmds.c
	$(DO_CC)

$(BUILDDIR)/client/sv_game.o :          $(SERVER_DIR)/sv_game.c
	$(DO_CC)

$(BUILDDIR)/client/sv_init.o :          $(SERVER_DIR)/sv_init.c
	$(DO_CC)

$(BUILDDIR)/client/sv_main.o :          $(SERVER_DIR)/sv_main.c
	$(DO_CC)

$(BUILDDIR)/client/sv_snapshot.o :      $(SERVER_DIR)/sv_snapshot.c
	$(DO_CC)

$(BUILDDIR)/client/sv_world.o :         $(SERVER_DIR)/sv_world.c
	$(DO_CC)

$(BUILDDIR)/client/ui_arena.o :         $(CLIENT_DIR)/ui_arena.c
	$(DO_CC)

$(BUILDDIR)/client/ui_connect.o :       $(CLIENT_DIR)/ui_connect.c
	$(DO_CC)

$(BUILDDIR)/client/ui_controls.o :       $(CLIENT_DIR)/ui_controls.c
	$(DO_CC)

$(BUILDDIR)/client/ui_demo.o :          $(CLIENT_DIR)/ui_demo.c
	$(DO_CC)

$(BUILDDIR)/client/ui_maps.o :          $(CLIENT_DIR)/ui_maps.c
	$(DO_CC)

$(BUILDDIR)/client/ui_menu.o :          $(CLIENT_DIR)/ui_menu.c
	$(DO_CC)

$(BUILDDIR)/client/ui_network.o :       $(CLIENT_DIR)/ui_network.c
	$(DO_CC)

$(BUILDDIR)/client/ui_preferences.o :   $(CLIENT_DIR)/ui_preferences.c
	$(DO_CC)

$(BUILDDIR)/client/ui_qmenu.o :         $(CLIENT_DIR)/ui_qmenu.c
	$(DO_CC)

$(BUILDDIR)/client/ui_servers.o :       $(CLIENT_DIR)/ui_servers.c
	$(DO_CC)

$(BUILDDIR)/client/ui_startserver.o :       $(CLIENT_DIR)/ui_startserver.c
	$(DO_CC)

$(BUILDDIR)/client/ui_video.o :         $(UNIX_DIR)/ui_video.c
	$(DO_CC)

$(BUILDDIR)/client/cm_trace.o :         $(COMMON_DIR)/cm_trace.c
	$(DO_CC)

$(BUILDDIR)/client/cm_load.o :         $(COMMON_DIR)/cm_load.c
	$(DO_CC)

$(BUILDDIR)/client/cm_test.o :         $(COMMON_DIR)/cm_test.c
	$(DO_CC)

$(BUILDDIR)/client/cm_patch.o :         $(COMMON_DIR)/cm_patch.c
	$(DO_CC)

$(BUILDDIR)/client/cm_polylib.o :         $(COMMON_DIR)/cm_polylib.c
	$(DO_CC)

$(BUILDDIR)/client/cm_tag.o :         $(COMMON_DIR)/cm_tag.c
	$(DO_CC)

$(BUILDDIR)/client/cmd.o :              $(COMMON_DIR)/cmd.c
	$(DO_CC)

$(BUILDDIR)/client/common.o :           $(COMMON_DIR)/common.c
	$(DO_CC)

$(BUILDDIR)/client/cvar.o :             $(COMMON_DIR)/cvar.c
	$(DO_CC)

$(BUILDDIR)/client/files.o :            $(COMMON_DIR)/files.c
	$(DO_CC)

$(BUILDDIR)/client/gameinfo.o :         $(COMMON_DIR)/gameinfo.c
	$(DO_CC)

$(BUILDDIR)/client/md4.o :              $(COMMON_DIR)/md4.c
	$(DO_CC)

$(BUILDDIR)/client/msg.o :              $(COMMON_DIR)/msg.c
	$(DO_CC)

$(BUILDDIR)/client/net_chan.o :         $(COMMON_DIR)/net_chan.c
	$(DO_CC)

$(BUILDDIR)/client/vm.o :         $(COMMON_DIR)/vm.c
	$(DO_CC)

$(BUILDDIR)/client/q_shared.o :         $(GAME_DIR)/q_shared.c
	$(DO_DEBUG_CC)

$(BUILDDIR)/client/q_math.o :         $(GAME_DIR)/q_math.c
	$(DO_CC)

$(BUILDDIR)/client/tr_bsp.o :           $(REF_DIR)/tr_bsp.c
	$(DO_CC)

$(BUILDDIR)/client/tr_backend.o :       $(REF_DIR)/tr_backend.c
	$(DO_CC)

$(BUILDDIR)/client/tr_calc.o :          $(REF_DIR)/tr_calc.c
	$(DO_CC)

$(BUILDDIR)/client/tr_calc_3dnow.o :    $(REF_DIR)/tr_calc_3dnow.c
	$(DO_CC)

$(BUILDDIR)/client/tr_calc_c.o :        $(REF_DIR)/tr_calc_c.c
	$(DO_CC)

$(BUILDDIR)/client/tr_calc_kni.o :      $(REF_DIR)/tr_calc_kni.c
	$(DO_CC)

$(BUILDDIR)/client/tr_cmds.o :      $(REF_DIR)/tr_cmds.c
	$(DO_CC)

$(BUILDDIR)/client/tr_curve.o :      $(REF_DIR)/tr_curve.c
	$(DO_CC)

$(BUILDDIR)/client/tr_draw.o :          $(REF_DIR)/tr_draw.c
	$(DO_CC)

$(BUILDDIR)/client/tr_flares.o :          $(REF_DIR)/tr_flares.c
	$(DO_CC)

$(BUILDDIR)/client/tr_image.o :         $(REF_DIR)/tr_image.c
	$(DO_CC)

$(BUILDDIR)/client/tr_init.o :          $(REF_DIR)/tr_init.c
	$(DO_CC)

$(BUILDDIR)/client/tr_light.o :         $(REF_DIR)/tr_light.c
	$(DO_CC)

$(BUILDDIR)/client/tr_main.o :          $(REF_DIR)/tr_main.c
	$(DO_CC)

$(BUILDDIR)/client/tr_mesh.o :          $(REF_DIR)/tr_mesh.c
	$(DO_CC)

$(BUILDDIR)/client/tr_misc.o :          $(REF_DIR)/tr_misc.c
	$(DO_CC)

$(BUILDDIR)/client/tr_model.o :         $(REF_DIR)/tr_model.c
	$(DO_CC)

$(BUILDDIR)/client/tr_noise.o :         $(REF_DIR)/tr_noise.c
	$(DO_CC)

$(BUILDDIR)/client/tr_scene.o :         $(REF_DIR)/tr_scene.c
	$(DO_CC)

$(BUILDDIR)/client/tr_shade.o :         $(REF_DIR)/tr_shade.c
	$(DO_CC)

$(BUILDDIR)/client/tr_shader.o :        $(REF_DIR)/tr_shader.c
	$(DO_CC)

$(BUILDDIR)/client/tr_shade_calc.o :         $(REF_DIR)/tr_shade_calc.c
	$(DO_CC)

$(BUILDDIR)/client/tr_shadows.o :        $(REF_DIR)/tr_shadows.c
	$(DO_CC)

$(BUILDDIR)/client/tr_sky.o :           $(REF_DIR)/tr_sky.c
	$(DO_CC)

$(BUILDDIR)/client/tr_smp.o :           $(REF_DIR)/tr_smp.c
	$(DO_CC)

$(BUILDDIR)/client/tr_stripify.o :      $(REF_DIR)/tr_stripify.c
	$(DO_CC)

$(BUILDDIR)/client/tr_subdivide.o :     $(REF_DIR)/tr_subdivide.c
	$(DO_CC)

$(BUILDDIR)/client/tr_surf.o :          $(REF_DIR)/tr_surf.c
	$(DO_CC)

$(BUILDDIR)/client/tr_world.o :         $(REF_DIR)/tr_world.c
	$(DO_CC)

$(BUILDDIR)/client/unix_qgl.o :         $(UNIX_DIR)/unix_qgl.c
	$(DO_CC)

$(BUILDDIR)/client/unix_dedicated.o :   $(UNIX_DIR)/unix_dedicated.c
	$(DO_CC)

$(BUILDDIR)/client/unix_earlycon.o :    $(UNIX_DIR)/unix_earlycon.c
	$(DO_CC)

$(BUILDDIR)/client/unix_main.o :        $(UNIX_DIR)/unix_main.c
	$(DO_CC)

$(BUILDDIR)/client/unix_net.o :         $(UNIX_DIR)/unix_net.c
	$(DO_CC)

$(BUILDDIR)/client/unix_shared.o :      $(UNIX_DIR)/unix_shared.c
	$(DO_CC)

$(BUILDDIR)/client/irix_glimp.o :       $(UNIX_DIR)/irix_glimp.c
	$(DO_CC)

$(BUILDDIR)/client/irix_snd.o :         $(UNIX_DIR)/irix_snd.c
	$(DO_CC)

$(BUILDDIR)/client/irix_input.o :       $(UNIX_DIR)/irix_input.c
	$(DO_CC)

$(BUILDDIR)/client/linux_glimp.o :      $(UNIX_DIR)/linux_glimp.c
	$(DO_CC)

$(BUILDDIR)/client/linux_qgl.o :        $(UNIX_DIR)/linux_qgl.c
	$(DO_CC)

$(BUILDDIR)/client/linux_input.o :      $(UNIX_DIR)/linux_input.c
	$(DO_CC)

$(BUILDDIR)/client/linux_glimp_vga.o :  $(UNIX_DIR)/linux_glimp_vga.c
	$(DO_CC)

$(BUILDDIR)/client/linux_snd.o :        $(UNIX_DIR)/linux_snd.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mixa.o :   $(UNIX_DIR)/snd_mixa.s
	$(DO_AS)

$(BUILDDIR)/client/matha.o :   $(UNIX_DIR)/matha.s
	$(DO_AS)

$(BUILDDIR)/client/sys_dosa.o :   $(UNIX_DIR)/sys_dosa.s
	$(DO_AS)

$(BUILDDIR)/client/linux_input_vga.o :  $(UNIX_DIR)/linux_input_vga.c
	$(DO_CC)

#############################################################################
# DEDICATED SERVER                        
#############################################################################

Q3DED_OBJS = \
	$(BUILDDIR)/ded/sv_bot.o \
	$(BUILDDIR)/ded/sv_client.o \
	$(BUILDDIR)/ded/sv_ccmds.o \
	$(BUILDDIR)/ded/sv_game.o \
	$(BUILDDIR)/ded/sv_init.o \
	$(BUILDDIR)/ded/sv_main.o \
	$(BUILDDIR)/ded/sv_snapshot.o \
	$(BUILDDIR)/ded/sv_world.o \
	\
	$(BUILDDIR)/ded/cm_trace.o \
	$(BUILDDIR)/ded/cm_load.o \
	$(BUILDDIR)/ded/cm_test.o \
	$(BUILDDIR)/ded/cm_patch.o \
	$(BUILDDIR)/ded/cm_tag.o \
	$(BUILDDIR)/ded/cmd.o \
	$(BUILDDIR)/ded/common.o \
	$(BUILDDIR)/ded/cvar.o \
	$(BUILDDIR)/ded/files.o \
	$(BUILDDIR)/ded/gameinfo.o \
	$(BUILDDIR)/ded/md4.o \
	$(BUILDDIR)/ded/msg.o \
	$(BUILDDIR)/ded/net_chan.o \
	\
	$(BUILDDIR)/ded/unix_dedicated.o \
	$(BUILDDIR)/ded/unix_main.o \
	$(BUILDDIR)/ded/unix_net.o \
	$(BUILDDIR)/ded/unix_shared.o \
	\
	$(BUILDDIR)/ded/cl_null.o

$(BUILDDIR)/linuxq3ded : $(Q3DED_OBJS)
	$(CC) $(CFLAGS) -o $@ $(Q3DED_OBJS) $(LDFLAGS)

$(BUILDDIR)/ded/sv_bot.o :			$(SERVER_DIR)/sv_bot.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_client.o :			$(SERVER_DIR)/sv_client.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_ccmds.o :        $(SERVER_DIR)/sv_ccmds.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_game.o :         $(SERVER_DIR)/sv_game.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_init.o :         $(SERVER_DIR)/sv_init.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_main.o :         $(SERVER_DIR)/sv_main.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_snapshot.o :     $(SERVER_DIR)/sv_snapshot.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/sv_world.o :        $(SERVER_DIR)/sv_world.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cm_trace.o :        $(COMMON_DIR)/cm_trace.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cm_load.o :        $(COMMON_DIR)/cm_load.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cm_test.o :        $(COMMON_DIR)/cm_test.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cm_patch.o :        $(COMMON_DIR)/cm_patch.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cm_tag.o :        $(COMMON_DIR)/cm_tag.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cmd.o :             $(COMMON_DIR)/cmd.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/common.o :          $(COMMON_DIR)/common.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cvar.o :            $(COMMON_DIR)/cvar.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/files.o :           $(COMMON_DIR)/files.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/gameinfo.o :        $(COMMON_DIR)/gameinfo.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/md4.o :             $(COMMON_DIR)/md4.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/msg.o :             $(COMMON_DIR)/msg.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/net_chan.o :        $(COMMON_DIR)/net_chan.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/unix_dedicated.o :  $(UNIX_DIR)/unix_dedicated.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/unix_main.o :       $(UNIX_DIR)/unix_main.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/unix_net.o :        $(UNIX_DIR)/unix_net.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/unix_shared.o :     $(UNIX_DIR)/unix_shared.c
	$(DO_DED_CC)

$(BUILDDIR)/ded/cl_null.o :         $(NULL_DIR)/cl_null.c
	$(DO_DED_CC)

#############################################################################
# GAME
#############################################################################

GAME_OBJS = \
	$(BUILDDIR)/game/b_ai.o \
	$(BUILDDIR)/game/b_files.o \
	$(BUILDDIR)/game/b_items.o \
	$(BUILDDIR)/game/b_main.o \
	$(BUILDDIR)/game/b_nav.o \
	$(BUILDDIR)/game/b_navgen.o \
	$(BUILDDIR)/game/bg_misc.o \
	$(BUILDDIR)/game/bg_pmove.o \
	$(BUILDDIR)/game/g_active.o \
	$(BUILDDIR)/game/g_aim.o \
	$(BUILDDIR)/game/g_client.o \
	$(BUILDDIR)/game/g_cmds.o \
	$(BUILDDIR)/game/g_combat.o \
	$(BUILDDIR)/game/g_items.o \
	$(BUILDDIR)/game/g_main.o \
	$(BUILDDIR)/game/g_mem.o \
	$(BUILDDIR)/game/g_misc.o \
	$(BUILDDIR)/game/g_missile.o \
	$(BUILDDIR)/game/g_mover.o \
	$(BUILDDIR)/game/g_spawn.o \
	$(BUILDDIR)/game/g_svcmds.o \
	$(BUILDDIR)/game/g_target.o \
	$(BUILDDIR)/game/g_team.o \
	$(BUILDDIR)/game/g_trigger.o \
	$(BUILDDIR)/game/g_utils.o \
	$(BUILDDIR)/game/g_weapon.o \
	$(BUILDDIR)/game/q_shared.o \
	$(BUILDDIR)/game/q_math.o

$(BUILDDIR)/qagame$(ARCH).$(SHLIBEXT) : $(GAME_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GAME_OBJS)

$(BUILDDIR)/game/b_ai.o :		$(GAME_DIR)/b_ai.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/b_files.o :     $(GAME_DIR)/b_files.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/b_items.o :     $(GAME_DIR)/b_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/b_main.o :     $(GAME_DIR)/b_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/b_nav.o :      $(GAME_DIR)/b_nav.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/b_navgen.o :      $(GAME_DIR)/b_navgen.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/bg_misc.o :   $(GAME_DIR)/bg_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/bg_pmove.o :   $(GAME_DIR)/bg_pmove.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_active.o :   $(GAME_DIR)/g_active.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_aim.o :   $(GAME_DIR)/g_aim.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_client.o :   $(GAME_DIR)/g_client.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_cmds.o :     $(GAME_DIR)/g_cmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_combat.o :   $(GAME_DIR)/g_combat.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_items.o :    $(GAME_DIR)/g_items.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_main.o :     $(GAME_DIR)/g_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_mem.o :     $(GAME_DIR)/g_mem.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_misc.o :     $(GAME_DIR)/g_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_missile.o :  $(GAME_DIR)/g_missile.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_mover.o :    $(GAME_DIR)/g_mover.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_spawn.o :    $(GAME_DIR)/g_spawn.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_svcmds.o :   $(GAME_DIR)/g_svcmds.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_target.o :   $(GAME_DIR)/g_target.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_team.o :   $(GAME_DIR)/g_team.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_trigger.o :  $(GAME_DIR)/g_trigger.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_utils.o :    $(GAME_DIR)/g_utils.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/g_weapon.o :   $(GAME_DIR)/g_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/game/q_shared.o :   $(GAME_DIR)/q_shared.c
	$(DO_SHLIB_DEBUG_CC)

$(BUILDDIR)/game/q_math.o :   $(GAME_DIR)/q_math.c
	$(DO_SHLIB_CC)

#############################################################################
# CGAME
#############################################################################

CGAME_OBJS = \
	$(BUILDDIR)/cgame/bg_misc.o \
	$(BUILDDIR)/cgame/bg_pmove.o \
	$(BUILDDIR)/cgame/cg_draw.o \
	$(BUILDDIR)/cgame/cg_effects.o \
	$(BUILDDIR)/cgame/cg_ents.o \
	$(BUILDDIR)/cgame/cg_event.o \
	$(BUILDDIR)/cgame/cg_info.o \
	$(BUILDDIR)/cgame/cg_localents.o \
	$(BUILDDIR)/cgame/cg_main.o \
	$(BUILDDIR)/cgame/cg_marks.o \
	$(BUILDDIR)/cgame/cg_menu.o \
	$(BUILDDIR)/cgame/cg_players.o \
	$(BUILDDIR)/cgame/cg_predict.o \
	$(BUILDDIR)/cgame/cg_scoreboard.o \
	$(BUILDDIR)/cgame/cg_snapshot.o \
	$(BUILDDIR)/cgame/cg_view.o \
	$(BUILDDIR)/cgame/cg_weapons.o \
	$(BUILDDIR)/cgame/q_shared.o \
	$(BUILDDIR)/cgame/q_math.o

$(BUILDDIR)/cgame$(ARCH).$(SHLIBEXT) : $(CGAME_OBJS)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(CGAME_OBJS)

$(BUILDDIR)/cgame/bg_misc.o :		$(GAME_DIR)/bg_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/bg_pmove.o :      $(GAME_DIR)/bg_pmove.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_draw.o :       $(CGAME_DIR)/cg_draw.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_effects.o :       $(CGAME_DIR)/cg_effects.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_ents.o :       $(CGAME_DIR)/cg_ents.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_event.o :      $(CGAME_DIR)/cg_event.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_info.o :      $(CGAME_DIR)/cg_info.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_localents.o :       $(CGAME_DIR)/cg_localents.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_main.o :       $(CGAME_DIR)/cg_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_marks.o :       $(CGAME_DIR)/cg_marks.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_menu.o :       $(CGAME_DIR)/cg_menu.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_players.o :    $(CGAME_DIR)/cg_players.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_predict.o :    $(CGAME_DIR)/cg_predict.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_scoreboard.o :       $(CGAME_DIR)/cg_scoreboard.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_snapshot.o :       $(CGAME_DIR)/cg_snapshot.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_view.o :       $(CGAME_DIR)/cg_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/cg_weapons.o :    $(CGAME_DIR)/cg_weapons.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/cgame/q_shared.o :      $(GAME_DIR)/q_shared.c
	$(DO_SHLIB_DEBUG_CC)

$(BUILDDIR)/cgame/q_math.o :      $(GAME_DIR)/q_math.c
	$(DO_SHLIB_CC)


#############################################################################
# RPM
#############################################################################

###### DISABLED

TMPDIR=/var/tmp
TARDIR=$(TMPDIR)/q3test
TARFILE = q3test-$(VERSION_FN)-$(RPM_RELEASE).$(ARCH).tar

tar:
	if [ ! -d archives ];then mkdir archives;chmod 755 archives;fi
	$(MAKE) copyfiles COPYDIR=$(TARDIR)
	cd $(TARDIR)/..; tar cvf $(TARFILE) q3test && gzip -9 $(TARFILE)
	mv $(TARDIR)/../$(TARFILE).gz archives/.
	chmod 644 archives/$(TARFILE).gz
	rm -rf $(TARDIR)
 
# Make RPMs.  You need to be root to make this work
RPMROOT=/usr/src/redhat
RPM = rpm
RPMFLAGS = -bb
INSTALLDIR = /usr/local/games/q3test
RPMDIR = $(TMPDIR)/q3test-$(VERSION_FN)
DESTDIR= $(RPMDIR)/$(INSTALLDIR)

rpm: q3test.spec
	touch $(RPMROOT)/SOURCES/q3test-$(VERSION_FN).tar.gz
	if [ ! -d archives ];then mkdir archives;fi
	$(MAKE) copyfiles COPYDIR=$(DESTDIR)
	cp $(UNIX_DIR)/quake3.gif $(RPMROOT)/SOURCES/.
	cp q3test.spec $(RPMROOT)/SPECS/.
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) q3test.spec
	rm -rf $(RPMDIR)
	mv $(RPMROOT)/RPMS/$(RPMARCH)/q3test-$(VERSION_FN)-$(RPM_RELEASE).$(RPMARCH).rpm archives/q3test-$(VERSION_FN)-$(RPM_RELEASE).$(RPMARCH).rpm 
	chmod 644 archives/q3test-$(VERSION_FN)-$(RPM_RELEASE).$(RPMARCH).rpm 

copyfiles:
	-mkdirhier $(COPYDIR)
	cp $(BUILD_RELEASE_DIR)/linuxquake3 $(COPYDIR)
	strip $(COPYDIR)/linuxquake3
	chmod 755 $(COPYDIR)/linuxquake3
	cp $(BUILD_RELEASE_DIR)/qagame$(ARCH).$(SHLIBEXT) $(COPYDIR)
	chmod 755 $(COPYDIR)/qagame$(ARCH).$(SHLIBEXT)
	cp $(BUILD_RELEASE_DIR)/cgame$(ARCH).$(SHLIBEXT) $(COPYDIR)
	chmod 755 $(COPYDIR)/cgame$(ARCH).$(SHLIBEXT)
	cp $(BUILD_DIR)/libMesaVoodooGL.so.3.1 $(COPYDIR)/.
	chmod 755 $(COPYDIR)/libMesaVoodooGL.so.3.1
	-mkdir $(COPYDIR)/demoq3
	chmod 1777 $(COPYDIR)/demoq3
	cp $(DEMO_PAK) $(COPYDIR)/demoq3/pak0.pk3
	cp $(UNIX_DIR)/README.EULA $(COPYDIR)
	chmod 644 $(COPYDIR)/README.EULA
	cp $(UNIX_DIR)/README.Q3Test $(COPYDIR)
	chmod 644 $(COPYDIR)/README.Q3Test

q3test.spec : $(UNIX_DIR)/q3test.spec.sh Makefile
	sh $< $(VERSION_FN) $(RPM_RELEASE) $(ARCH) $(INSTALLDIR) > $@

#############################################################################
# MISC
#############################################################################

clean: clean-debug clean-release

clean-debug:
	$(MAKE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS)"

clean-release:
	$(MAKE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(DEBUG_CFLAGS)"

