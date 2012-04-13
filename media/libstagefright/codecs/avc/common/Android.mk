LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/deblock.cpp \
 	src/dpb.cpp \
 	src/fmo.cpp \
 	src/mb_access.cpp \
 	src/reflist.cpp

LOCAL_MODULE := libstagefright_avc_common

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
 	$(LOCAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)
