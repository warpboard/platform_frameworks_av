LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                 \
        TextDescriptions.cpp      \
        TimedText3GPPSource.cpp \
        TimedTextDriver.cpp       \
        TimedTextPlayer.cpp
        TimedTextSRTSource.cpp    \
        TimedTextSource.cpp       \
        TimedTextTTMLSource.cpp   \
        TTMLNode.cpp              \
        TTMLParser.cpp            \
        TTMLUtils.cpp

LOCAL_CFLAGS += -Wno-multichar
LOCAL_C_INCLUDES:= \
        $(TOP)/external/expat/lib \
        $(TOP)/frameworks/av/include/media/stagefright/timedtext \
        $(TOP)/frameworks/av/media/libstagefright

LOCAL_MODULE:= libstagefright_timedtext

include $(BUILD_STATIC_LIBRARY)
