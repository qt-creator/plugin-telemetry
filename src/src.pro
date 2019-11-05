DEFINES += USAGESTATISTIC_LIBRARY

INCLUDEPATH *= "$${PWD}"

CONFIG += c++1z
QMAKE_CXXFLAGS *= -Wall
!msvc:QMAKE_CXXFLAGS *= -Wextra -pedantic

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

RESOURCES += \
    usagestatistic.qrc

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

# KUserFeedback
include(../3rdparty/kuserfeedback/kuserfeedback.pri)

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)

FORMS += \
    ui/usagestatisticwidget.ui \
    ui/encouragementwidget.ui

QMAKE_EXTRA_TARGETS += docs install_docs # dummy targets for consistency

