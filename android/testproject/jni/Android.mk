LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sdltest

SDL := /home/paul/Projects/gsoc/SDL-gsoc2010_android/

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL \
                -I$(SDL)/include

LOCAL_SRC_FILES := \
    importgl.cpp \
    app-android.cpp \
    lesson05.c \

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -lSDL -lEGL -lgcc -L$(SDL) -L$(SDL)/build-scripts/android_libs/

include $(BUILD_SHARED_LIBRARY)
