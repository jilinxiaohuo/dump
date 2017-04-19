LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS 	:=-llog
LOCAL_MODULE    := Dump
LOCAL_SRC_FILES := Dump.cpp

LOCAL_SHARED_LIBRARIES := mono

include $(BUILD_SHARED_LIBRARY)

