LOCAL_PATH:= $(call my-dir)

PREBUILT=$(LOCAL_PATH)/include/prebuilt-armeabi-v7a/

include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libcutils
LOCAL_SRC_FILES 		:= $(PREBUILT)/libcutils.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libutils
LOCAL_SRC_FILES 		:= $(PREBUILT)/libutils.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libbinder
LOCAL_SRC_FILES 		:= $(PREBUILT)/libbinder.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libskia
LOCAL_SRC_FILES 		:= $(PREBUILT)/libskia.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libui
LOCAL_SRC_FILES 		:= $(PREBUILT)/libui.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES       	:= $(PREBUILT)
include $(CLEAR_VARS)
LOCAL_MODULE 			:= libgui
LOCAL_SRC_FILES 		:= $(PREBUILT)/libgui.so
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_MODULE = $(MODULE_NAME)
LOCAL_SRC_FILES := \
	main.cpp \
	server.cpp

LOCAL_CFLAGS += -Wno-multichar -DHAVE_SYS_UIO_H
LOCAL_CPPFLAGS += -DHAVE_SYS_UIO_H
#LOCAL_MODULE:= screenstream
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libskia \
    libui \
    libgui
LOCAL_C_INCLUDES += \
 	$(LOCAL_PATH)/include/system/core/include \
	$(LOCAL_PATH)/include/frameworks/native/include \
	$(LOCAL_PATH)/include/hardware/libhardware/include \
	$(LOCAL_PATH)/include/external/skia/include/core \
	$(LOCAL_PATH)/include/external/skia/include/effects \
	$(LOCAL_PATH)/include/external/skia/include/images \
	$(LOCAL_PATH)/include/external/skia/src/ports \
	$(LOCAL_PATH)/include/external/skia/include/utils
include $(BUILD_EXECUTABLE)
