LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sdlapp
SDL := ../../../

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL \
                -I$(SDL)/include

LOCAL_SRC_FILES := \
    android-support.cpp \
    lesson05.c \

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -lSDL -lgcc -L$(SDL)

include $(BUILD_SHARED_LIBRARY)
