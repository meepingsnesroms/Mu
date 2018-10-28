LOCAL_PATH := $(call my-dir)
GIT_VERSION := " $(shell git rev-parse --short HEAD)"

include $(CLEAR_VARS)

LOCAL_MODULE    := retro

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM

#arm speedups
#default to thumb because its smaller and more can fit in the cpu cache
LOCAL_ARM_MODE := thumb
#switch to arm or thumb instruction set per function based on speed(dont restrict to only arm or thumb use both)
#LOCAL_CFLAGS += -mthumb-interwork

#enable/disable optimization
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
   LOCAL_CFLAGS += -munaligned-access 
   V7NEONOPTIMIZATION ?= 0
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
#neon is a requirement for armv8 so just enable it
   LOCAL_CFLAGS += -munaligned-access
   LOCAL_ARM_NEON := true
endif

#armv7+ optimizations
ifeq ($(V7NEONOPTIMIZATION),1)
   LOCAL_ARM_NEON := true
endif
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

CORE_DIR := ..

include $(CORE_DIR)/build/Makefile.common

LOCAL_SRC_FILES    += $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CFLAGS += -O3 -std=gnu99 -funroll-loops -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 -DGIT_VERSION=\"$(GIT_VERSION)\" $(INCFLAGS)
#LOCAL_CFLAGS += -ffast-math

include $(BUILD_SHARED_LIBRARY)
