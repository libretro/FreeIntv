STATIC_LINKING := 0
AR := ar

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
	platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
	platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif


CORE_DIR	+= .
TARGET_NAME := freeintv
SOURCE_DIR := src
LIBM			= -lm

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
	ARCHFLAGS = -arch ppc -arch ppc64
else
	ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
	CXXFLAGS += $(ARCHFLAGS)
	LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
	fpic := -fPIC -nostdlib
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=
else ifneq (,$(findstring osx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
else ifneq (,$(findstring ios,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(IOSSDK),)
	IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

DEFINES := -DIOS
CC = cc -arch armv7 -isysroot $(IOSSDK)
LD = cc -arch armv7 -isysroot $(IOSSDK)

ifeq ($(platform),ios9)
	CC	+= -miphoneos-version-min=8.0
	CXXFLAGS += -miphoneos-version-min=8.0
else
	CC	+= -miphoneos-version-min=5.0
	CXXFLAGS += -miphoneos-version-min=5.0
endif

# PS3
else ifeq ($(platform), ps3)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
	AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
	ENDIANNESS_DEFINES := -DMSB_FIRST
	PLATFORM_DEFINES := -D__CELLOS_LV2__ -D__ppc__
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# PS3 (SNC)
else ifeq ($(platform), sncps3)
	TARGET := $(TARGET_NAME)_libretro_ps3.a
	CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
	AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
	ENDIANNESS_DEFINES := -DMSB_FIRST
	CFLAGS += -DWORDS_BIGENDIAN=1
	GCC_DEFINES :=
	PLATFORM_DEFINES := -D__CELLOS_LV2__ -D__ppc__
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# Lightweight PS3 Homebrew SDK
else ifeq ($(platform), psl1ght)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
	ENDIANNESS_DEFINES := -DMSB_FIRST
	CFLAGS += -DWORDS_BIGENDIAN=1
	GCC_DEFINES :=
	PLATFORM_DEFINES := -D__CELLOS_LV2__ -D__ppc__
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# Xbox 360 (libxenon)
else ifeq ($(platform), xenon)
	TARGET := $(TARGET_NAME)_libretro.a
	CC = xenon-gcc$(EXE_EXT)
	AR = xenon-ar$(EXE_EXT)
	ENDIANNESS_DEFINES := -DMSB_FIRST
	CFLAGS += -D__LIBXENON__ -m32 -D__ppc__
	PLATFORM_DEFINES := -D__LIBXENON__ -D__ppc_
	STATIC_LINKING := 1

# Nintendo Game Cube
else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES := -DMSB_FIRST
	PLATFORM_DEFINES += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float
	PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# QNX / BLackberry
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
# Nintendo Wii
else ifeq ($(platform), wii)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES := -DMSB_FIRST
	PLATFORM_DEFINES += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
	PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# Nintendo WiiU
else ifeq ($(platform), wiiu)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	ENDIANNESS_DEFINES := -DMSB_FIRST
	PLATFORM_DEFINES += -DGEKKO -DWIIU -DHW_RVL -mwup -mcpu=750 -meabi -mhard-float
	PLATFORM_DEFINES += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	HAVE_RZLIB := 1
	STATIC_LINKING := 1

# Nintendo Switch (libtransistor)
else ifeq ($(platform), switch)
	EXT=a
	TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
	include $(LIBTRANSISTOR_HOME)/libtransistor.mk
	HAVE_RZLIB := 1
	STATIC_LINKING=1
	
# CTR/3DS
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	CFLAGS += -DARM11 -D_3DS
	CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
	HAVE_RZLIB := 1
	DISABLE_ERROR_LOGGING := 1
	ARM = 1
	STATIC_LINKING := 1
	
# Emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_emscripten.bc
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	STATIC_LINKING := 1

# Playstation Vita
else ifeq ($(platform), vita)
	TARGET := $(TARGET_NAME)_vita.a
	CC = arm-vita-eabi-gcc
	AR = arm-vita-eabi-ar
	CXXFLAGS += -Wl,-q -Wall -O3
	STATIC_LINKING = 1

# GCW0
else ifeq ($(platform), gcw0)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
	AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=link.T -Wl,-no-undefined
	
	DISABLE_ERROR_LOGGING := 1
	CFLAGS += -march=mips32 -mtune=mips32r2 -mhard-float
	
# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/lib/amd64")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib/x64
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib/x64

	WindowsSdkDirInc := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include
	WindowsSdkDirInc ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE)
	export LIB := $(LIB);$(WindowsSdkDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	LIBS =

# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib

	WindowsSdkDirInc := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include
	WindowsSdkDirInc ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE)
	export LIB := $(LIB);$(WindowsSdkDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	LIBS =

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
	BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

	WindowsSdkDir := $(INETSDK)

	export INCLUDE := $(INCLUDE);$(INETSDK)/Include;src/libretro/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
	LIBS =
	
# Windows MSVC 2003 Xbox 1
else ifeq ($(platform), xbox1_msvc2003)
	TARGET := $(TARGET_NAME)_libretro_xdk1.lib
	MSVCBINDIRPREFIX = $(XDK)/xbox/bin/vc71
	CC  = "$(MSVCBINDIRPREFIX)/CL.exe"
	CXX  = "$(MSVCBINDIRPREFIX)/CL.exe"
	LD   = "$(MSVCBINDIRPREFIX)/lib.exe"

	export INCLUDE := $(XDK)/xbox/include
	export LIB := $(XDK)/xbox/lib
	PSS_STYLE :=2
	CFLAGS   += -D_XBOX -D_XBOX1
	CXXFLAGS += -D_XBOX -D_XBOX1
	STATIC_LINKING=1
	
# Windows MSVC 2010 Xbox 360
else ifeq ($(platform), xbox360_msvc2010)
	TARGET := $(TARGET_NAME)_libretro_xdk360.lib
	MSVCBINDIRPREFIX = $(XEDK)/bin/win32
	CC  = "$(MSVCBINDIRPREFIX)/cl.exe"
	CXX  = "$(MSVCBINDIRPREFIX)/cl.exe"
	LD   = "$(MSVCBINDIRPREFIX)/lib.exe"

	export INCLUDE := $(XEDK)/include/xbox
	export LIB := $(XEDK)/lib/xbox
	PSS_STYLE :=2
	CFLAGS   += -D_XBOX -D_XBOX360
	CXXFLAGS += -D_XBOX -D_XBOX360
	STATIC_LINKING=1
	BIGENDIAN = 1

# Current Windows builds via mingw/gcc
else
	TARGET := $(TARGET_NAME)_libretro.dll
	CC = gcc
	SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	CFLAGS += -D__WIN32__ -D__WIN32_LIBRETRO__ -Wno-missing-field-initializers
endif

LDFLAGS += $(LIBM)

ifneq ($(platform), sncps3)
	ifeq (,$(findstring msvc,$(platform)))
		CFLAGS += -Wall -Wno-sign-compare -Wunused \
		-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
		-Wshadow -Wstrict-prototypes \
		-Wformat-security -Wwrite-strings \
		-Wdisabled-optimization
	endif
endif

ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -Wall -Wno-unused -g
else
	CFLAGS += -O2 -DNDEBUG
endif

ifeq (,$(findstring msvc,$(platform)))
	CFLAGS += -fomit-frame-pointer -fstrict-aliasing
endif

ifeq ($(DEBUG), 1)
	CXXFLAGS += -O0 -g
else
	CXXFLAGS += -O3
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

CFLAGS	+= -Wall -D__LIBRETRO__ $(INCLUDES) $(fpic)
CXXFLAGS += -Wall -D__LIBRETRO__ $(INCLUDES) $(fpic)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
