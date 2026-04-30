cmake_minimum_required(VERSION 3.24)
project(OpenCodeScan VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)
if (DEFINED ENV{CMAKE_PREFIX_PATH} AND NOT "$ENV{CMAKE_PREFIX_PATH}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH})
endif ()
if (DEFINED ENV{Qt6_DIR} AND NOT "$ENV{Qt6_DIR}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
endif ()
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Concurrent
        REQUIRED)
enable_testing()
set(OPEN_CODE_SCAN_CORE_SOURCES
        src/app/ApplicationBootstrap.hpp
        src/app/ApplicationBootstrap.cpp
        src/analysis/CompileCommandsLoader.hpp
        src/analysis/CompileCommandsLoader.cpp
        src/analysis/DeterministicAnalysisEngine.hpp
        src/analysis/DeterministicAnalysisEngine.cpp
        src/analysis/ProjectFileDiscoverer.hpp
        src/analysis/ProjectFileDiscoverer.cpp
        src/core/Diagnostic.hpp
        src/core/IAnalysisEngine.hpp
        src/core/Logging.hpp
        src/core/Logging.cpp
        src/core/SettingsService.hpp
        src/core/SettingsService.cpp
        src/ui/DiagnosticsTableModel.hpp
        src/ui/DiagnosticsTableModel.cpp
        src/ui/MainWindow.hpp
        src/ui/MainWindow.cpp
)
add_library(OpenCodeScanCore STATIC ${OPEN_CODE_SCAN_CORE_SOURCES})
target_include_directories(OpenCodeScanCore PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(OpenCodeScanCore PUBLIC cxx_std_23)
target_link_libraries(OpenCodeScanCore PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Concurrent
)
qt_add_executable(OpenCodeScan MANUAL_FINALIZATION src/app/main.cpp)
target_link_libraries(OpenCodeScan PRIVATE OpenCodeScanCore)
add_executable(OpenCodeScanTests tests/unit/Phase1Tests.cpp)
target_include_directories(OpenCodeScanTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(OpenCodeScanTests PRIVATE cxx_std_23)
target_link_libraries(OpenCodeScanTests PRIVATE OpenCodeScanCore)
add_test(NAME OpenCodeScanTests COMMAND OpenCodeScanTests)
if (WIN32 AND DEFINED Qt6_DIR AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    get_filename_component(QT_INSTALL_PATH "${Qt6_DIR}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET OpenCodeScan POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:OpenCodeScan>/plugins/platforms/")
        add_custom_command(TARGET OpenCodeScan POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:OpenCodeScan>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Concurrent)
        add_custom_command(TARGET OpenCodeScan POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:OpenCodeScan>")
    endforeach ()
endif ()
qt_finalize_executable(OpenCodeScan)
