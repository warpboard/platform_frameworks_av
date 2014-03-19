LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    AudioResamplerCoefficients.cpp \
    AudioResampler.cpp.arm \
    AudioResamplerCubic.cpp.arm \
    AudioResamplerSinc.cpp.arm


LOCAL_MODULE := libaudio-resampler

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES  := libutils liblog libdl libcutils

include $(BUILD_SHARED_LIBRARY)
