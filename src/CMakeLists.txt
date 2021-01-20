cmake_minimum_required(VERSION 3.9)

project(UsageStatistic)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator COMPONENTS Core TextEditor ProjectExplorer VcsBase REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KUserFeedback REQUIRED)

add_qtc_plugin(UsageStatistic
    PLUGIN_DEPENDS
        QtCreator::Core QtCreator::Debugger QtCreator::ProjectExplorer QtCreator::QtSupport
    DEPENDS Qt5::Widgets QtCreator::ExtensionSystem QtCreator::Utils KUserFeedbackCore KUserFeedbackWidgets
    SOURCES
        usagestatisticplugin.cpp
        datasources/qtclicensesource.cpp
        datasources/buildcountsource.cpp
        common/scopedsettingsgroupsetter.cpp
        datasources/buildsystemsource.cpp
        datasources/timeusagesourcebase.cpp
        datasources/modeusagetimesource.cpp
        datasources/examplesdatasource.cpp
        datasources/kitsource.cpp
        datasources/qmldesignerusagetimesource.cpp
        datasources/qmldesignerusageeventsource.cpp
        datasources/servicesource.cpp
        ui/usagestatisticpage.cpp
        ui/usagestatisticwidget.cpp
        services/datasubmitter.cpp
        usagestatistic.qrc
    DEFINES
        USP_AUTH_KEY="${USP_AUTH_KEY}"
        USP_SERVER_URL="${USP_SERVER_URL}"
)
