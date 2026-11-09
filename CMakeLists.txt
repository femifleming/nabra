cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")

project(Nabra VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local checkout in ./JUCE if present, otherwise download the pinned release.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Copy plug-ins into the system plug-in folders after building (handy locally; CI turns it off).
option(NABRA_COPY_PLUGINS "Copy built plug-ins to the user plug-in folders" ON)

if(APPLE)
    set(NABRA_FORMATS AU VST3 Standalone)
elseif(UNIX)
    set(NABRA_FORMATS VST3 LV2 Standalone)
else()
    set(NABRA_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Nabra
    VERSION 1.0.0
    COMPANY_NAME "Tapestry"
    BUNDLE_ID "com.tapestry.nabra"
    PLUGIN_MANUFACTURER_CODE Tpst
    PLUGIN_CODE Nbra
    FORMATS ${NABRA_FORMATS}
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon/nabra-icon.png"
    LV2URI "https://github.com/femifleming/nabra"
    PRODUCT_NAME "Nabra"
    DESCRIPTION "a knobless tapestry modular: 52 modules + MIDI-to-CV, patched with alligator cords"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${NABRA_COPY_PLUGINS})

file(GLOB NABRA_PHOTOS   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/photos/*.jpg")
file(GLOB NABRA_DIAGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/diagrams/*.png")
file(GLOB NABRA_BOARDS   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/boards/*.jpg")
juce_add_binary_data(NabraArt HEADER_NAME NabraArt.h NAMESPACE NabraArt
    SOURCES ${NABRA_PHOTOS} ${NABRA_DIAGRAMS} ${NABRA_BOARDS})

set(NABRA_SOURCES
    Source/Specs.cpp
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/Art.cpp
    Source/Help.cpp
    Source/PluginEditor.cpp)

target_sources(Nabra PRIVATE ${NABRA_SOURCES})

target_compile_definitions(Nabra PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Nabra
    PRIVATE
        NabraArt
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# offline render test (no GUI): renders the recipes to WAV and checks for sanity
option(NABRA_BUILD_TESTS "Build offline test" ON)
if(MSVC)
    target_compile_options(Nabra PRIVATE /utf-8)
endif()
if(NABRA_BUILD_TESTS)
    juce_add_console_app(NabraTest PRODUCT_NAME "NabraTest")
    target_sources(NabraTest PRIVATE tests/OfflineTest.cpp Source/Specs.cpp Source/Engine.cpp)
    target_compile_definitions(NabraTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(NabraTest PRIVATE juce::juce_dsp juce::juce_audio_formats
        PUBLIC juce::juce_recommended_config_flags)
    if(APPLE)
        set_target_properties(NabraTest PROPERTIES OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
endif()

if(NABRA_BUILD_TESTS)
    juce_add_console_app(NabraSnapshot PRODUCT_NAME "NabraSnapshot")
    target_sources(NabraSnapshot PRIVATE tests/Snapshot.cpp ${NABRA_SOURCES})
    target_compile_definitions(NabraSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(NabraSnapshot PRIVATE NabraArt juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
    if(APPLE)
        set_target_properties(NabraSnapshot PROPERTIES OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
endif()
