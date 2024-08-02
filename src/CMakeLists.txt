add_qtc_plugin(UsageStatistic
    PLUGIN_DEPENDS
        QtCreator::Core QtCreator::Debugger QtCreator::ProjectExplorer QtCreator::QtSupport
    DEPENDS Qt::Widgets Qt::QuickWidgets QtCreator::ExtensionSystem QtCreator::Utils KUserFeedbackCore KUserFeedbackWidgets
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
        datasources/applicationsource.cpp
        ui/usagestatisticpage.cpp
        ui/usagestatisticwidget.cpp
        ui/usagestatisticsimplewidget.cpp
        services/datasubmitter.cpp
        usagestatistic.qrc
    DEFINES
        USP_AUTH_KEY="${USP_AUTH_KEY}"
        USP_SERVER_URL="${USP_SERVER_URL}"
)
