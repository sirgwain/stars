cmake_minimum_required(VERSION 3.23)
# The version this line is building toward. Tags (vX.Y.Z) and build numbers
# come from git; see cmake/version.cmake and docs/VERSIONING.md.
project(stars VERSION 2.9.0 LANGUAGES C)

# Windows builds the game (stars.exe) and stars-host.exe with MinGW or MSVC.
# Other platforms build only stars-host, the game's host without windows.
if(WIN32)
    if(NOT MINGW AND NOT MSVC)
        message(FATAL_ERROR "The Windows build requires MinGW or MSVC.")
    endif()
    enable_language(RC)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The game: rules, turn generation, AI and file I/O. A source file belongs
# here only if the command-line host needs it; Windows dialogs and drawing go
# in STARS_UI_SOURCES. Some of these files still hold UI functions; the 2.9
# split moves those out (docs/ROADMAP.md).
set(STARS_CORE_SOURCES
    ai.c ai2.c ai3.c ai4.c aiutil.c battle.c build.c create.c file.c globals.c
    log.c memory.c mine.c msg.c native.c parts.c planet.c produce.c race.c
    report.c research.c save.c ship.c ship2.c strings.c
    thing.c turn.c turn2.c turn3.c util.c utilgen.c
)

# The Windows game's user interface.
set(STARS_UI_SOURCES
    battleui.c buildui.c createui.c fileui.c globalsui.c init.c logui.c mdi.c
    mineui.c msgui.c nativeui.c planetui.c popup.c produceui.c raceui.c
    reportui.c researchui.c scan.c ship2ui.c shipui.c stars.c tb.c thingui.c
    tutor.c tutor2.c utilgenui.c utilui.c vcr.c
)

# Settings every target that compiles game code shares.
add_library(stars_options INTERFACE)

# Each set is built once for stars.exe and the unit tests.
add_library(stars_core OBJECT ${STARS_CORE_SOURCES})
target_link_libraries(stars_core PUBLIC stars_options)

# The command-line host: the game code and host.c, no windows.
add_executable(stars-host host.c hostui.c)
target_link_libraries(stars-host PRIVATE stars_core)
set_target_properties(stars-host PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# version.h is regenerated on every build so the build number follows HEAD;
# it is rewritten only when the version changes.
set(STARS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
# The platform label in the version shown (cmake/version.cmake): os-arch.
if(WIN32)
    set(STARS_PLATFORM_OS "windows")
elseif(APPLE)
    set(STARS_PLATFORM_OS "macos")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" STARS_PLATFORM_OS)
endif()
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(STARS_PLATFORM_ARCH "${CMAKE_OSX_ARCHITECTURES}")
else()
    set(STARS_PLATFORM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()
string(TOLOWER "${STARS_PLATFORM_ARCH}" STARS_PLATFORM_ARCH)
if(STARS_PLATFORM_ARCH MATCHES "^(x86_64|amd64)$")
    set(STARS_PLATFORM_ARCH "x64")
elseif(STARS_PLATFORM_ARCH MATCHES "^(arm64|aarch64)$")
    set(STARS_PLATFORM_ARCH "arm64")
elseif(STARS_PLATFORM_ARCH MATCHES ";")
    set(STARS_PLATFORM_ARCH "universal")
endif()
set(STARS_PLATFORM "${STARS_PLATFORM_OS}-${STARS_PLATFORM_ARCH}")

set(STARS_VERSION_COMMAND "${CMAKE_COMMAND}"
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DPLATFORM=${STARS_PLATFORM}
    -DOUTPUT=${STARS_GENERATED_DIR}/version.h
    -DVERSION_BASE=${PROJECT_VERSION}
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")
file(MAKE_DIRECTORY "${STARS_GENERATED_DIR}")
execute_process(COMMAND ${STARS_VERSION_COMMAND})
add_custom_target(stars_version
    COMMAND ${STARS_VERSION_COMMAND}
    BYPRODUCTS "${STARS_GENERATED_DIR}/version.h"
    COMMENT "Checking Stars! version"
    VERBATIM
)
add_dependencies(stars_core stars_version)

# The game's text: text/textgen.py builds the ID enums (textids.h) and the
# compressed string tables from the rows in text/*.txt.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(STARS_TEXT_SOURCES text/strings.txt text/messages.txt text/tutorial.txt text/planets.txt)
list(TRANSFORM STARS_TEXT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set(STARS_TEXT_OUTPUTS textids.h text_strings.c text_messages.c text_planets.c text_tutorial.c)
list(TRANSFORM STARS_TEXT_OUTPUTS PREPEND "${STARS_GENERATED_DIR}/")
add_custom_command(
    OUTPUT ${STARS_TEXT_OUTPUTS}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/text/textgen.py" "${STARS_GENERATED_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/text/textgen.py" ${STARS_TEXT_SOURCES}
    COMMENT "Building the text tables"
    VERBATIM
)
add_custom_target(stars_text DEPENDS ${STARS_TEXT_OUTPUTS})

# The data tables: data/datagen.py builds dataids.h (the hull, part,
# starting design and AI recipe enums) and data_tables.c (the parts, races,
# default battle plans and AI recipes) from data/**/*.yaml. It needs PyYAML.
execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import yaml" RESULT_VARIABLE STARS_PYYAML_MISSING OUTPUT_QUIET ERROR_QUIET)
if(STARS_PYYAML_MISSING)
    message(FATAL_ERROR "The build needs PyYAML for ${Python3_EXECUTABLE}. Install it (python3 -m pip install pyyaml), "
                        "or configure with -DPython3_EXECUTABLE=<a python that has it>.")
endif()
file(GLOB_RECURSE STARS_DATA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/*.yaml")
set(STARS_DATA_OUTPUTS "${STARS_GENERATED_DIR}/dataids.h" "${STARS_GENERATED_DIR}/data_tables.c")
add_custom_command(
    OUTPUT ${STARS_DATA_OUTPUTS}
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/data/datagen.py" "${STARS_GENERATED_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/datagen.py" ${STARS_DATA_SOURCES}
    COMMENT "Building the data tables"
    VERBATIM
)
add_custom_target(stars_data DEPENDS ${STARS_DATA_OUTPUTS})

# Everything that includes enums.h needs textids.h and dataids.h first.
add_dependencies(stars_options stars_text stars_data)
target_sources(stars_core PRIVATE
    "${STARS_GENERATED_DIR}/text_strings.c"
    "${STARS_GENERATED_DIR}/text_messages.c"
    "${STARS_GENERATED_DIR}/text_planets.c"
    "${STARS_GENERATED_DIR}/data_tables.c"
)

# The game's headers are found for #include "..." only (-iquote): strings.h
# is the game's string table, and glibc's <string.h> includes the system
# <strings.h>. MSVC has no -iquote or conflicting system strings.h; it and
# the resource compiler take ordinary include directories.
foreach(STARS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/res" "${STARS_GENERATED_DIR}")
    if(MSVC)
        target_include_directories(stars_options INTERFACE "${STARS_INCLUDE_DIR}")
    else()
        target_compile_options(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:C>:SHELL:-iquote \"${STARS_INCLUDE_DIR}\">")
        target_include_directories(stars_options INTERFACE "$<$<COMPILE_LANGUAGE:RC>:${STARS_INCLUDE_DIR}>")
    endif()
endforeach()

# Match the existing MinGW compile-analysis flags. Plain char is signed, as
# in the original MSC build; the decompiler relies on it to drop char casts.
if(MSVC)
    # MSVC's plain char is signed by default. Keep the existing CRT calls;
    # the _s alternatives are not portable replacements for their semantics.
    target_compile_definitions(stars_options INTERFACE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    target_compile_options(stars_options INTERFACE
        $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>
        $<$<COMPILE_LANGUAGE:C>:-fsigned-char>
    )
endif()

if(NOT WIN32)
    target_link_libraries(stars_options INTERFACE m)
endif()
if(NOT WIN32 OR MSVC)
    # Turn generation rounds through long double as the original's x87 code
    # did (docs/NATIVE-PORT.md). Without x87 extended precision (a 64-bit
    # mantissa), as on Apple silicon or other ARM, some turns differ from
    # the Windows game's.
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #include <float.h>
        #if LDBL_MANT_DIG != 64
        #error not x87 extended precision
        #endif
        int main(void) { return 0; }" STARS_X87_LONG_DOUBLE)
    if(NOT STARS_X87_LONG_DOUBLE)
        message(WARNING "long double is not x87 extended precision here, so generated turns can differ "
                        "from the MinGW game's. MSVC uses double precision for long double. "
                        "On Apple silicon, use the macos-host presets (x86_64, run by Rosetta).")
    endif()
endif()
if(NOT WIN32)
    # Unit tests (tests/unit) that need no windows, with hostui.c for the
    # user interface. The Windows build below runs them all under Wine.
    option(STARS_BUILD_TESTS "Build the unit tests in tests/unit" ON)
    if(STARS_BUILD_TESTS)
        enable_testing()
        set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
        file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
        configure_file(tests/scaffold/fixtures/newgame/tiny/humanoid.r1 "${STARS_TEST_DIR}/data/humanoid.r1" COPYONLY)
        file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.c")
        # test_native_ports also exercises Windows controls and UI state.
        list(REMOVE_ITEM STARS_UNIT_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_native_ports.c")
        foreach(STARS_UNIT_TEST ${STARS_UNIT_TESTS})
            get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
            add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.c hostui.c)
            target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_core)
            target_include_directories(${STARS_TEST_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
            target_compile_definitions(${STARS_TEST_NAME} PRIVATE STARS_TEST_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/golden")
            set_target_properties(${STARS_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
            add_test(NAME ${STARS_TEST_NAME} COMMAND ${STARS_TEST_NAME})
            set_tests_properties(${STARS_TEST_NAME} PROPERTIES WORKING_DIRECTORY "${STARS_TEST_DIR}")
        endforeach()
    endif()
    # The rest builds the Windows game and its tests.
    return()
endif()

add_library(stars_ui OBJECT ${STARS_UI_SOURCES} "${STARS_GENERATED_DIR}/text_tutorial.c")
target_link_libraries(stars_ui PUBLIC stars_options)
target_link_libraries(stars_ui PUBLIC user32 gdi32 comdlg32)
add_dependencies(stars_ui stars_version)

add_executable(stars WIN32 res/stars.rc)
target_link_libraries(stars PRIVATE stars_core stars_ui)
set_target_properties(stars PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# MinGW runtime objects can contain debug data even in a Release build.
if(MINGW)
    target_link_options(stars PRIVATE
        $<$<CONFIG:Release>:LINKER:--strip-debug>
    )
endif()

# Portable test replacements preserve --wrap's cross-file semantics: only the
# source defining a hook renames it (including its internal calls). Other
# objects call the replacement supplied by the harness. Product objects stay
# unchanged, and all Windows compilers exercise the same test hooks.
include(cmake/test-hooks.cmake)
option(STARS_BUILD_TESTS "Build the unit tests in tests/unit" ON)
option(STARS_TEST_TUTORIAL "Expose read-only tutorial state for AutoHotkey tests" OFF)
option(STARS_TEST_TRACE "Log Random and other selected calls to $STARS_TRACE for regression tests" OFF)

set(STARS_GAME_HOOKS)
if(STARS_TEST_TUTORIAL)
    list(APPEND STARS_GAME_HOOKS InitInstance ScannerWndProc CchGetString)
    target_sources(stars PRIVATE tests/scaffold/tutorial/observer.c)
endif()
if(STARS_TEST_TRACE)
    list(APPEND STARS_GAME_HOOKS Random PctPlanetCapacity)
    target_sources(stars PRIVATE tests/scaffold/regression_trace.c)
endif()
if(STARS_GAME_HOOKS)
    stars_test_variant(stars_game_core stars_core ${STARS_GAME_HOOKS})
    stars_test_variant(stars_game_ui stars_ui ${STARS_GAME_HOOKS})
    set_property(TARGET stars PROPERTY LINK_LIBRARIES stars_game_core stars_game_ui)
    stars_test_hooks(stars ${STARS_GAME_HOOKS})
endif()

# Recompile resources when an embedded bitmap, icon, cursor, or data file changes.
file(GLOB_RECURSE STARS_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/res/*"
)
list(REMOVE_ITEM STARS_RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/res/stars.rc"
)
set_source_files_properties(res/stars.rc PROPERTIES
    OBJECT_DEPENDS "${STARS_RESOURCE_FILES};${STARS_GENERATED_DIR}/version.h"
)

# Wine is needed only to run the Windows executable on the build host.
find_program(WINE_EXECUTABLE NAMES wine)
if(WINE_EXECUTABLE)
    set_property(TARGET stars PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
    add_custom_target(run_in_wine
        COMMAND "${WINE_EXECUTABLE}" "$<TARGET_FILE:stars>"
        DEPENDS stars
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        USES_TERMINAL
        VERBATIM
    )
else()
    message(STATUS "Wine not found; stars.exe can still be built, but run_in_wine is unavailable")
endif()

# Unit tests (tests/unit): one acutest executable per test_*.c, linked with
# the game code and its resources. They are Windows console programs; CTest
# runs them through Wine when it is available.
if(STARS_BUILD_TESTS)
    enable_testing()
    set(STARS_TEST_DIR "${CMAKE_BINARY_DIR}/tests")
    file(MAKE_DIRECTORY "${STARS_TEST_DIR}/data")
    configure_file(tests/scaffold/fixtures/newgame/tiny/humanoid.r1 "${STARS_TEST_DIR}/data/humanoid.r1" COPYONLY)
    file(GLOB STARS_UNIT_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_*.c")
    stars_test_variant(stars_unit_core stars_core AlertSz)
    stars_test_variant(stars_ports_core stars_core AlertSz WriteRt ReadRt FCreateFile StreamOpen StreamClose DirtyGame)
    foreach(STARS_UNIT_TEST ${STARS_UNIT_TESTS})
        get_filename_component(STARS_TEST_NAME "${STARS_UNIT_TEST}" NAME_WE)
        add_executable(${STARS_TEST_NAME} "${STARS_UNIT_TEST}" tests/unit/stars_test.c res/stars.rc)
        if(STARS_TEST_NAME STREQUAL "test_native_ports")
            target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_ports_core stars_ui)
            stars_test_hooks(${STARS_TEST_NAME} WriteRt ReadRt FCreateFile StreamOpen StreamClose DirtyGame)
        else()
            target_link_libraries(${STARS_TEST_NAME} PRIVATE stars_unit_core stars_ui)
        endif()
        # Message boxes would wait for a click; stars_test.c records them.
        stars_test_hooks(${STARS_TEST_NAME} AlertSz)
        target_include_directories(${STARS_TEST_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
        target_compile_definitions(${STARS_TEST_NAME} PRIVATE STARS_TEST_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/golden")
        set_target_properties(${STARS_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
        if(WINE_EXECUTABLE)
            set_property(TARGET ${STARS_TEST_NAME} PROPERTY CROSSCOMPILING_EMULATOR "${WINE_EXECUTABLE}")
        endif()
        add_test(NAME ${STARS_TEST_NAME} COMMAND ${STARS_TEST_NAME})
        set_tests_properties(${STARS_TEST_NAME} PROPERTIES
            WORKING_DIRECTORY "${STARS_TEST_DIR}"
            ENVIRONMENT "WINEDEBUG=-all")
    endforeach()
    # Games to open by hand (tests/scenarios, make scenario).
    add_executable(stars_scenario tests/scenarios/scenarios.c tests/unit/stars_test.c res/stars.rc)
    target_link_libraries(stars_scenario PRIVATE stars_unit_core stars_ui)
    stars_test_hooks(stars_scenario AlertSz)
    target_include_directories(stars_scenario PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit")
    set_target_properties(stars_scenario PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${STARS_TEST_DIR}")
endif()
