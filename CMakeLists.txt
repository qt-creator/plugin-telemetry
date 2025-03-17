cmake_minimum_required(VERSION 3.16)

project(qtctelemetry)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 COMPONENTS Widgets QuickWidgets InsightTracker REQUIRED)
find_package(Qt6 OPTIONAL_COMPONENTS Test)

set(QTC_SBOM_READY ON)
find_package(QtCreator COMPONENTS Core TextEditor ProjectExplorer VcsBase REQUIRED)

if(COMMAND qtc_setup_sbom AND COMMAND qtc_sbom_begin_project)
    qtc_setup_sbom()
    qtc_sbom_compute_cpe(project_cpe
        VENDOR "qt"
        PRODUCT "${PROJECT_NAME}"
        VERSION "${IDE_VERSION}"
    )
    qtc_sbom_get_default_open_source_license(license_expression)
    qtc_sbom_get_default_copyright(copyright)
    qtc_sbom_begin_project(
        PURL_NAMESPACE "qt"
        PURL_NAME "${PROJECT_NAME}"
        VCS_URL_BASE
            "https://codereview.qt-project.org/a/qt-creator/qt-creator/plugin-telemetry.git"
        DOWNLOAD_LOCATION
            "git://codereview.qt-project.org:29418/qt-creator/plugin-telemetry.git"
        LICENSE_EXPRESSION "${license_expression}"
        DEFAULT_LICENSE_EXPRESSION "${license_expression}"
        COPYRIGHTS "${copyright}"
        CPE "${project_cpe}"
    )
endif()

add_subdirectory(src)

if(COMMAND qtc_sbom_end_project)
    qtc_sbom_end_project()
endif()
