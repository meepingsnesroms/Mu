LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

include $(CORE_DIR)/build/Makefile.common

COREFLAGS := -funroll-loops -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS) $(COREDEFINES)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
   COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

#set ASM CPU core
ifeq ($(TARGET_ARCH),arm)
   ifneq ($(TARGET_ARCH_ABI),arm64-v8a)
      #armv8 is 64 bit, it cant run < armv8 assembly outside of legacy emulation mode
      #EMU_OPTIMIZE_FOR_ARM = true #disable until the SIGSEGVs are fixed
   endif
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
