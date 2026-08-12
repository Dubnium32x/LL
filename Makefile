# ============================
#
#	Lincoln's Labyrinthine
#	Makefile
#
# ============================

# ============================
#	Init Config
#	(Change as needed!)
# ============================

# Memory Config
HEAP_SIZE = 8388208
STACK_SIZE = 61800

# Product Config
PRODUCT = lincolnlabyrinthine.pdx
BUNDLE_ID = com.diskodev.lincolnlabyrinthine
VERSION := 0.2.2

# SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif
ifeq ($(SDK),)
$(error SDK path not found; set PLAYDATE_SDK_PATH environment variable)
endif

SDK_VERSION = $(file < $(SDK)/VERSION.txt)

# Expand tilde to HOME directory
SDK := $(patsubst ~%,$(HOME)%,$(SDK))

# Ensure Source directory exists (required by SDK's common.mk)
$(shell mkdir -p Source)
$(shell if [ -f pdxinfo ]; then cp -f pdxinfo Source/; fi)

# Ensure ARM tool wrappers are visible first in PATH (important inside Flatpak VS Code).
# export PATH := $(abspath Tools/hostbin):$(PATH)

# ============================
#	Source Config
# ============================

# Add source directories to VPATH
VPATH += Source/code/**

# Auto-discover source files
SRC = $(shell find Source/code -name '*.c' -print 2>/dev/null)

# User include directories
UINCDIR = $(shell find Source/code -type d)

UDEFS = -DGAME_VERSION=\"$(strip $(VERSION))\"

# Optimization flags
ifdef DEBUG
UDEFS += -DDEBUG=1 -O0 -g
BUILD_TYPE = debug
else
UDEFS += -DNDEBUG=1 -O2
BUILD_TYPE = release
endif

# Additional compiler warnings
UDEFS += -Wall -Wextra -Wno-unused-parameter -std=c23

# User libraries
ULIBDIR =
ULIBS = -specs=nosys.specs

# User ASM files (if any)
UASRC =
UADEFS =

# ================================
# Include the Playdate SDK's common makefile
# ================================
include $(SDK)/C_API/buildsupport/common.mk

# Custom clean target to remove build artifacts
.PHONY: clean info run check-toolchain device-safe all-safe strip-pdx-source check-mingw windows windows-safe

default: simulator

# pdc packages the entire Source/ directory verbatim into $(PRODUCT) — it only has special
# handling for Lua scripts and known asset types, so Source/code rides along as plain files.
# None of that .c/.h source is read at runtime (only the compiled pdex.bin/pdex.so + assets
# are), so strip it back out of the built .pdx after every build.
strip-pdx-source:
	@if [ -d "$(PRODUCT)/code" ]; then \
		echo "Stripping C source from $(PRODUCT) (not needed at runtime)..."; \
		rm -rf "$(PRODUCT)/code"; \
	fi

# common.mk's own device/simulator/all/pdx targets are what actually invoke pdc — redeclared
# here (same prerequisites + pdc invocation as common.mk, just with strip-pdx-source appended)
# so that EVERY entry point that produces $(PRODUCT), including a bare `make device` or
# `make simulator`, always gets stripped — not just the safe/wrapper targets below.
device: device_bin
	$(PDC) $(PDCFLAGS) Source $(PRODUCT)
	@$(MAKE) --no-print-directory strip-pdx-source

simulator: simulator_bin
	$(PDC) $(PDCFLAGS) Source $(PRODUCT)
	@$(MAKE) --no-print-directory strip-pdx-source

all: device_bin simulator_bin
	$(PDC) $(PDCFLAGS) Source $(PRODUCT)
	@$(MAKE) --no-print-directory strip-pdx-source

pdx:
	$(PDC) $(PDCFLAGS) Source $(PRODUCT)
	@$(MAKE) --no-print-directory strip-pdx-source

# Cross-compiled Windows simulator plugin (pdex.dll), built from Linux via MinGW-w64 — the SDK's
# common.mk only builds a simulator lib for whatever OS `make` is run on (pdex.so here), so this
# fills the gap for friends testing on Windows. Copies alongside whatever's already in Source/
# (e.g. pdex.so, pdex.elf from prior builds), so the resulting $(PRODUCT) works on every platform
# that's been built for. There's no equivalent path for pdex.dylib (macOS) — that needs a real Mac.
MINGW_CC := x86_64-w64-mingw32-gcc

check-mingw:
	@if $(MINGW_CC) --version >/dev/null 2>&1; then \
		echo "OK: $(MINGW_CC) found"; \
	else \
		echo "Error: $(MINGW_CC) not available in this environment."; \
		echo "Arch host install: sudo pacman -S mingw-w64-gcc"; \
		exit 1; \
	fi

$(OBJDIR)/pdex.dll: $(SRC) | MKOBJDIR
	$(MINGW_CC) -g -shared -lm -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 $(INCDIR) -o $(OBJDIR)/pdex.dll $(SRC)

windows_bin: $(OBJDIR)/pdex.dll
	cp $(OBJDIR)/pdex.dll Source/

windows: windows_bin
	$(PDC) $(PDCFLAGS) Source $(PRODUCT)
	@$(MAKE) --no-print-directory strip-pdx-source

windows-safe: check-mingw windows

clean:
	@echo "Cleaning..."
	@rm -rf build dist
	@rm -rf *.pdx

info:
	@echo "Product: $(PRODUCT)"
	@echo "BundleID: $(BUNDLE_ID)"
	@echo "Version: $(VERSION)"
	@echo "SDK Path: $(SDK)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "SDK Version: $(SDK_VERSION)"

check-toolchain:
	@echo "Checking ARM toolchain..."
	@if arm-none-eabi-gcc --version >/dev/null 2>&1; then \
		echo "OK: arm-none-eabi-gcc found"; \
	else \
		echo "Error: arm-none-eabi-gcc not available in this environment."; \
		echo "Arch host install: sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib"; \
		exit 1; \
	fi
	@if arm-none-eabi-objcopy --version >/dev/null 2>&1; then \
		echo "OK: arm-none-eabi-objcopy found"; \
	else \
		echo "Error: arm-none-eabi-objcopy not available in this environment."; \
		echo "Arch host install: sudo pacman -S arm-none-eabi-binutils"; \
		exit 1; \
	fi

device-safe: check-toolchain device

all-safe: check-toolchain all

run: simulator
	@echo "Launching Playdate Simulator..."
	@if [ -f "$(SDK)/bin/PlaydateSimulator" ]; then \
		if [ -n "$$FLATPAK_ID" ] && command -v flatpak-spawn >/dev/null 2>&1; then \
			PLAYDATE_SDK_PATH="$(SDK)" flatpak-spawn --host "$(SDK)/bin/PlaydateSimulator" "$(abspath $(PRODUCT))"; \
		else \
			PLAYDATE_SDK_PATH="$(SDK)" "$(SDK)/bin/PlaydateSimulator" "$(abspath $(PRODUCT))"; \
		fi; \
	elif [ -f "$(SDK)/bin/Playdate Simulator.app/Contents/MacOS/Playdate Simulator" ]; then \
		PLAYDATE_SDK_PATH="$(SDK)" "$(SDK)/bin/Playdate Simulator.app/Contents/MacOS/Playdate Simulator" "$(abspath $(PRODUCT))"; \
	else \
		echo "Error: Playdate Simulator not found in SDK"; \
		exit 1; \
	fi

debug: $(MAKE) DEBUG=1 simulator

release: $(MAKE) DEBUG=0 simulator