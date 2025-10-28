set(ENV_QTC_INSIGHT_TOKEN $ENV{QTC_INSIGHT_TOKEN})
set(ENV_QTC_INSIGHT_URL $ENV{QTC_INSIGHT_URL})
set(QTC_INSIGHT_URL "${ENV_QTC_INSIGHT_URL}" CACHE STRING "URL for Qt Insight")
set(QTC_INSIGHT_TOKEN "${ENV_QTC_INSIGHT_TOKEN}" CACHE STRING "Token for Qt Insight")

if(NOT QTC_INSIGHT_URL)
  message(WARNING "No QTC_INSIGHT_URL set, UsageStatistic plugin will not send any data")
endif()
if(NOT QTC_INSIGHT_TOKEN)
  message(WARNING "No QTC_INSIGHT_TOKEN set, UsageStatistic plugin will not send any data")
endif()

add_qtc_plugin(UsageStatistic
    PLUGIN_DEPENDS
        QtCreator::Core
        QtCreator::Debugger
        QtCreator::ProjectExplorer
        QtCreator::QtSupport
        ${ADDITIONAL_PLUGIN_DEPENDENCIES}
    DEPENDS
        Qt::Widgets
        Qt::InsightTracker
        QtCreator::ExtensionSystem
        QtCreator::Utils
    SOURCES
        usagestatisticplugin.cpp
        usagestatisticplugin.h
        usagestatistic.qrc
    DEFINES
        QTC_INSIGHT_TOKEN="${QTC_INSIGHT_TOKEN}"
        QTC_INSIGHT_URL="${QTC_INSIGHT_URL}"
)

if(TARGET UsageStatistic)
  qt_add_resources(UsageStatistic usagestatistic_resources FILES qtinsight.conf)

  # Deploy QtInsightTracker
  if(APPLE)
    # copy the framework
    get_target_property(location Qt::InsightTracker LOCATION)
    string(REGEX REPLACE "(/.*[.]framework)/.*" "\\1" framework "${location}")
    if(CMAKE_MATCH_1)
      set(lib_is_framework TRUE)
    else()
      set(lib_is_framework FALSE)
    endif()

    if(lib_is_framework)
      install(DIRECTORY "${framework}"
        DESTINATION ${IDE_LIBRARY_PATH}
        COMPONENT Dependencies
        EXCLUDE_FROM_ALL
        FILES_MATCHING
          PATTERN "*"
          REGEX "/Headers.*" EXCLUDE
      )
    else()
      install(FILES "${location}"
        DESTINATION ${IDE_LIBRARY_PATH}
        COMPONENT Dependencies
        EXCLUDE_FROM_ALL
      )
    endif()
  elseif(WIN32)
    # copy the DLL
    get_target_property(location Qt::InsightTracker LOCATION)

    install(FILES
      ${location}
      DESTINATION ${IDE_LIBRARY_ARCHIVE_PATH}
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  else()
    # copy the lib
    get_target_property(location Qt::InsightTracker LOCATION)
    # that's with full version. we also need just major
    string(REGEX MATCH ".*InsightTracker[.]so[.][0-9]*" loc_major ${location})

    install(FILES
      ${location}
      ${loc_major}
      DESTINATION ${IDE_LIBRARY_BASE_PATH}/Qt/lib
      COMPONENT Dependencies
      EXCLUDE_FROM_ALL
    )
  endif()

endif()
