# Qt Creator Telemetry Plugin

The plugin is used in Qt Creator and Qt Design Studio to send telemetry data.
It is based on [KUserFeedback](https://api.kde.org/frameworks/kuserfeedback/html).

# Checking out sources

Run `git submodule update --init` to set up the git submodules.

# Building the plugin

The plugin needs to be built against a Qt Creator version, and the matching Qt installation.
This happens by setting `CMAKE_PREFIX_PATH` to the Qt and Qt Creator build directories.

You also net to set `CMAKE_INSTALL_PREFIX`. Either to the Qt Creator installation directory,
or to a separate new directory - you can let Qt Creator load the plugin then by passing the
directory with `-pluginpath`.

To configure the backend you need to set the server credentials:

`USP_SERVER_URL`: server url

`USP_AUTH_KEY`: authentication key

If `USP_SERVER_URL` and `USP_AUTH_KEY` is not set, no data will be send.

## Example build

````
mkdir build && cd build
cmake -G Ninja -D "CMAKE_PREFIX_PATH=<QT_DIR>;<QTC_BUILD_DIR>" -D CMAKE_INSTALL_PREFIX=install ..
cmake --build .
````

Afterwards you should be able to launch Qt Creator with ``-pluginpath`` argument:

````
qtcreator -pluginpath install
````


# Data Storage

The configuration and so far collected data is stored in the local user settings.

Windows: `\\Computer\HKEY_CURRENT_USER\SOFTWARE\QtProject\UserFeedback.QtCreator`

Linux: `$HOME/.config/QtProject/UserFeedback.QtCreator.conf`

macOS: `$HOME/Library/Preferences/com.qtproject.UserFeedback.QtCreator.plist`
