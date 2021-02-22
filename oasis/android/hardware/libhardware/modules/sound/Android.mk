#@company : yingzhou.oasis
#@author  : ivan.xiaofei
#@date    : 2017.08.26
#@describe: for sounder


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sound.default

# HAL module implementation stored in
# hw/<SOUND_HARDWARE_MODULE_ID>.default.so
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_C_INCLUDES := hardware/libhardware
LOCAL_SRC_FILES := sound.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

