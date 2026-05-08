# Convenience wrapper around the CMake build.
#
#   make            — configure (if needed) and build every plugin in Release
#   make install    — copy every built .vst3 bundle to $(VST3_INSTALL_DIR)
#   make clean      — wipe the build tree
#
# Override the build directory, build type or install location from the
# command line, e.g.
#
#   make CMAKE_BUILD_TYPE=Debug
#   make install VST3_INSTALL_DIR=/usr/local/lib/vst3

BUILD_DIR        ?= build
CMAKE_BUILD_TYPE ?= Release
VST3_INSTALL_DIR ?= $(HOME)/.vst3
JOBS             ?= 4

VST3_GLOB := $(BUILD_DIR)/Source/*/Fxme*_artefacts/$(CMAKE_BUILD_TYPE)/VST3/*.vst3

.PHONY: all configure build install clean

all: build

# CMakeCache.txt is the configuration's marker — only re-run cmake when it's
# missing so repeated `make` invocations stay fast.
$(BUILD_DIR)/CMakeCache.txt:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

configure: $(BUILD_DIR)/CMakeCache.txt

build: configure
	cmake --build $(BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --parallel $(JOBS)

install: build
	@mkdir -p "$(VST3_INSTALL_DIR)"
	@found=0; \
	for vst in $(VST3_GLOB); do \
	    [ -e "$$vst" ] || continue; \
	    found=1; \
	    name=$$(basename "$$vst"); \
	    echo "  install  $$name  →  $(VST3_INSTALL_DIR)/$$name"; \
	    rm -rf "$(VST3_INSTALL_DIR)/$$name"; \
	    cp -R "$$vst" "$(VST3_INSTALL_DIR)/"; \
	done; \
	if [ $$found -eq 0 ]; then \
	    echo "No .vst3 bundles found under $(BUILD_DIR). Run 'make' first."; \
	    exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
