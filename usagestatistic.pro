DEFINES += USAGESTATISTIC_LIBRARY

KUSERFEEDBACK_INSTALL_PATH = "$${OUT_PWD}/kuserfeedback"

INCLUDEPATH *= "$$shell_path($${KUSERFEEDBACK_INSTALL_PATH}/include)" "$${PWD}"

CONFIG += c++1z
QMAKE_CXXFLAGS *= -Wall -Wextra -pedantic

DEFINES += $$shell_quote(USP_AUTH_KEY=\"$$(USP_AUTH_KEY)\")
DEFINES += $$shell_quote(USP_SERVER_URL=\"$$(USP_SERVER_URL)\")

# UsageStatistic files
SOURCES += \
    usagestatisticplugin.cpp \
    datasources/qtclicensesource.cpp \
    datasources/buildcountsource.cpp \
    common/scopedsettingsgroupsetter.cpp \
    datasources/buildsystemsource.cpp \
    datasources/timeusagesourcebase.cpp \
    datasources/modeusagetimesource.cpp \
    datasources/examplesdatasource.cpp \
    datasources/kitsource.cpp \
    datasources/qmldesignerusagetimesource.cpp \
    ui/usagestatisticpage.cpp \
    ui/usagestatisticwidget.cpp \
    ui/outputpane.cpp \
    ui/encouragementwidget.cpp \
    services/datasubmitter.cpp

HEADERS += \
    usagestatisticplugin.h \
    usagestatistic_global.h \
    usagestatisticconstants.h \
    datasources/qtclicensesource.h \
    datasources/buildcountsource.h \
    common/scopedsettingsgroupsetter.h \
    datasources/buildsystemsource.h \
    datasources/timeusagesourcebase.h \
    datasources/modeusagetimesource.h \
    datasources/examplesdatasource.h \
    datasources/kitsource.h \
    datasources/qmldesignerusagetimesource.h \
    ui/usagestatisticpage.h \
    ui/usagestatisticwidget.h \
    ui/outputpane.h \
    ui/encouragementwidget.h \
    services/datasubmitter.h \
    common/utils.h

# Qt Creator linking

## Either set the IDE_SOURCE_TREE when running qmake,
## or set the QTC_SOURCE environment variable, to override the default setting
isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)

## Either set the IDE_BUILD_TREE when running qmake,
## or set the QTC_BUILD environment variable, to override the default setting
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)

## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on macOS
# USE_USER_DESTDIR = yes

###### If the plugin can be depended upon by other plugins, this code needs to be outsourced to
###### <dirname>_dependencies.pri, where <dirname> is the name of the directory containing the
###### plugin's sources.

QTC_PLUGIN_NAME = UsageStatistic
QTC_LIB_DEPENDS += \
    # nothing here at this time

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    debugger \
    projectexplorer \
    qtsupport

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time

###### End _dependencies.pri contents ######

!build_pass {
    EXTRA_CMAKE_MODULES_BUILD_PATH   = "$${OUT_PWD}/extra-cmake-modules/build"
    EXTRA_CMAKE_MODULES_SOURCE_PATH  = "$${PWD}/3rdparty/extra-cmake-modules"
    EXTRA_CMAKE_MODULES_INSTALL_PATH = "$${OUT_PWD}/extra-cmake-modules"

    # Configure extra-cmake-modules
    system("cmake -S $$shell_path($${EXTRA_CMAKE_MODULES_SOURCE_PATH}) \
                  -B $$shell_path($${EXTRA_CMAKE_MODULES_BUILD_PATH}) \
                  -DCMAKE_INSTALL_PREFIX:PATH=\"$$shell_path($${EXTRA_CMAKE_MODULES_INSTALL_PATH})\"")

    # "Build" extra-cmake-modules first time. This step is required to configure KUserFeedback
    EXTRA_CMAKE_MODULES_BUILD_CMD = "cmake --build $$shell_path($${EXTRA_CMAKE_MODULES_BUILD_PATH}) --parallel --target install"
    system("$${EXTRA_CMAKE_MODULES_BUILD_CMD}")

    # Configure KUserFeedback
    KUSERFEEDBACK_BUILD_PATH  = "$${OUT_PWD}/kuserfeedback/build"
    KUSERFEEDBACK_SOURCE_PATH = "$${PWD}/3rdparty/kuserfeedback"

    CMAKE_PREFIX_PATHS = "$$shell_path($$[QT_INSTALL_LIBS]/cmake);$$shell_path($${EXTRA_CMAKE_MODULES_INSTALL_PATH}/share/ECM/cmake)"

    BUILD_TYPE = Debug
    CONFIG(release, debug|release): BUILD_TYPE = Release

    system("cmake -S $$shell_path($${KUSERFEEDBACK_SOURCE_PATH}) \
                  -B $$shell_path($${KUSERFEEDBACK_BUILD_PATH}) \
                  -DBUILD_SHARED_LIBS=OFF \
                  -DENABLE_SURVEY_TARGET_EXPRESSIONS=OFF \
                  -DENABLE_PHP=OFF \
                  -DENABLE_PHP_UNIT=OFF \
                  -DENABLE_TESTING=OFF \
                  -DENABLE_DOCS=OFF \
                  -DENABLE_CONSOLE=OFF \
                  -DENABLE_CLI=OFF \
                  -DBUILD_SHARED_LIBS=OFF \
                  -DCMAKE_BUILD_TYPE=$${BUILD_TYPE} \
                  -DCMAKE_INSTALL_PREFIX:PATH=\"$$shell_path($${KUSERFEEDBACK_INSTALL_PATH})\" \
                  -DCMAKE_PREFIX_PATH=\"$${CMAKE_PREFIX_PATHS}\" \
                  -DKDE_INSTALL_LIBDIR=lib")

    buildextracmakemodules.commands = "$${EXTRA_CMAKE_MODULES_BUILD_CMD}"

    buildkuserfeedback.commands = "cmake --build $$shell_path($${KUSERFEEDBACK_BUILD_PATH}) --parallel --target install"
    buildkuserfeedback.depends = buildextracmakemodules

    # Force build order. Without this flag Make tries building targets
    # in a random order when -jN specified.
    # All targets themselves (but not the plugin itself) will be built in parallel
    notParallel.target = .NOTPARALLEL

    QMAKE_EXTRA_TARGETS += buildextracmakemodules buildkuserfeedback notParallel
    PRE_TARGETDEPS += buildkuserfeedback
}

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)

# Put it here to use qtLibraryName function without extra hacks
LIBS *= -L"$$shell_path($${KUSERFEEDBACK_INSTALL_PATH}/lib)" \
            -l$$qtLibraryName(KUserFeedbackCore)             \
            -l$$qtLibraryName(KUserFeedbackWidgets)          \
            -l$$qtLibraryName(KUserFeedbackCommon)

FORMS += \
    ui/usagestatisticwidget.ui \
    ui/encouragementwidget.ui
