.DEFAULT_GOAL := help
.PHONY: help all version-header text-header data-header text-unreferenced save-cli compile host test-unit test-host scenario run-wine fmt compile-check res resources regression regression-quick regression-host regression-export tutorial tutorial-reject clean

DIST_DIR    ?= dist
CMAKE       ?= cmake
CLANG_FORMAT ?= clang-format
PYTHON      ?= python3
GO          ?= go
ifeq ($(OS),Windows_NT)
EXE_SUFFIX := .exe
endif
SAVE_CLI    := $(abspath $(DIST_DIR))/stars-save$(EXE_SUFFIX)
MINGW_CC    ?= x86_64-w64-mingw32-gcc
MINGW_RC    ?= x86_64-w64-mingw32-windres
FILES       ?= $(wildcard *.c)
FORMAT_FILES ?= $(wildcard *.c *.h res/*.h tests/scaffold/*.c tests/scaffold/*.h tests/scaffold/tutorial/*.c tests/scaffold/tutorial/*.h)
SEED        ?= 12345
# The native regression: a release build, its fixed-seed run, and the
# checked-in baseline it is compared against (tests/scaffold/REGRESSION.md).
REGRESSION_BUILD := $(DIST_DIR)/regression-build
REGRESSION_WORK  ?= $(DIST_DIR)/scaffold/regression/native
REGRESSION_REPORT := $(DIST_DIR)/scaffold/regression/comparison.json
BASELINE_DIR     := tests/scaffold/fixtures/regression/native
# Every scenario in the baseline, unless SCENARIOS names some.
SCENARIOS ?= $(shell $(PYTHON) -c "import json; print(' '.join(json.load(open('$(BASELINE_DIR)/run.json'))['scenarios']))")
THROUGH   ?= 150
REGRESSION_ARGS = $(foreach s,$(SCENARIOS),--scenario $(s)) --through $(THROUGH)
# stars-host with the native compiler. Apple silicon builds it for x86_64
# (run by Rosetta) to keep x87 rounding; see docs/NATIVE-PORT.md.
ifeq ($(shell uname -sm),Darwin arm64)
HOST_PRESET ?= macos-host-release
else
HOST_PRESET ?= host-release
endif
HOST_WORK   ?= $(DIST_DIR)/scaffold/regression/host

help:
	@echo "Targets:"
	@echo "  save-cli             Build the standalone test save CLI"
	@echo "  compile              Build stars.exe with the MinGW CMake preset"
	@echo "  host                 Build stars-host with the native compiler (HOST_PRESET=$(HOST_PRESET))"
	@echo "  test-unit            Build and run the unit tests in tests/unit under Wine"
	@echo "  test-host            Build and run the unit tests that need no windows, natively"
	@echo "  scenario             Build a test game into dist/scenarios/SCENARIO (no SCENARIO: list them)"
	@echo "  fmt                  Format C sources and headers (FORMAT_FILES=ai.c to limit)"
	@echo "  compile-check        Check C syntax (FILES=ai.c to limit) and resources"
	@echo "  text-unreferenced    List text rows that no code names and no @range covers"
	@echo "  res / resources      Compile res/stars.rc into $(DIST_DIR)/stars_res.o"
	@echo "  regression           Run the native regression and compare it with the baseline"
	@echo "                       (SCENARIOS=\"noai smallai4\" and THROUGH=10 to limit)"
	@echo "  regression-quick     Run smallai4 through turn 10 and compare"
	@echo "  regression-host      Run the native regression with stars-host instead of stars.exe"
	@echo "  regression-export    Replace the baseline with the last full regression run"
	@echo "  tutorial             Run the complete AutoHotkey v2 tutorial under Wine"
	@echo "  tutorial-reject      Verify early Generate is rejected"
	@echo "  clean                Remove $(DIST_DIR)/"

all: compile

save-cli:
	@mkdir -p "$(DIST_DIR)"
	cd tests/savecli && $(GO) build -o "$(SAVE_CLI)" .

compile:
	$(CMAKE) --preset mingw-debug
	$(CMAKE) --build --preset mingw-debug

test-unit: compile
	cd "$(DIST_DIR)/mingw-debug" && ctest --output-on-failure --timeout 300 $(CTEST_ARGS)

# The unit tests that need no windows, built with the host preset and run
# without Wine.
test-host: host
	cd "$(DIST_DIR)/$(HOST_PRESET)" && ctest --output-on-failure --timeout 300 $(CTEST_ARGS)

# SCENARIO names one of the games in tests/scenarios/scenarios.c.
SCENARIO_DIR = $(abspath $(DIST_DIR))/scenarios/$(SCENARIO)
scenario: compile
ifeq ($(SCENARIO),)
	cd "$(DIST_DIR)/mingw-debug/tests" && WINEDEBUG=-all wine ./stars_scenario.exe
else
	rm -rf "$(SCENARIO_DIR)"
	mkdir -p "$(SCENARIO_DIR)"
	cd "$(DIST_DIR)/mingw-debug/tests" && WINEDEBUG=-all wine ./stars_scenario.exe "$(SCENARIO)" "$(SCENARIO_DIR)"
endif

run-wine:
	$(CMAKE) --preset mingw-debug
	$(CMAKE) --build --preset run-wine

fmt:
	$(CLANG_FORMAT) --style=file -i $(FORMAT_FILES)

# Direct compiles need the generated version.h, text and data tables that
# CMake builds normally make.
VERSION_DIR := $(abspath $(DIST_DIR))/generated
text-header:
	@$(PYTHON) text/textgen.py "$(VERSION_DIR)"

data-header:
	@$(PYTHON) data/datagen.py "$(VERSION_DIR)"

text-unreferenced:
	@$(PYTHON) text/textgen.py --unreferenced

version-header:
	@mkdir -p "$(VERSION_DIR)"
	@$(CMAKE) -DSOURCE_DIR="$(CURDIR)" -DOUTPUT="$(VERSION_DIR)/version.h" \
		-DVERSION_BASE=$$(sed -n 's/^project(stars VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt) \
		-P cmake/version.cmake

compile-check: version-header text-header data-header
	@set -e; for f in $(FILES); do \
		$(MINGW_CC) -std=gnu11 -fsigned-char -fsyntax-only -fmax-errors=0 -Wno-pointer-sign -I. -I"$(VERSION_DIR)" "$$f"; \
	done
	cd res && $(MINGW_RC) -I"$(VERSION_DIR)" stars.rc -O res -o /dev/null

res: resources

resources: version-header
	@mkdir -p "$(DIST_DIR)"
	cd res && $(MINGW_RC) -I"$(VERSION_DIR)" stars.rc -O coff -o "$(abspath $(DIST_DIR))/stars_res.o"

# Builds the release exe as CI does, runs a fresh work directory with
# stars.exe -s$(SEED) and compares it with the baseline. A difference exits
# nonzero.
regression: save-cli
	$(CMAKE) --preset mingw-release -B "$(REGRESSION_BUILD)"
	$(CMAKE) --build "$(REGRESSION_BUILD)"
	rm -rf "$(REGRESSION_WORK)"
	$(PYTHON) tests/scaffold/regression.py prepare --seed $(SEED) --exe "$(REGRESSION_BUILD)/bin/stars.exe" --work "$(REGRESSION_WORK)"
	$(PYTHON) tests/scaffold/regression.py run --cli "$(SAVE_CLI)" --work "$(REGRESSION_WORK)" $(REGRESSION_ARGS)
	$(PYTHON) tests/scaffold/regression.py compare "$(BASELINE_DIR)" "$(REGRESSION_WORK)" $(REGRESSION_ARGS) --report "$(REGRESSION_REPORT)"

regression-quick:
	$(MAKE) regression SCENARIOS=smallai4 THROUGH=10

host:
	$(CMAKE) --preset $(HOST_PRESET)
	$(CMAKE) --build --preset $(HOST_PRESET)

# The same run and baseline as `make regression`, through stars-host and
# without Wine.
regression-host: save-cli host
	rm -rf "$(HOST_WORK)"
	$(PYTHON) tests/scaffold/regression.py prepare --seed $(SEED) --exe "$(DIST_DIR)/$(HOST_PRESET)/bin/stars-host$(EXE_SUFFIX)" --work "$(HOST_WORK)"
	$(PYTHON) tests/scaffold/regression.py run --cli "$(SAVE_CLI)" --work "$(HOST_WORK)" $(REGRESSION_ARGS)
	$(PYTHON) tests/scaffold/regression.py compare "$(BASELINE_DIR)" "$(HOST_WORK)" $(REGRESSION_ARGS) --report "$(DIST_DIR)/scaffold/regression/host-comparison.json"

# For a commit that changes behavior on purpose: after a full `make
# regression`, replace the baseline with that run.
regression-export:
	$(PYTHON) tests/scaffold/regression.py export --work "$(REGRESSION_WORK)" $(foreach s,$(SCENARIOS),--scenario $(s)) --replace

tutorial:
	$(PYTHON) tests/scaffold/tutorial/run.py --download-ahk $(TUTORIAL_ARGS)

tutorial-reject:
	$(PYTHON) tests/scaffold/tutorial/run.py --download-ahk --scenario reject-generate $(TUTORIAL_ARGS)

clean:
	rm -rf "$(DIST_DIR)"
