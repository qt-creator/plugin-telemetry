cmake_minimum_required(VERSION 3.10)

project(plugin-telemetry)
find_package(Qt6 COMPONENTS Core Widgets QuickWidgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

# pre-configure and install extra-cmake-modules (ECM) to ensure module availability for
# kuserfeedback configuration
# with ExternalProject_Add it would install it during build, which is too late
set(ECM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/extra-cmake-modules")
set(ECM_BUILD_DIR "${CMAKE_BINARY_DIR}/ecm_build")
set(ECM_INSTALL_DIR "${CMAKE_BINARY_DIR}/ecm_install")

if(NOT EXISTS "${ECM_BUILD_DIR}/ECMConfig.cmake")
    file(MAKE_DIRECTORY ${ECM_BUILD_DIR})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_INSTALL_PREFIX:PATH=${ECM_INSTALL_DIR} ${ECM_SOURCE_DIR}
        WORKING_DIRECTORY ${ECM_BUILD_DIR}
        RESULT_VARIABLE result
    )
    if(result EQUAL "0")
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build . --target install
            WORKING_DIRECTORY ${ECM_BUILD_DIR}
            RESULT_VARIABLE result
        )
    endif()
endif()

list(APPEND CMAKE_PREFIX_PATH ${ECM_INSTALL_DIR})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(ENABLE_SURVEY_TARGET_EXPRESSIONS OFF CACHE BOOL "Enable survey target expressions" FORCE)
set(ENABLE_PHP OFF CACHE BOOL "Enable PHP" FORCE)
set(ENABLE_PHP_UNIT OFF CACHE BOOL "Enable PHP unit" FORCE)
set(ENABLE_DOCS OFF CACHE BOOL "Enable documentation" FORCE)
set(ENABLE_CONSOLE OFF CACHE BOOL "Enable console" FORCE)
set(ENABLE_CLI OFF CACHE BOOL "Enable CLI" FORCE)
set(ENABLE_QML OFF CACHE BOOL "Enable QML plugin" FORCE)
set(KDE_INSTALL_LIBDIR lib CACHE STRING "Lib install dir" FORCE)

#avoid install pollution
get_filename_component(CURRENT_BUILD_DIR_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
get_filename_component(KDE_TEMP_INSTALL_DIR "${CMAKE_BINARY_DIR}/../${CURRENT_BUILD_DIR_NAME}_temp_kde_install" REALPATH)
set(KDE_INSTALL_LIBDIR "${KDE_TEMP_INSTALL_DIR}/lib")
set(KDE_INSTALL_BINDIR "${KDE_TEMP_INSTALL_DIR}/bin")
set(KDE_INSTALL_INCLUDEDIR "${KDE_TEMP_INSTALL_DIR}/include")
set(KDE_INSTALL_LOGGINGCATEGORIESDIR "${KDE_TEMP_INSTALL_DIR}/qlogging-categories5")

#overwrite internal used variable to not get the unnecessary mkspecs/modules content
set(ECM_MKSPECS_INSTALL_DIR "${KDE_TEMP_INSTALL_DIR}/mkspecs/modules" CACHE PATH "Custom mkspecs install directory" FORCE)

# we do not install it so we can not use #include <KUserFeedback/*>
# if we want to keep that includes we would need similar approach like ECMConfig
add_subdirectory(3rdparty/kuserfeedback)

target_include_directories(KUserFeedbackCore PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/kuserfeedback
  ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/kuserfeedback
)

target_include_directories(KUserFeedbackCommon PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/kuserfeedback
  ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/kuserfeedback
)

# TODO: should we fix this in kuserfeedback?
if(MSVC)
  target_compile_options(KUserFeedbackCommon PRIVATE /wd4267 /wd4996)
  target_compile_options(KUserFeedbackCore PRIVATE /wd4267)
else()
  target_compile_options(KUserFeedbackCommon PRIVATE -Wno-sign-conversion -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function)
  target_compile_options(KUserFeedbackCore PRIVATE -Wno-sign-conversion)
endif()

find_package(QtCreator REQUIRED)
add_subdirectory(src)
