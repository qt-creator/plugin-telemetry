cmake_minimum_required(VERSION 3.10)

project(plugin-telemetry)

include(ExternalProject)

# Set common CMAKE_PREFIX_PATH/ CMAKE_PREFIX_PATH_ALT_SEP
# that can be forwarded to external projects
list(APPEND CMAKE_PREFIX_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/extra-cmake-modules
    ${CMAKE_CURRENT_BINARY_DIR}/kuserfeedback)
string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH}")

ExternalProject_Add(extra-cmake-modules
    PREFIX extra-cmake-modules
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/extra-cmake-modules"
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/extra-cmake-modules
)

set(KUSERFEEDBACK_COMPONENTS
    -DBUILD_SHARED_LIBS=OFF
    -DBUILD_TESTING=OFF
    -DENABLE_SURVEY_TARGET_EXPRESSIONS=OFF
    -DENABLE_PHP=OFF
    -DENABLE_PHP_UNIT=OFF
    -DENABLE_DOCS=OFF
    -DENABLE_CONSOLE=OFF
    -DENABLE_CLI=OFF
    -DBUILD_SHARED_LIBS=OFF
)

ExternalProject_Add(kuserfeedback
    PREFIX kuserfeedback
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/kuserfeedback"
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${KUSERFEEDBACK_DEFINES}
        ${KUSERFEEDBACK_COMPONENTS}
        -DKDE_INSTALL_LIBDIR=lib
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/kuserfeedback
    BUILD_ALWAYS ON
    DEPENDS extra-cmake-modules
)

if(NOT DEFINED USP_SERVER_URL OR NOT DEFINED USP_AUTH_KEY)
    message(WARNING "Collected data won't be sent
Define both USP_SERVER_URL and USP_AUTH_KEY to enable data submission")
endif()

find_package(QtCreator COMPONENTS Core Debugger ProjectExplorer QtSupport QUIET)

ExternalProject_Add(plugin
    PREFIX plugin
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    LIST_SEPARATOR |
    CMAKE_ARGS
        -DUSP_AUTH_KEY=${USP_AUTH_KEY}
        -DUSP_SERVER_URL=${USP_SERVER_URL}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DQtCreator_BINARY_DIR:PATH=${QtCreator_BINARY_DIR}
        -DQTC_MERGE_BINARY_DIR:BOOL=${QTC_MERGE_BINARY_DIR}
    DEPENDS kuserfeedback QtCreator::Core QtCreator::Debugger QtCreator::ProjectExplorer QtCreator::QtSupport
    BUILD_ALWAYS ON
)
