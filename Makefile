
#------------------------------------------------------------------------------

# Build: devel, release
BUILD := $(shell cat neverball-build.txt 2> /dev/null || echo release)

# Version: MJ.MN.PT.RV.BD
VERSION := $(shell sh scripts/version.sh || echo 1.7.0)

$(info Will make a "$(BUILD)" build of Neverball $(VERSION).)

ifeq ($(FS_VERSION),1)
ifeq ($(ENABLE_FETCH),curl)
$(error The codelib cURL for Neverball requires latest FS version!)
endif
ifeq ($(ENABLE_IAP),paypal)
$(error The Paypal payment for Neverball requires latest FS version!)
endif
endif

ifeq ($(LEGACY_MODE),0)
$(error Sorry, original start position for Neverball are converted to legacy mode)
endif

#------------------------------------------------------------------------------
# Provide a target system hint for the Makefile.
# Recognized PLATFORM values: darwin, mingw, haiku.

ifeq ($(shell uname),Darwin)
	PLATFORM := darwin
endif

ifeq ($(shell uname), FreeBSD)
	PLATFORM := freebsd
endif

ifeq ($(shell uname -o 2> /dev/null),Msys)
	PLATFORM := mingw
endif

ifeq ($(shell uname),Haiku)
	PLATFORM := haiku
endif

#------------------------------------------------------------------------------
# Auto-install dependencies (note: they must be connected to the internet)

LBITS := $(shell getconf LONG_BIT)

ifneq ($(SKIP_INSTALL_DEPS),1)
ifeq ($(PLATFORM),mingw)
ifeq ($(LBITS),64)
$(shell sh scripts/install-deps-msys2-x86_64.sh)
else
$(shell sh scripts/install-deps-msys2-i686.sh)
endif
else
$(shell sh scripts/install-devel-linux.sh)
endif
endif

LIBCURL_PKGNAME   := 'libcurl4-openssl-dev'
LIBCURL_CHECK     := $(shell apt search $(LIBCURL_PKGNAME) | grep installed)
LIBCURL_DOINSTALL := 'apt-get install $(LIBCURL_PKGNAME)'

ifeq ($(PLATFORM),mingw)
ifeq ($(LBITS),64)
LIBCURL_PKGNAME   := 'mingw-w64-x86_64-curl-winssl'
else
LIBCURL_PKGNAME   := 'mingw-w64-i686-curl-winssl'
endif
LIBCURL_CHECK     := $(shell pacman -Q | grep $(LIBCURL_PKGNAME))
LIBCURL_DOINSTALL := 'pacman -Sy $(LIBCURL_PKGNAME)'
endif

ifneq ($(FS_VERSION),1)
ifeq ($(ENABLE_FETCH),curl)
ifeq (, $(LIBCURL_CHECK))
$(error No package $(LIBCURL_PKGNAME) installed, consider doing $(LIBCURL_DOINSTALL))
endif
endif
ifneq (, $(LIBCURL_CHECK))
ENABLE_FETCH := curl
else
$(info No package $(LIBCURL_PKGNAME) installed, so downloads are disabled)
endif
DATADIR := ./data
endif

#------------------------------------------------------------------------------
# Paths (packagers might want to set DATADIR and LOCALEDIR)

USERDIR   := .neverball
DATADIR   := ./data
LOCALEDIR := ./locale

ifeq ($(PLATFORM),mingw)
	USERDIR := Neverball_$(VERSION)
endif

ifeq ($(PLATFORM),haiku)
	USERDIR := /config/settings/neverball
endif

ifneq ($(BUILD),release)
	USERDIR := $(USERDIR)-dev
endif

#------------------------------------------------------------------------------
# Optional flags (CFLAGS, CPPFLAGS, ...)

ifeq ($(DEBUG),1)
	CFLAGS   := -g
	CXXFLAGS := -g
	CPPFLAGS := -D_DEBUG
ifeq ($(BUILD),release)
	# Release builds cannot be continue
	CFLAGS   := -O2
	CXXFLAGS := -O2
	CPPFLAGS := -DNDEBUG
endif
else
	CFLAGS   := -O2
	CXXFLAGS := -O2
	CPPFLAGS := -DNDEBUG
endif

ifneq ($(BUILD),release)
	CFLAGS   += -DDEVEL_BUILD
	CXXFLAGS += -DDEVEL_BUILD
endif

ifneq ($(CRT_SECURE_NO_WARNINGS),0)
	CFLAGS   += -D_CRT_SECURE_NO_WARNINGS
	CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS
endif

#------------------------------------------------------------------------------
# Mandatory flags

# New design specifications for entities in Neverball
#
# We're introducing new design specifications for entities in Neverball.
# If you do not do so,  your Entities will be displayed in legacy mode.
# Entities created after June 20, 2020 must be meet the new electricity
# before the level is compiled.

ALL_CPPFLAGS := -DLEGACY_MODE=1

ifeq ($(NEVERBALL_FAMILY_API),xbox)
	# Xbox
	ALL_CPPFLAGS += -DNEVERBALL_FAMILY_API=1
endif
ifeq ($(NEVERBALL_FAMILY_API),xbox360)
	# Xbox 360
	ALL_CPPFLAGS += -DNEVERBALL_FAMILY_API=2
endif
ifeq ($(NEVERBALL_FAMILY_API),ps4)
	# PS4
	ALL_CPPFLAGS += -DNEVERBALL_FAMILY_API=3
endif
ifeq ($(NEVERBALL_FAMILY_API),switch)
	# Switch
	ALL_CPPFLAGS += -DNEVERBALL_FAMILY_API=4
endif
ifeq ($(NEVERBALL_FAMILY_API),handset)
	# Handset
	ALL_CPPFLAGS += -DNEVERBALL_FAMILY_API=5
endif

$(info You had been choosen edition $(EDITION))
ifeq ($(EDITION),home)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=0
else
ifeq ($(EDITION),pro)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=1
else
ifeq ($(EDITION),enterprise)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=2
else
ifeq ($(EDITION),education)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=3
else
ifeq ($(EDITION),server_essentials)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=10000
else
ifeq ($(EDITION),server_standard)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=10001
else
ifeq ($(EDITION),server_datacenter)
ALL_CPPFLAGS += -DNEVERBALL_EDITION=10002
else
ALL_CPPFLAGS += -DNEVERBALL_EDITION=-1
endif
endif
endif
endif
endif
endif
endif

# ALL_CPPFLAGS += -DENABLE_ACCOUNT_BINARY

# Compiler...
ifeq ($(ENABLE_TILT),wii)
	# -std=c99 because we need isnormal and -fms-extensions because
	# libwiimote headers make heavy use of the "unnamed fields" GCC
	# extension.

	ALL_CFLAGS := -Wall -Wshadow -std=c99 -pedantic -fms-extensions $(CFLAGS)
else
	# Standard C++17 or C99
	ALL_CFLAGS := -Wall -Wshadow -std=c99 -pedantic $(CFLAGS)
endif

ALL_CXXFLAGS := -fno-rtti -fno-exceptions $(CXXFLAGS)

ifeq ($(ENABLE_TILT),wii)
	ALL_CFLAGS += -D__WII__ -D__WIIU__
else
ifeq ($(ENABLE_TILT),wiiuse)
	ALL_CFLAGS += -D__WII__ -D__WIIU__
else
ifeq ($(ENABLE_TILT),loop)
	ALL_CFLAGS += -D__HILLCREST_LOOP__
else
ifeq ($(ENABLE_TILT),leapmotion)
	ALL_CXXFLAGS += -D__LEAPMOTION__
endif
endif
endif
endif

# Preprocessor...
# New: Game Transfer
ifeq ($(ENABLE_GAME_TRANSFER_SOURCE),1)
	GAMETRANSFER_CPPFLAGS := -DENABLE_GAME_TRANSFER=1
endif

ifeq ($(ENABLE_GAME_TRANSFER),1)
	GAMETRANSFER_CPPFLAGS := -DENABLE_GAME_TRANSFER=1 -DGAME_TRANSFER_TARGET=1
endif

# New: Steam support
ifeq ($(ENABLE_STEAM),1)
	STEAM_CPPFLAGS := -I/usr/local/include -DSTEAM_GAMES=1
endif

# New: Dedicated server
ifeq ($(ENABLE_DEDICATED_SERVER),1)
	DEDICATED_CPPFLAGS := -DENABLE_DEDICATED_SERVER=1
endif

# New: Recipes for disaster
ifeq ($(ENABLE_RFD),1)
	RFD_CPPFLAGS := -DENABLE_RFD=1
endif

# Basically used
SDL_CPPFLAGS := $(shell sdl2-config --cflags)
PNG_CPPFLAGS := $(shell libpng-config --cflags)
JPEG_CPPFLAGS := $(shell pkg-config --cflags libjpeg)

ALL_CPPFLAGS += $(GAMETRANSFER_CPPFLAGS) $(DEDICATED_CPPFLAGS) \
	$(STEAM_CPPFLAGS) $(SDL_CPPFLAGS) $(PNG_CPPFLAGS) $(SQL_CPPFLAGS) \
	$(RFD_CPPFLAGS) -Iwgcl -Ipayments -Ishare

ALL_CPPFLAGS += \
	-DCONFIG_USER=\"$(USERDIR)\" \
	-DCONFIG_DATA=\"$(DATADIR)\" \
	-DCONFIG_LOCALE=\"$(LOCALEDIR)\"

ifeq ($(ENABLE_OPENGLES),1)
	ALL_CPPFLAGS += -DENABLE_OPENGLES=1
endif

ifeq ($(ENABLE_NLS),0)
	ALL_CPPFLAGS += -DENABLE_NLS=0
else
	ALL_CPPFLAGS += -DENABLE_NLS=1 -DNLS_GETTEXT=1
endif

ifeq ($(ENABLE_HMD),openhmd)
	ALL_CPPFLAGS += -DENABLE_HMD=1 -D__OHMD__
endif
ifeq ($(ENABLE_HMD),libovr)
	ALL_CPPFLAGS += -DENABLE_HMD=1 -D__OVR__
endif

ifeq ($(ENABLE_RADIANT_CONSOLE),1)
	ALL_CPPFLAGS += -DENABLE_RADIANT_CONSOLE=1
endif

ifneq ($(BUILD),release)
	ALL_CPPFLAGS += -DENABLE_VERSION=1
endif

MKLOCALVAR_CURL_PREPARED := 0

ifneq ($(FS_VERSION),1)
# Paypal payment support
ifeq ($(ENABLE_IAP),paypal)
	ALL_CPPFLAGS +=						\
		$(shell curl-config --cflags)	\
		-DENABLE_FETCH=1
	MKLOCALVAR_CURL_PREPARED := 1
endif
endif

ifeq ($(ENABLE_FETCH),curl)
ifneq ($(MKLOCALVAR_CURL_PREPARED),1)
	ALL_CPPFLAGS +=						\
		$(shell curl-config --cflags)	\
		-DENABLE_FETCH=1
endif
endif

ifeq ($(PLATFORM),darwin)
	ALL_CFLAGS += -Wno-newline-eof
	ALL_CPPFLAGS += \
		-DGL_SILENCE_DEPRECATION=1 \
		$(patsubst %, -I%, $(wildcard /opt/local/include \
		                              /usr/local/include \
		                              /opt/homebrew/include))
endif

ALL_CPPFLAGS += $(CPPFLAGS)

#------------------------------------------------------------------------------
# HMD handling is a complicated with 6 platform-backend combinations.

ifeq ($(ENABLE_HMD),openhmd)
	HMD_LIBS := -lopenhmd
endif

ifeq ($(ENABLE_HMD),libovr)
	HMD_LIBS     := -L/usr/local/OculusSDK/LibOVR/Lib/Linux/Release/x86_64 -lovr -ludev -lX11 -lXinerama
	HMD_CPPFLAGS := -I/usr/local/OculusSDK/LibOVR/Include

	ifeq ($(PLATFORM),mingw)
		HMD_LIBS     := -L/usr/local/OculusSDK/LibOVR/Lib/MinGW/Release/w32 -lovr -lsetupapi -lwinmm
		HMD_CPPFLAGS := -I/usr/local/OculusSDK/LibOVR/Include
	endif
	ifeq ($(PLATFORM),darwin)
		HMD_LIBS     := -L/usr/local/OculusSDK/LibOVR/Lib/MacOS/Release -lovr -framework IOKit -framework CoreFoundation -framework ApplicationServices
		HMD_CPPFLAGS := -I/usr/local/OculusSDK/LibOVR/Include
	endif
endif

ALL_CPPFLAGS += $(HMD_CPPFLAGS)

#------------------------------------------------------------------------------
# Libraries
# New: Steam support
ifeq ($(ENABLE_STEAM),1)
	STEAM_LIBS := -L/usr/local/lib/steam/libsteam_api.so -lsteam_api
endif

# New: Dedicated server
ifeq ($(ENABLE_DEDICATED_SERVER),1)
	DEDICATED_LIBS := -L"."                  \
					  -lneverball_net_client \
					  -lws2_32
endif

# Basically used
SDL_LIBS := $(shell sdl2-config --libs)
PNG_LIBS := $(shell libpng-config --libs)
PNG_LIBS += $(shell libpng-config --ldflags)
JPEG_LIBS := $(shell pkg-config --libs libjpeg)

ENABLE_FS := stdio
ifeq ($(ENABLE_FS),stdio)
FS_LIBS :=
endif
ifeq ($(FS_VERSION),1)
ALL_CPPFLAGS += -DFS_VERSION_1
ifeq ($(ENABLE_FS),physfs)
FS_LIBS := -lphysfs
endif
endif

ifneq ($(FS_VERSION),1)
ifeq ($(ENABLE_FETCH),curl)
	FETCH_LIBS += $(shell curl-config --libs) 
endif
# Paypal payment support
ifeq ($(ENABLE_IAP),paypal)
	PAYPAL_LIBS += $(shell curl-config --libs) 
endif
endif

# The  non-conditionalised values  below  are specific  to the  native
# system. The native system of this Makefile is Linux (or GNU+Linux if
# you prefer). Please be sure to  override ALL of them for each target
# system in the conditional parts below.

INTL_LIBS :=

ifeq ($(ENABLE_TILT),wii)
	TILT_LIBS := -lcwiimote -lbluetooth
else
ifeq ($(ENABLE_TILT),wiiuse)
	TILT_LIBS := -lbluetooth
else
ifeq ($(ENABLE_TILT),loop)
	TILT_LIBS := -lusb-1.0 -lfreespace
else
ifeq ($(ENABLE_TILT),leapmotion)
	TILT_LIBS := /usr/lib/Leap/libLeap.so -Wl,-rpath,/usr/lib/Leap
endif
endif
endif
endif

ifeq ($(ENABLE_OPENGLES),1)
	OGL_LIBS := -lGLESv1_CM
else
	OGL_LIBS := -lGL
endif

ifeq ($(PLATFORM),mingw)
	ifneq ($(ENABLE_NLS),0)
		INTL_LIBS := -lintl
	endif

	TILT_LIBS :=
	OGL_LIBS  := -lopengl32
endif

ifeq ($(PLATFORM),darwin)
	ifneq ($(ENABLE_NLS),0)
		INTL_LIBS := -lintl
	endif

	TILT_LIBS :=
	OGL_LIBS  := -framework OpenGL
endif

ifeq ($(PLATFORM),freebsd)
	ifneq ($(ENABLE_NLS),0)
		INTL_LIBS := -lintl
	endif
endif

ifeq ($(PLATFORM),haiku)
	ifneq ($(ENABLE_NLS),0)
		INTL_LIBS := -lintl
	endif
endif

BASE_LIBS := $(JPEG_LIBS) $(PNG_LIBS) $(FS_LIBS) -lm

ifeq ($(PLATFORM),darwin)
	BASE_LIBS += $(patsubst %, -L%, $(wildcard /opt/local/lib \
	                                           /usr/local/lib \
	                                           /opt/homebrew/lib))
endif

OGG_LIBS := -lvorbisfile
TTF_LIBS := -lSDL2_ttf

ifeq ($(PLATFORM), haiku)
	TTF_LIBS := -lSDL2_ttf -lfreetype
endif

ifeq ($(ENABLE_FETCH),curl)
	CURL_LIBS := $(shell curl-config --libs)
endif

ALL_LIBS := $(DEDICATED_LIBS) $(STEAM_LIBS) $(HMD_LIBS) $(TILT_LIBS)  \
	$(INTL_LIBS) $(TTF_LIBS) $(CURL_LIBS) $(OGG_LIBS) $(SDL_LIBS)     \
	$(OGL_LIBS) $(BASE_LIBS) $(SQL_LIBS) $(FETCH_LIBS) $(PAYPAL_LIBS)

MAPC_LIBS := $(BASE_LIBS)

ifeq ($(PLATFORM), haiku)
	TTF_LIBS := -lSDL2_ttf -lfreetype
endif

ifeq ($(ENABLE_RADIANT_CONSOLE),1)
	MAPC_LIBS += -lSDL2_net
endif

#------------------------------------------------------------------------------

ifeq ($(PLATFORM),mingw)
X := .exe
endif

MAPC_TARG := mapc$(X)
BALL_TARG := neverball$(X)
PUTT_TARG := neverputt$(X)

ifeq ($(PLATFORM),mingw)
	MAPC := $(WINE) ./$(MAPC_TARG)
else
	MAPC := ./$(MAPC_TARG)
endif

#------------------------------------------------------------------------------

MAPC_OBJS := \
	share/vec3.o        \
	share/base_image.o  \
	share/solid_base.o  \
	share/binary.o      \
	share/log.o         \
	share/base_config.o \
	share/common.o      \
	share/fs_common.o   \
	share/fs_png.o      \
	share/fs_jpg.o      \
	share/dir.o         \
	share/array.o       \
	share/list.o        \
	share/mapclib.o     \
	share/mapc.o
BALL_OBJS := \
	payments/currency_curl.o\
	share/accessibility.o\
	share/dbg_config.o  \
	share/st_setup.o    \
	share/lang_gettext.o\
	share/st_common.o   \
	share/vec3.o        \
	share/base_image.o  \
	share/image.o       \
	share/solid_base.o  \
	share/solid_vary.o  \
	share/solid_draw.o  \
	share/solid_all.o   \
	share/mtrl.o        \
	share/part.o        \
	share/geom.o        \
	share/ball.o        \
	share/ease.o        \
	share/transition.o  \
	share/gui.o         \
	share/font.o        \
	share/theme.o       \
	share/base_config.o \
	share/config.o      \
	share/video.o       \
	share/glext.o       \
	share/binary.o      \
	share/state.o       \
	share/audio.o       \
	share/text.o        \
	share/common.o      \
	share/st_end_support.o\
	share/list.o        \
	share/queue.o       \
	share/cmd.o         \
	share/array.o       \
	share/dir.o         \
	share/fbo.o         \
	share/glsl.o        \
	share/fs_common.o   \
	share/fs_png.o      \
	share/fs_jpg.o      \
	share/fs_ov.o       \
	share/log.o         \
	share/console_control_gui.o\
	share/mapclib.o     \
	ball/hud.o          \
	ball/game_common.o  \
	ball/game_client.o  \
	ball/game_server.o  \
	ball/game_proxy.o   \
	ball/game_draw.o    \
	ball/score.o        \
	ball/level.o        \
	ball/progress.o     \
	ball/boost_rush.o   \
	ball/campaign.o     \
	ball/checkpoints.o  \
	ball/powerup.o      \
	ball/mediation.o    \
	ball/set.o          \
	ball/demo.o         \
	ball/demo_dir.o     \
	ball/util.o         \
	ball/st_intro.o     \
	ball/st_tutorial.o  \
	ball/st_campaign_setup.o\
	ball/st_conf.o      \
	ball/st_demo.o      \
	ball/st_save.o      \
	ball/st_goal.o      \
	ball/st_fail.o      \
	ball/st_done.o      \
	ball/st_level.o     \
	ball/st_over.o      \
	ball/st_play.o      \
	ball/st_set.o       \
	ball/st_playmodes.o \
	ball/st_start.o     \
	ball/st_title.o     \
	ball/st_help.o      \
	ball/st_name.o      \
	ball/st_shared.o    \
	ball/st_shop.o      \
	ball/st_pause.o     \
	ball/st_ball.o      \
	ball/st_beam_style.o\
	ball/st_malfunction.o\
	ball/main.o
PUTT_OBJS := \
	share/accessibility.o\
	share/dbg_config.o  \
	share/st_setup.o    \
	share/lang_gettext.o\
	share/st_common.o   \
	share/vec3.o        \
	share/base_image.o  \
	share/image.o       \
	share/solid_base.o  \
	share/solid_vary.o  \
	share/solid_draw.o  \
	share/solid_all.o   \
	share/mtrl.o        \
	share/part.o        \
	share/geom.o        \
	share/ball.o        \
	share/base_config.o \
	share/config.o      \
	share/video.o       \
	share/glext.o       \
	share/binary.o      \
	share/audio.o       \
	share/state.o       \
	share/ease.o        \
	share/transition.o  \
	share/gui.o         \
	share/font.o        \
	share/theme.o       \
	share/text.o        \
	share/common.o      \
	share/list.o        \
	share/fs_common.o   \
	share/fs_png.o      \
	share/fs_jpg.o      \
	share/fs_ov.o       \
	share/dir.o         \
	share/fbo.o         \
	share/glsl.o        \
	share/array.o       \
	share/log.o         \
	share/console_control_gui.o\
	putt/hud.o          \
	putt/game.o         \
	putt/hole.o         \
	putt/course.o       \
	putt/st_all.o       \
	putt/st_conf.o      \
	putt/main.o

BALL_OBJS += share/solid_sim_sol.o
PUTT_OBJS += share/solid_sim_sol.o

ifeq ($(ENABLE_RFD),1)
	BALL_OBJS += share/rfd.o
endif

ifeq ($(PLATFORM),mingw)
	BALL_OBJS += share/joy_xbox.o
	PUTT_OBJS += share/joy_xbox.o
else
	BALL_OBJS += share/joy.o
	PUTT_OBJS += share/joy.o
endif

ifneq ($(FS_VERSION),1)
ifeq ($(ENABLE_IAP),paypal)
	BALL_OBJS += payments/game_payment_curl.o
endif
endif

ifeq ($(ENABLE_STEAM),1)
BALL_OBJS += share/account_steam.o wgcl/account_wgcl.o wgcl/st_wgcl.o
PUTT_OBJS += share/account_steam.o
else
BALL_OBJS += share/account_binary.o wgcl/account_wgcl.o wgcl/st_wgcl.o
PUTT_OBJS += share/account_binary.o
endif

ifeq ($(ENABLE_GAME_TRANSFER_SOURCE),1)
BALL_OBJS += share/account_transfer.o
BALL_OBJS += share/st_transfer_source.o
endif

ifeq ($(ENABLE_GAME_TRANSFER),1)
BALL_OBJS += share/account_transfer.o
BALL_OBJS += share/st_transfer_target.o
endif

ifeq ($(ENABLE_DEDICATED_SERVER),1)
# Dedicated server, most of people worldwide internet
$(info You had been choosen dedicated server)
BALL_OBJS += share/networking_dedicated.o
PUTT_OBJS += share/networking_dedicated.o
else
ifeq ($(EDITION),home)
BALL_OBJS += share/networking_home.o
PUTT_OBJS += share/networking_home.o
else
ifeq ($(EDITION),pro)
BALL_OBJS += share/networking_pro.o
PUTT_OBJS += share/networking_pro.o
else
ifeq ($(EDITION),enterprise)
BALL_OBJS += share/networking_enterprise.o
PUTT_OBJS += share/networking_enterprise.o
else
ifeq ($(EDITION),education)
BALL_OBJS += share/networking_education.o
PUTT_OBJS += share/networking_education.o
else
ifeq ($(EDITION),server_essentials)
BALL_OBJS += share/networking_srv_essentials.o
PUTT_OBJS += share/networking_srv_essentials.o
else
ifeq ($(EDITION),server_standard)
BALL_OBJS += share/networking_srv_standard.o
PUTT_OBJS += share/networking_srv_standard.o
else
ifeq ($(EDITION),server_datacenter)
BALL_OBJS += share/networking_srv_datacenter.o
PUTT_OBJS += share/networking_srv_datacenter.o
else
BALL_OBJS += share/networking_opensource.o
PUTT_OBJS += share/networking_opensource.o
endif
endif
endif
endif
endif
endif
endif
endif

ifeq ($(FS_VERSION),1)
ifeq ($(ENABLE_FS),stdio)
BALL_OBJS += share/fs_stdio.o
PUTT_OBJS += share/fs_stdio.o
MAPC_OBJS += share/fs_stdio.o
endif
ifeq ($(ENABLE_FS),physfs)
BALL_OBJS += share/fs_physfs.o
PUTT_OBJS += share/fs_physfs.o
MAPC_OBJS += share/fs_physfs.o
endif
else
ifeq ($(ENABLE_FS),stdio)
BALL_OBJS += share/fs_stdio.o share/zip.o
PUTT_OBJS += share/fs_stdio.o share/zip.o
MAPC_OBJS += share/fs_stdio.o share/zip.o
endif
endif

ifneq ($(FS_VERSION),1)
BALL_OBJS += share/package.o
BALL_OBJS += share/st_package.o
PUTT_OBJS += share/package.o
PUTT_OBJS += share/st_package.o
endif

ifeq ($(ENABLE_TILT),wii)
BALL_OBJS += share/tilt_wii.o
else
ifeq ($(ENABLE_TILT),wiiuse)
BALL_OBJS += share/tilt_wiiuse.o
else
ifeq ($(ENABLE_TILT),loop)
BALL_OBJS += share/tilt_loop.o
else
ifeq ($(ENABLE_TILT),leapmotion)
BALL_OBJS += share/tilt_leapmotion.o
else
BALL_OBJS += share/tilt_null.o
endif
endif
endif
endif

ifeq ($(ENABLE_HMD),openhmd)
BALL_OBJS += share/hmd_openhmd.o share/hmd_common.o
PUTT_OBJS += share/hmd_openhmd.o share/hmd_common.o
else
ifeq ($(ENABLE_HMD),libovr)
BALL_OBJS += share/hmd_libovr.o share/hmd_common.o
PUTT_OBJS += share/hmd_libovr.o share/hmd_common.o
else
BALL_OBJS += share/hmd_null.o
PUTT_OBJS += share/hmd_null.o
endif
endif

ifeq ($(PLATFORM),mingw)
BALL_OBJS += neverball.ico.o
PUTT_OBJS += neverputt.ico.o
endif

FETCH_OBJS := share/fetch_null.o
ifeq ($(ENABLE_FETCH),curl)
FETCH_OBJS := share/fetch_curl.o
endif

BALL_OBJS += $(FETCH_OBJS)
PUTT_OBJS += $(FETCH_OBJS)

BALL_DEPS := $(BALL_OBJS:.o=.d)
PUTT_DEPS := $(PUTT_OBJS:.o=.d)
MAPC_DEPS := $(MAPC_OBJS:.o=.d)

MAPS := $(shell find data_standard -name "*.map" \! -name "*.autosave.map")
CMAPS := $(shell find data_campaign -name "*.cmap")
SOLS := $(MAPS:%.map=%.sol)
CSOLS := $(CMAPS:%.cmap=%.csol)

DESKTOPS := $(basename $(wildcard dist/*.desktop.in))

# The build environment defines this (or should).
# This is a fallback that likely only works on a Windows host.
WINDRES ?= windres

#------------------------------------------------------------------------------

%.o : %.c
	@$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -MM -MP -MF $*.d -MT "$@" $<
	@$(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) -o $@ -c $<
	@echo "$(CC) $<"

%.o : %.cpp
	@$(CXX) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) -MM -MP -MF $*.d -MT "$@" $<
	@$(CXX) $(ALL_CXXFLAGS) $(ALL_CPPFLAGS) -o $@ -c $<
	@echo "$(CXX) $<"

%.sol : %.map $(MAPC_TARG)
	$(MAPC) $< data_development --skip_verify
	@echo "$(MAPC) $<"

%.csol : %.cmap $(MAPC_TARG)
	$(MAPC) $< data_development --skip_verify --campaign
	@echo "$(MAPC) $<"

%.desktop : %.desktop.in
	sh scripts/translate-desktop.sh < $< > $@

%.ico.o: dist/ico/%.ico
	echo "1 ICON \"$<\"" | $(WINDRES) -o $@

#------------------------------------------------------------------------------

# Don't compile the sols yet, until the game binaries are finished.

ALTERNATIVE_BUILDENV :=

BALL_BUILDCOND :=
PUTT_BUILDCOND :=
MAPC_BUILDCOND :=

ifeq ($(PLATFORM),mingw)
ifneq ($(ALTERNATIVE_BUILDENV),Msys2)
	BALL_BUILDCOND := error Don't compile this project with makefile! Use Microsoft Visual Studio instead, where was premaded. To bypass restrictions, use ALTERNATIVE_BUILDENV=Msys2
	PUTT_BUILDCOND := error Don't compile this project with makefile! Use Microsoft Visual Studio instead, where was premaded. To bypass restrictions, use ALTERNATIVE_BUILDENV=Msys2
	MAPC_BUILDCOND := error Don't compile this project with makefile! Use Microsoft Visual Studio instead, where was premaded. To bypass restrictions, use ALTERNATIVE_BUILDENV=Msys2
endif
endif

#all : $(BALL_BUILDCOND) $(PUTT_BUILDCOND) $(MAPC_BUILDCOND) $(BALL_TARG) $(PUTT_TARG) $(MAPC_TARG) locales
#all : $(error No target specified! Usage: ball, putt, mapc, publish, test, sols, csols, locales, desktops, clean-src, clean-sols, clean)

ball : $(BALL_BUILDCOND) $(BALL_TARG)
putt : $(PUTT_BUILDCOND) $(PUTT_TARG)
mapc : $(MAPC_BUILDCOND) $(MAPC_TARG)

publish : $(BALL_BUILDCOND) $(PUTT_BUILDCOND) $(MAPC_BUILDCOND) \
	$(BALL_TARG) $(PUTT_TARG) $(MAPC_TARG) sols locales desktops

test : $(BALL_BUILDCOND) $(PUTT_BUILDCOND) $(MAPC_BUILDCOND) \
	$(BALL_TARG) $(PUTT_TARG) $(MAPC_TARG)

ifeq ($(ENABLE_HMD),libovr)
LINK := $(CXX) $(ALL_CXXFLAGS)
else
ifeq ($(ENABLE_TILT),leapmotion)
LINK := $(CXX) $(ALL_CXXFLAGS)
else
ifeq ($(ENABLE_STEAM),1)
LINK := $(CXX) $(ALL_CXXFLAGS)
else
LINK := $(CC) $(ALL_CFLAGS)
endif
endif
endif

$(BALL_TARG) : $(BALL_OBJS)
	@$(LINK) -o $(BALL_TARG) $(BALL_OBJS) $(LDFLAGS) $(ALL_LIBS)
	@echo "$(LINK) $<"

$(PUTT_TARG) : $(PUTT_OBJS)
	@$(LINK) -o $(PUTT_TARG) $(PUTT_OBJS) $(LDFLAGS) $(ALL_LIBS)
	@echo "$(LINK) $<"

$(MAPC_TARG) : $(MAPC_OBJS)
	@$(CC) $(ALL_CFLAGS) -o $(MAPC_TARG) $(MAPC_OBJS) $(LDFLAGS) $(MAPC_LIBS)
	@echo "$(CC) $<"

# Work around some extremely helpful sdl-config scripts.

ifeq ($(PLATFORM),mingw)
$(MAPC_TARG) : ALL_CPPFLAGS := $(ALL_CPPFLAGS) -Umain
endif

sols : $(SOLS)

csols : $(CSOLS)

locales :
ifneq ($(ENABLE_NLS),0)
	@$(MAKE) -C po
endif

desktops : $(DESKTOPS)

clean-src :
	@$(RM) $(BALL_TARG) $(PUTT_TARG) $(MAPC_TARG)
	@find ball share putt \( -name '*.o' -o -name '*.d' \) -delete
	@$(RM) neverball.ico.o neverputt.ico.o

clean-sols :
	@$(RM) $(SOLS)
	@$(RM) $(CSOLS)

clean : clean-src clean-sols
	@$(RM) $(DESKTOPS)
	@$(MAKE) -C po clean

#------------------------------------------------------------------------------

.PHONY : ball putt mapc publish test sols csols locales desktops clean-src \
	clean-sols clean

-include $(BALL_DEPS) $(PUTT_DEPS) $(MAPC_DEPS)

#------------------------------------------------------------------------------
