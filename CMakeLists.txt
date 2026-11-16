cmake_minimum_required(VERSION 3.22)
project(Organix VERSION 0.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(Tests)
endif()

option(ORGANIX_BUILD_PLUGIN "Build the JUCE plugin" ON)
if (ORGANIX_BUILD_PLUGIN)
    if (DEFINED JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    set(ORGANIX_FORMATS VST3 Standalone)
    if (APPLE)
        list(APPEND ORGANIX_FORMATS AU)
    endif()
    juce_add_plugin(Organix
        COMPANY_NAME "Liberty Music Software"
        BUNDLE_ID "com.libertymusicsoftware.organix"
        PLUGIN_MANUFACTURER_CODE LbMs
        PLUGIN_CODE Orgx
        FORMATS ${ORGANIX_FORMATS}
        PRODUCT_NAME "Organix"
        COPY_PLUGIN_AFTER_BUILD OFF
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    )
    juce_generate_juce_header(Organix)
    target_sources(Organix PRIVATE
        Source/MaterialDSP.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
    )
    target_compile_definitions(Organix PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
    )
    target_link_libraries(Organix PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        PUBLIC juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
endif()
