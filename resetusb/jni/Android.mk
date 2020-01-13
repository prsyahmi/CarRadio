LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_MODULE := gpstest
LOCAL_SRC_FILES := test.cpp
LOCAL_CPPFLAGS := -std=c++11 -Wall  -fexceptions
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -fPIE -pie   # whatever ld flags you like

#LOCAL_LDLIBS := -L$(LOCAL_PATH)/libs/ -lhardware
#LOCAL_STATIC_LIBRARIES := libhardware

include $(BUILD_EXECUTABLE)    # <-- Use this to build an executable.