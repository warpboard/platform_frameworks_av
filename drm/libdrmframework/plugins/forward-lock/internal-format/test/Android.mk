#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    FwdLockTest.cpp

LOCAL_C_INCLUDES := \
    bionic \
    external/gtest/include \
    external/stlport/stlport \
    frameworks/av/drm/libdrmframework/plugins/forward-lock/internal-format/common \
    frameworks/av/drm/libdrmframework/plugins/forward-lock/internal-format/converter \
    frameworks/av/drm/libdrmframework/plugins/forward-lock/internal-format/decoder

LOCAL_SHARED_LIBRARIES := libcrypto libstlport liblog

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libfwdlock-common

LOCAL_STATIC_LIBRARIES := \
    libgtest \
    libgtest_main \
    libfwdlock-common \
    libfwdlock-converter \
    libfwdlock-decoder

LOCAL_LDLIBS := -llog

LOCAL_MODULE := gtest_fwdlock

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

# The needed resource files needs to be manually copied in the same way as the
# product copy files used to do
#PRODUCT_COPY_FILES += \
#    $(LOCAL_PATH)/res/7bit.dm:/data/drm/fwdlock-test/7bit.dm \
#    $(LOCAL_PATH)/res/8bit.dm:/data/drm/fwdlock-test/8bit.dm \
#    $(LOCAL_PATH)/res/base64.dm:/data/drm/fwdlock-test/base64.dm \
#    $(LOCAL_PATH)/res/binary.dm:/data/drm/fwdlock-test/binary.dm \
#    $(LOCAL_PATH)/res/boundary-too-long.dm:/data/drm/fwdlock-test/boundary-too-long.dm \
#    $(LOCAL_PATH)/res/delimiter-truncated.dm:/data/drm/fwdlock-test/delimiter-truncated.dm \
#    $(LOCAL_PATH)/res/false-match-just-before-delimiter.dm:/data/drm/fwdlock-test/false-match-just-before-delimiter.dm \
#    $(LOCAL_PATH)/res/combined-delivery.dm:/data/drm/fwdlock-test/combined-delivery.dm \
#    $(LOCAL_PATH)/res/interspersed-whitespace.dm:/data/drm/fwdlock-test/interspersed-whitespace.dm \
#    $(LOCAL_PATH)/res/non-whitespace-after-padding.dm:/data/drm/fwdlock-test/non-whitespace-after-padding.dm \
#    $(LOCAL_PATH)/res/preamble.dm:/data/drm/fwdlock-test/preamble.dm \
#    $(LOCAL_PATH)/res/separate-delivery.dm:/data/drm/fwdlock-test/separate-delivery.dm \
#    $(LOCAL_PATH)/res/whitespace-after-boundary.dm:/data/drm/fwdlock-test/whitespace-after-boundary.dm
#endif
