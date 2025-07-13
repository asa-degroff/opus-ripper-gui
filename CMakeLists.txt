cmake_minimum_required(VERSION 3.16)
project(OpusRipperGUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find required Qt6 components
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Quick Concurrent QuickControls2)

# Find external libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)
pkg_check_modules(OGG REQUIRED ogg)
pkg_check_modules(FLAC REQUIRED flac++)
pkg_check_modules(SAMPLERATE REQUIRED samplerate)
find_package(Threads REQUIRED)

# Try to find TagLib
find_path(TAGLIB_INCLUDE_DIR taglib/tag.h)
find_library(TAGLIB_LIBRARY tag)

if(NOT TAGLIB_INCLUDE_DIR OR NOT TAGLIB_LIBRARY)
    message(FATAL_ERROR "TagLib not found. Please install taglib development packages.")
endif()

qt_standard_project_setup(REQUIRES 6.5)

# Set Qt policy for QML directories (this causes issues)
# qt_policy(SET QTP0004 NEW)

# Add executable
qt_add_executable(opus-ripper-gui
    src/main.cpp
)

# Add QML module
qt_add_qml_module(opus-ripper-gui
    URI OpusRipperGUI
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/DirectoryPicker.qml
        qml/components/ProgressView.qml
        qml/components/SettingsPanel.qml
        qml/components/StyledGroupBox.qml
        qml/theme/Style.qml
)

# Core library
add_library(OpusRipperCore STATIC
    src/core/AudioConverter.cpp
    src/core/AudioConverter.h
    src/core/OpusEncoder.cpp
    src/core/OpusEncoder.h
    src/core/MetadataHandler.cpp
    src/core/MetadataHandler.h
    src/core/FileScanner.cpp
    src/core/FileScanner.h
    src/models/ConversionModel.cpp
    src/models/ConversionModel.h
    src/models/ProgressModel.cpp
    src/models/ProgressModel.h
    src/controllers/ConversionController.cpp
    src/controllers/ConversionController.h
)

target_include_directories(OpusRipperCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPUS_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
    ${FLAC_INCLUDE_DIRS}
    ${SAMPLERATE_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIR}
)

target_link_libraries(OpusRipperCore PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Concurrent
    ${OPUS_LIBRARIES}
    ${OGG_LIBRARIES}
    ${FLAC_LIBRARIES}
    ${SAMPLERATE_LIBRARIES}
    ${TAGLIB_LIBRARY}
    Threads::Threads
)

target_compile_options(OpusRipperCore PUBLIC
    ${OPUS_CFLAGS_OTHER}
    ${FLAC_CFLAGS_OTHER}
)

# Link executable
target_link_libraries(opus-ripper-gui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    OpusRipperCore
)

# Installation
install(TARGETS opus-ripper-gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)