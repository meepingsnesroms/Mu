LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"

include $(CORE_DIR)/build/Makefile.common

COREFLAGS := -ffast-math -funroll-loops -D__LIBRETRO__ -DINLINE=inline -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS) $(COREDEFINES)

ifneq ($(GIT_VERSION), " unknown")
	COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_ASM)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/build/link.T

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
	LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
