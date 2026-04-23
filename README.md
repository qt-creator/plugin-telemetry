# Qt Creator Telemetry Plugin

The plugin is used in Qt Creator and Qt Design Studio to send telemetry data.
It is based on [Qt Insight Tracker](https://doc.qt.io/qtinsighttracker/qtinsighttracker-overview.html)

# Building the plugin

The plugin needs to be built against a Qt Creator version, and the matching Qt installation.
This happens by setting `CMAKE_PREFIX_PATH` to the Qt and Qt Creator build directories.

Qt must include the Qt Insight Tracker module.

To configure the backend you need to set the server credentials:

`QTC_INSIGHT_URL`: Qt Insight server URL to your project

`QTC_INSIGHT_TOKEN`: Authentication token

If `QTC_INSIGHT_URL` and `QTC_INSIGHT_TOKEN` are not set, no data will be send.

# Data Storage

The cache path for collected data until sent is stored in the local user settings:

`QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/insight"`

Windows: `C:\Users\<USER>\AppData\Local\QtProject\QtCreator\insight\`

Linux: `$HOME/.cache/QtProject/QtCreator/insight/`

macOS: `$HOME/Library/Caches/QtProject/QtCreator/insight/`
