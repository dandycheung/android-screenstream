ifndef ANDROID_SDK_ROOT
	$(error "ANDROID_SDK_ROOT not set")
endif
ifndef ANDROID_NDK_HOME
ANDROID_NDK_HOME=$(ANDROID_SDK_ROOT)/ndk
endif

PROJECT_PATH := $(PWD)
BUILD_PATH := $(PWD)/build
NDK_OUT := $(BUILD_PATH)
NDK_LIBS_OUT := $(BUILD_PATH)
APP_BUILD_SCRIPT  := $(PWD)/Android.mk
APP_ABI := armeabi-v7a
LOCAL_MODULE:= screenstream

ANDROID_ARGS := \
	PROJECT_PATH=$(PROJECT_PATH) \
	APP_BUILD_SCRIPT=$(APP_BUILD_SCRIPT) \
	APP_ABI=$(APP_ABI) \
	NDK_OUT=$(NDK_OUT) \
	NDK_LIBS_OUT=$(NDK_LIBS_OUT) \
	PROJECT_PATH=$(PROJECT_PATH) \
	APP_PLATFORM=android-19 \
	NDK_PROJECT_PATH=null

ANDROID_BUILD := @make  --no-print-dir -f $(ANDROID_NDK_HOME)/build/core/build-local.mk $(ANDROID_ARGS)
UNLOCK_SCREEN = input keyevent 26 && input swipe 360 320 360 900 100

build:
ifeq (,$(wildcard include/frameworks))
	make -C include
endif
	$(ANDROID_BUILD)

clean:
	$(ANDROID_BUILD) clean
	@rm -rf $(BUILD_PATH)/* 2>/dev/null

exec: build
	@adb push $(NDK_OUT)/$(APP_ABI)/$(LOCAL_MODULE) /data/local/tmp/ > /dev/null
	@adb shell "$(UNLOCK_SCREEN) && \
		cd /data/local/tmp/ && \
		busybox chmod +x $(LOCAL_MODULE) && \
		(pidof $(LOCAL_MODULE) && killall $(LOCAL_MODULE) || true ) && \
		./$(LOCAL_MODULE) 192.168.1.10 &"

libs:
	$(ANDROID_NDK_HOME)/ndk-build \
	NDK_PROJECT_PATH=$(PWD)/libs \
	APP_BUILD_SCRIPT=$(PWD)/libs/Android.mk \
	APP_ALLOW_MISSING_DEPS=true \
	APP_ABI=$(APP_ABI) \
	APP_PLATFORM=android-19

.PHONY: build clean exec libs