cmake_minimum_required(VERSION 3.22)

project(MidSide670 VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Never bake the local dependency sysroot into distributed binaries.
set(CMAKE_SKIP_RPATH TRUE)

option(MIDSIDE670_BUILD_TESTS "Build the offline DSP test executable" ON)
option(MIDSIDE670_BUILD_SCREENSHOT "Build the headless editor renderer" ON)
option(MIDSIDE670_BUILD_HOST_TEST "Build the VST3 host smoke test" ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(MidSide670Assets
    SOURCES
        Resources/LatoRegular.ttf
        Resources/LatoSemibold.ttf
        Resources/LatoBlack.ttf
        Resources/Lato-OFL.txt)

set(MIDSIDE670_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.h
    Source/Presets.h
    Source/dsp/VariableMu.h
    Source/ui/Theme.cpp
    Source/ui/Theme.h
    Source/ui/StudioLookAndFeel.cpp
    Source/ui/StudioLookAndFeel.h
    Source/ui/Controls.cpp
    Source/ui/Controls.h
    Source/ui/WaveformDisplay.cpp
    Source/ui/WaveformDisplay.h)

set(MIDSIDE670_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_add_plugin(MidSide670
    COMPANY_NAME "BLS Audio"
    BUNDLE_ID "com.blsaudio.midside670"
    PLUGIN_MANUFACTURER_CODE Blsa
    PLUGIN_CODE Ms67
    FORMATS VST3 Standalone
    PRODUCT_NAME "MidSide-670"
    DESCRIPTION "Variable-mu tube compressor with true Mid/Side dynamics"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx")

target_sources(MidSide670 PRIVATE ${MIDSIDE670_SOURCES})
target_include_directories(MidSide670 PRIVATE Source)
target_compile_definitions(MidSide670 PUBLIC ${MIDSIDE670_DEFINITIONS})
target_link_libraries(MidSide670
    PRIVATE
        MidSide670Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(MIDSIDE670_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(MidSide670Tests PRODUCT_NAME "MidSide670Tests")
    target_sources(MidSide670Tests PRIVATE Tests/DspTests.cpp ${MIDSIDE670_SOURCES})
    target_include_directories(MidSide670Tests PRIVATE Source)
    target_compile_definitions(MidSide670Tests PRIVATE ${MIDSIDE670_DEFINITIONS})
    target_link_libraries(MidSide670Tests
        PRIVATE
            MidSide670Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME midside670_dsp COMMAND $<TARGET_FILE:MidSide670Tests>)
endif()

if(MIDSIDE670_BUILD_HOST_TEST)
    juce_add_console_app(MidSide670HostSmokeTest PRODUCT_NAME "MidSide670HostSmokeTest")

    target_sources(MidSide670HostSmokeTest PRIVATE Tools/HostSmokeTest.cpp)
    target_compile_definitions(MidSide670HostSmokeTest PRIVATE
        ${MIDSIDE670_DEFINITIONS}
        JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(MidSide670HostSmokeTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_gui_extra
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_dependencies(MidSide670HostSmokeTest MidSide670_VST3)
endif()

if(MIDSIDE670_BUILD_SCREENSHOT)
    juce_add_gui_app(MidSide670Screenshot
        PRODUCT_NAME "MidSide-670 Screenshot"
        COMPANY_NAME "BLS Audio"
        BUNDLE_ID "com.blsaudio.midside670.screenshot")

    target_sources(MidSide670Screenshot PRIVATE ${MIDSIDE670_SOURCES} Tools/RenderEditor.cpp)
    target_include_directories(MidSide670Screenshot PRIVATE Source)
    target_compile_definitions(MidSide670Screenshot PRIVATE ${MIDSIDE670_DEFINITIONS})
    target_link_libraries(MidSide670Screenshot
        PRIVATE
            MidSide670Assets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
