LOCAL_PATH:= $(call my-dir)

# ================================================================
# Unit tests for libstagefright_timedtext
# See also /development/testrunner/test_defs.xml
# ================================================================

# ================================================================
# A test for TimedTextSRTSource
# ================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := TimedTextSRTSource_test

LOCAL_MODULE_TAGS := eng tests

LOCAL_SRC_FILES := TimedTextSRTSource_test.cpp

LOCAL_C_INCLUDES := \
    $(TOP)/external/expat/lib \
    $(TOP)/frameworks/base/media/libstagefright/timedtext

LOCAL_SHARED_LIBRARIES := \
    libexpat \
    libstagefright

include $(BUILD_NATIVE_TEST)

# ================================================================
# A test for TTMLUtils
# ================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := TTMLUtils_test

LOCAL_MODULE_TAGS := eng tests

LOCAL_SRC_FILES := TTMLUtils_test.cpp

LOCAL_C_INCLUDES := \
    frameworks/av/media/libstagefright/timedtext

LOCAL_SHARED_LIBRARIES := \
    libstagefright \
    libstagefright_foundation

include $(BUILD_NATIVE_TEST)

# ================================================================
# A test for TimedTextTTMLSource
# ================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := TimedTextTTMLSource_test

LOCAL_MODULE_TAGS := eng tests

LOCAL_SRC_FILES := TimedTextTTMLSource_test.cpp

LOCAL_C_INCLUDES := \
    external/expat/lib \
    frameworks/av/media/libstagefright/timedtext

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    libexpat \
    liblog \
    libstagefright \
    libstagefright_foundation \
    libutils

include $(BUILD_NATIVE_TEST)
