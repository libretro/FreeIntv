LOCAL_PATH := $(call my-dir)

CORE_DIR    := $(LOCAL_PATH)/..
SOURCE_DIR  := $(CORE_DIR)/src
LIBRETRO_COMM_DIR := $(SOURCE_DIR)/deps/libretro-common

# For Android NDK, define sources explicitly since Makefile.common uses SOURCE_DIR
INCLUDE_DIRS := \
	$(SOURCE_DIR) \
	$(LIBRETRO_COMM_DIR)/include

ANDROID_SOURCES_C := \
	../src/libretro.c \
	../src/intv.c \
	../src/memory.c \
	../src/cp1610.c \
	../src/cart.c \
	../src/controller.c \
	../src/osd.c \
	../src/ivoice.c \
	../src/psg.c \
	../src/stic.c \
	../src/stb_image_impl.c \
	../src/deps/libretro-common/file/file_path.c \
	../src/deps/libretro-common/compat/compat_posix_string.c \
	../src/deps/libretro-common/compat/compat_snprintf.c \
	../src/deps/libretro-common/compat/compat_strl.c \
	../src/deps/libretro-common/compat/compat_strcasestr.c \
	../src/deps/libretro-common/compat/fopen_utf8.c \
	../src/deps/libretro-common/encodings/encoding_utf.c \
	../src/deps/libretro-common/string/stdstring.c \
	../src/deps/libretro-common/time/rtime.c

include $(CLEAR_VARS)
LOCAL_MODULE    := freeintv_libretro
LOCAL_SRC_FILES := $(ANDROID_SOURCES_C)
LOCAL_C_INCLUDES := $(INCLUDE_DIRS)
LOCAL_CFLAGS    := -DANDROID -D__LIBRETRO__ -DHAVE_STRINGS_H -DRIGHTSHIFT_IS_SAR
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
