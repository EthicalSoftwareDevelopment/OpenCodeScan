cmake_minimum_required(VERSION 3.24)
project(OpenCodeScan VERSION 0.1.0 LANGUAGES CXX)

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
        REQUIRED)

enable_testing()

set(OPEN_CODE_SCAN_SOURCES
        src/app/main.cpp
        src/app/ApplicationBootstrap.hpp
        src/app/ApplicationBootstrap.cpp
        src/analysis/MockAnalysisEngine.hpp
        src/analysis/MockAnalysisEngine.cpp
        src/core/Diagnostic.hpp
        src/core/IAnalysisEngine.hpp
        src/core/IDiagnosticsModel.hpp
        src/core/IReportExporter.hpp
        src/core/IAiAdvisor.hpp
        src/core/IHintProvider.hpp
        src/core/Logging.hpp
        src/core/Logging.cpp
        src/core/SettingsService.hpp
        src/core/SettingsService.cpp
        src/ui/DiagnosticsTableModel.hpp
        src/ui/DiagnosticsTableModel.cpp
        src/ui/MainWindow.hpp
        src/ui/MainWindow.cpp
)

qt_add_executable(OpenCodeScan MANUAL_FINALIZATION ${OPEN_CODE_SCAN_SOURCES})

target_include_directories(OpenCodeScan PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(OpenCodeScan PRIVATE cxx_std_23)

target_link_libraries(OpenCodeScan PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

set(OPEN_CODE_SCAN_TEST_SOURCES
        tests/unit/Phase1Tests.cpp
        src/analysis/MockAnalysisEngine.cpp
        src/core/SettingsService.cpp
        src/ui/DiagnosticsTableModel.cpp
)

add_executable(OpenCodeScanPhase1Tests ${OPEN_CODE_SCAN_TEST_SOURCES})

target_include_directories(OpenCodeScanPhase1Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(OpenCodeScanPhase1Tests PRIVATE cxx_std_23)

target_link_libraries(OpenCodeScanPhase1Tests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

add_test(NAME OpenCodeScanPhase1Tests COMMAND OpenCodeScanPhase1Tests)

if (WIN32 AND DEFINED Qt6_DIR AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()

    get_filename_component(QT_INSTALL_PATH "${Qt6_DIR}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_INSTALL_PATH}" DIRECTORY)

    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)

endif ()

qt_finalize_executable(OpenCodeScan)

