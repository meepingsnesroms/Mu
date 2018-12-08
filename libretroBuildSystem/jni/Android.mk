LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

# prevent persisting in non ARM builds after the first ARM build
EMU_OPTIMIZE_FOR_ARM32 = 0

# set ASM CPU core, only use with ARMv4<->7, ARMv8 is its own architecture
ifeq ($(TARGET_ARCH),arm)
	ifneq ($(TARGET_ARCH_ABI),arm64-v8a)
		EMU_OPTIMIZE_FOR_ARM32 = 1
	endif
endif

include $(CORE_DIR)/build/Makefile.common

COREFLAGS := -ffast-math -funroll-loops -D__LIBRETRO__ -DINLINE=inline -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS) $(COREDEFINES)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/build/link.T

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
