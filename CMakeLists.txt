cmake_minimum_required(VERSION 3.22)

project(TapeStop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE acquisition
#
# Default: pull a pinned JUCE release via FetchContent (single source of truth,
# no submodules to keep in sync).
#
# Override: pass -DJUCE_PATH=/path/to/JUCE to use a local/vendored checkout
# instead. Useful in locked-down CI / containers where cloning from GitHub is
# blocked by network policy.
# ---------------------------------------------------------------------------
if(DEFINED JUCE_PATH)
    message(STATUS "TapeStop: using vendored JUCE at ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} juce_build)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(TapeStop
    COMPANY_NAME                "ZandersAudio"
    BUNDLE_ID                   com.zandersaudio.tapestop
    PLUGIN_MANUFACTURER_CODE    Zand
    PLUGIN_CODE                 Tstp
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Tape Stop"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Modulation
    AU_MAIN_TYPE                kAudioUnitType_Effect)

juce_generate_juce_header(TapeStop)

# Embed the editor's fonts (Space Grotesk + JetBrains Mono, OFL) as BinaryData.
juce_add_binary_data(TapeStopData SOURCES
    Fonts/SpaceGroteskRegular.ttf
    Fonts/SpaceGroteskSemiBold.ttf
    Fonts/SpaceGroteskBold.ttf
    Fonts/JetBrainsMonoRegular.ttf)

target_sources(TapeStop PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TapeStopLookAndFeel.cpp)

target_compile_definitions(TapeStop
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(TapeStop
    PRIVATE
        TapeStopData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Unit tests for the DSP core (TapeStopEngine). Headless and framework-free,
# so they run on every platform without a display. Run with: ctest --test-dir build
# ---------------------------------------------------------------------------
option(TAPESTOP_BUILD_TESTS "Build TapeStopEngine unit tests" ON)
option(TAPESTOP_ENABLE_COVERAGE "Instrument the unit tests for gcov/llvm coverage" OFF)

if(TAPESTOP_BUILD_TESTS)
    enable_testing()

    add_executable(TapeStopTests Tests/TapeStopEngineTests.cpp)
    target_include_directories(TapeStopTests PRIVATE Source)
    target_compile_definitions(TapeStopTests PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0)
    target_link_libraries(TapeStopTests
        PRIVATE
            juce::juce_audio_basics
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    # Coverage instrumentation for the DSP core, fed to SonarQube from CI. Only
    # the test target is instrumented, and -O0 keeps gcov's line mapping honest.
    if(TAPESTOP_ENABLE_COVERAGE)
        target_compile_options(TapeStopTests PRIVATE --coverage -O0 -g)
        target_link_options(TapeStopTests PRIVATE --coverage)
    endif()

    add_test(NAME TapeStopEngine COMMAND TapeStopTests)
endif()
