# CardputerZero power menu
#
# Targets:
#   make            host build (compile check, LVGL framebuffer backend)
#   make device     AArch64 build for the CardputerZero
#   make preview    render 320x170 PNG mockups into build/preview/shots
#   make stage      Debian staging tree under build/stage
#   make deb        build the .deb package (needs a device binary)
#   make install    copy the staged tree into /usr/share/APPLaunch (root only)
#   make check      validate desktop entry, icon and binary
#   make clean

SHELL := /bin/bash

APP_SLUG      ?= power-menu
APP_TITLE     ?= Power Menu
APP_VERSION   ?= 0.1.2
APP_SUMMARY   ?= Restart or shut down the CardputerZero safely
MAINTAINER    ?= Publius <liptoon@gmail.com>
HOMEPAGE      ?= https://github.com/Liptoon/cardputer-zero-power-menu
ARCH          ?= arm64

LVGL_DIR      ?= third_party/lvgl
LVGL_REF      ?= v9.5.0
BUILD_ROOT    ?= build
DIST_DIR      ?= dist

DEVICE_CROSS_COMPILE ?= aarch64-linux-gnu-
CROSS_COMPILE ?=
STATIC ?= $(if $(CROSS_COMPILE),1,0)

TARGET     := $(if $(CROSS_COMPILE),device,host)
BUILD_DIR  := $(BUILD_ROOT)/$(TARGET)
LVGL_BUILD := $(BUILD_DIR)/lvgl
LVGL_LIB   := $(LVGL_BUILD)/lib/liblvgl.a

ifeq ($(CROSS_COMPILE),)
BIN := $(BUILD_DIR)/bin/$(APP_SLUG)
else
BIN := $(DIST_DIR)/$(APP_SLUG)
endif
DIST_BIN := $(DIST_DIR)/$(APP_SLUG)

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
CMAKE_CROSS_ARGS := $(if $(CROSS_COMPILE),-DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64)

SOURCES := $(sort $(wildcard main/src/*.cpp))
OBJECTS := $(patsubst main/src/%.cpp,$(BUILD_DIR)/obj/%.o,$(SOURCES))

CPPFLAGS := -DLV_CONF_PATH='"$(abspath lv_conf.h)"' -DAPP_TITLE='"$(APP_TITLE)"' \
            -I$(LVGL_DIR) -Imain/src
CXXFLAGS := -std=c++17 -O2 -g -Wall -Wextra $(CPPFLAGS)

CHECK_BIN := $(if $(wildcard $(DIST_BIN)),$(DIST_BIN),$(BIN))

STAGE_DIR := $(BUILD_ROOT)/stage
STAGE_BIN ?= $(DIST_BIN)
PREVIEW   := $(BUILD_ROOT)/preview/preview

.PHONY: all device dist lvgl preview stage deb install check clean deploy

all: $(BIN)

device:
	@command -v $(DEVICE_CROSS_COMPILE)g++ >/dev/null 2>&1 || { \
		echo "error: $(DEVICE_CROSS_COMPILE)g++ not found (install g++-aarch64-linux-gnu)"; exit 1; }
	$(MAKE) CROSS_COMPILE=$(DEVICE_CROSS_COMPILE) all

dist:
	$(MAKE) device

lvgl: $(LVGL_LIB)

$(LVGL_LIB): lv_conf.h $(LVGL_DIR)/CMakeLists.txt
	cmake -S $(LVGL_DIR) -B $(LVGL_BUILD) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		$(CMAKE_CROSS_ARGS) \
		-DLV_BUILD_CONF_PATH=$(abspath lv_conf.h) \
		-DCONFIG_LV_BUILD_DEMOS=OFF \
		-DCONFIG_LV_BUILD_EXAMPLES=OFF
	cmake --build $(LVGL_BUILD) -j$(shell nproc)

$(BUILD_DIR)/obj/%.o: main/src/%.cpp Makefile lv_conf.h
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN): $(OBJECTS) $(LVGL_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $@ -L$(dir $(LVGL_LIB)) -llvgl -lm -lpthread \
		$(if $(filter 1,$(STATIC)),-static -s)
	@echo "built: $@"

$(PREVIEW): tests/preview_screens.cpp $(filter-out main/src/main.cpp,$(SOURCES)) $(LVGL_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) tests/preview_screens.cpp \
		$(filter-out main/src/main.cpp,$(SOURCES)) \
		-L$(dir $(LVGL_LIB)) -llvgl -lm -lpthread -o $@

preview: BUILD_DIR := $(BUILD_ROOT)/host
preview: $(BUILD_ROOT)/host/lvgl/lib/liblvgl.a
	$(MAKE) BUILD_DIR=$(BUILD_ROOT)/host $(PREVIEW)
	@rm -rf $(BUILD_ROOT)/preview/shots
	@mkdir -p $(BUILD_ROOT)/preview/shots
	./$(PREVIEW) $(BUILD_ROOT)/preview/shots
	@if command -v magick >/dev/null 2>&1; then \
		for f in $(BUILD_ROOT)/preview/shots/*.ppm; do magick "$$f" "$${f%.ppm}.png"; done; \
	else echo "install ImageMagick to convert PPM snapshots to PNG"; fi

stage:
	@test -f $(STAGE_BIN) || { echo "error: $(STAGE_BIN) missing; run 'make device' first"; exit 1; }
	rm -rf $(STAGE_DIR)
	mkdir -p $(STAGE_DIR)/usr/share/APPLaunch/applications \
	         $(STAGE_DIR)/usr/share/APPLaunch/bin \
	         $(STAGE_DIR)/usr/share/APPLaunch/share/images \
	         $(STAGE_DIR)/DEBIAN
	install -m 755 $(STAGE_BIN) $(STAGE_DIR)/usr/share/APPLaunch/bin/$(APP_SLUG)
	install -m 644 applaunch/applications/$(APP_SLUG).desktop \
		$(STAGE_DIR)/usr/share/APPLaunch/applications/
	install -m 644 applaunch/share/images/$(APP_SLUG).png \
		$(STAGE_DIR)/usr/share/APPLaunch/share/images/
	@printf '%s\n' \
		'Package: $(APP_SLUG)' \
		'Version: $(APP_VERSION)' \
		'Architecture: $(ARCH)' \
		'Maintainer: $(MAINTAINER)' \
		'Section: APPLaunch' \
		'Priority: optional' \
		'Homepage: $(HOMEPAGE)' \
		'Description: $(APP_SUMMARY)' \
		> $(STAGE_DIR)/DEBIAN/control
	@echo "staged: $(STAGE_DIR)"

deb: dist stage
	@command -v dpkg-deb >/dev/null 2>&1 || { echo "error: dpkg-deb not installed"; exit 1; }
	dpkg-deb --root-owner-group -b $(STAGE_DIR) $(DIST_DIR)/$(APP_SLUG)_$(APP_VERSION)_$(ARCH).deb
	@echo "deb: $(DIST_DIR)/$(APP_SLUG)_$(APP_VERSION)_$(ARCH).deb"

install: stage
	@test "$$(id -u)" -eq 0 || { echo "error: run as root, no sudo wrapper here"; exit 1; }
	cp -a $(STAGE_DIR)/usr/share/APPLaunch/. /usr/share/APPLaunch/
	@if systemctl list-unit-files APPLaunch.service 2>/dev/null | grep -q '^APPLaunch.service'; then \
		systemctl restart APPLaunch.service; \
	elif systemctl list-unit-files applaunch.service 2>/dev/null | grep -q '^applaunch.service'; then \
		systemctl restart applaunch.service; \
	else \
		echo "no APPLaunch systemd unit; launcher detects new entries by itself"; \
	fi
	@echo "installed; APPLaunch restarted when a matching unit exists"

deploy:
	./tools/deploy.sh \
		$(if $(HOST),--host $(HOST)) \
		$(if $(PORT),--port $(PORT)) \
		$(if $(KEY),--key $(KEY)) \
		$(if $(METHOD),--method $(METHOD)) \
		$(if $(NO_BUILD),--no-build) \
		$(if $(DRY_RUN),--dry-run)

check: $(if $(wildcard $(DIST_BIN)),,$(BIN))
	@test -x $(CHECK_BIN) && echo "binary ok: $(CHECK_BIN)"
	@grep -q '^\[Desktop Entry\]$$' applaunch/applications/$(APP_SLUG).desktop && echo "desktop header ok"
	@grep -q '^Name=.\+' applaunch/applications/$(APP_SLUG).desktop && echo "desktop Name ok"
	@grep -q '^Exec=.\+' applaunch/applications/$(APP_SLUG).desktop && echo "desktop Exec ok"
	@test -f applaunch/share/images/$(APP_SLUG).png && echo "icon ok"
	@file $(CHECK_BIN)

clean:
	rm -rf $(BUILD_ROOT) $(DIST_DIR)
