
URL := https://android.googlesource.com/platform
BASE_TAG := android-4.4.2_r2.0.1
SKIA_TAG := android-4.4.2_r2.0.1

all: PlatformInclude/frameworks/native \
	PlatformInclude/hardware/libhardware \
	PlatformInclude/system/core \
	external/skia

PlatformInclude/%:
	$(info downloading $*/include:$(BASE_TAG))
	mkdir -p $*/include
	wget -O - $(URL)/$*/+archive/refs/tags/$(BASE_TAG)/include.tar.gz | \
	tar -xzC $*/include

external/skia:
	$(info downloading $@:$(SKIA_TAG))
	mkdir -p $@ $@/include $@/src
	wget -O - $(URL)/$@/+archive/refs/tags/$(SKIA_TAG)/include.tar.gz | \
	tar -xzC $@/include
	wget -O - $(URL)/$@/+archive/refs/tags/$(SKIA_TAG)/src.tar.gz | \
	tar -xzC $@/src

clean:
	rm -rf external frameworks hardware system

.PHONY: clean