// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usagestatisticplugin.h"
#include "coreplugin/actionmanager/actionmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/aspects.h>
#include <utils/async.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>
#include <utils/theme/theme.h>

#include <coreplugin/designmode.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QCryptographicHash>
#include <QGuiApplication>
#include <QInsightConfiguration>
#include <QInsightTracker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QTimer>

// BUILD TIME DEPENDENCIES ONLY:
#include <android/androidconstants.h>
#include <baremetal/baremetalconstants.h>
#include <boot2qt/qdbconstants.h>
#include <devcontainer/devcontainerplugin_constants.h>
#include <docker/dockerconstants.h>
#include <ios/iosconstants.h>
#include <mcusupport/mcusupportconstants.h>
#include <remotelinux/remotelinux_constants.h>
#include <webassembly/webassemblyconstants.h>
#include <qnx/qnxconstants.h>

using namespace Core;
using namespace Debugger;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

Q_LOGGING_CATEGORY(statLog, "qtc.usagestatistic", QtWarningMsg);
Q_LOGGING_CATEGORY(qtmodulesLog, "qtc.usagestatistic.qtmodules", QtWarningMsg);
Q_LOGGING_CATEGORY(qtexampleLog, "qtc.usagestatistic.qtexample", QtWarningMsg);
Q_LOGGING_CATEGORY(qmlmodulesLog, "qtc.usagestatistic.qmlmodules", QtWarningMsg);

const char kSettingsPageId[] = "UsageStatistic.PreferencesPage";

namespace UsageStatistic::Internal {

static UsageStatisticPlugin *m_instance = nullptr;

#define QT_WITH_CONTEXTDATA QT_VERSION_CHECK(6, 9, 2)

static void addEvent(QInsightTracker *tracker, const QString &key, const QString &data)
{
#if QT_VERSION >= QT_WITH_CONTEXTDATA
    tracker->contextData(key, data);
#else
    tracker->interaction(key, data, 0);
#endif
}

static QString hashed(const QString &path)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex());
}

static QString projectId(Project *project)
{
    return hashed(project->projectFilePath().toFSPathString());
}

class ModeChanges : public QObject
{
    Q_OBJECT
public:
    ModeChanges(QInsightTracker *tracker)
    {
        const auto id = [](const Id &modeId) -> QString {
            QString ret = ":MODE:" + QString::fromUtf8(modeId.name());
            if (modeId == Core::Constants::MODE_DESIGN)
                ret += ":" + QString::fromUtf8(DesignMode::currentDesignWidget().name());
            return ret;
        };
        connect(ModeManager::instance(), &ModeManager::currentModeChanged, this, [=](const Id &modeId) {
            tracker->transition(id(modeId));
        });
        // initialize with current mode
        tracker->transition(id(ModeManager::currentModeId()));
    }
};

class UILanguage : public QObject
{
    Q_OBJECT
public:
    UILanguage(QInsightTracker *tracker)
    {
        addEvent(tracker, ":CONFIG:UILanguage", ICore::userInterfaceLanguage());
        const QStringList languages = QLocale::system().uiLanguages();
        addEvent(tracker,
                 ":CONFIG:SystemLanguage",
                 languages.isEmpty() ? QString("Unknown") : languages.first());
    }
};

class Theme : public QObject
{
    Q_OBJECT
public:
    Theme(QInsightTracker *tracker)
    {
        addEvent(tracker, ":CONFIG:Theme", creatorTheme() ? creatorTheme()->id() : QString("Unknown"));
        const QString systemTheme = QString::fromUtf8(QMetaEnum::fromType<Qt::ColorScheme>().valueToKey(
                                                          int(Utils::Theme::systemColorScheme())))
                                        .toLower();
        addEvent(tracker, ":CONFIG:SystemTheme", systemTheme);
    }
};

class AndroidManifestGuiEnabled : public QObject
{
    Q_OBJECT

public:
    AndroidManifestGuiEnabled(QInsightTracker *tracker)
    {
        connect(ExtensionSystem::PluginManager::instance(), &ExtensionSystem::PluginManager::objectAdded,
        this, [this, tracker](QObject *object) {
            if (object->objectName() == QString("AndroidManifestEditorEnabled"))
                addEvent(tracker, ":ANDROID:ManifestGuiEditorEnabled", object->objectName());
        });
    }
};

class BuildConfig : public QObject
{
    Q_OBJECT
public:
    static QStringList getQtPackages(Project *project, QtVersion *qtVersion)
    {
        if (!qtVersion)
            return {};
        const FilePath qtLibPath = qtVersion->libraryPath();
        using ModuleHash = QHash<QString, Utils::Link>;
        const ModuleHash all
            = project->activeBuildSystem()->additionalData("FoundPackages").value<ModuleHash>();
        QStringList packages;
        for (auto it = all.begin(); it != all.end(); ++it) {
            const QString name = it.key();
            const FilePath cmakePath = it.value().targetFilePath;
            if (name.size() > 4 && name.startsWith("Qt") && name[2].isDigit() && name[3].isUpper()
                && !name.endsWith("plugin", Qt::CaseInsensitive) && cmakePath.isChildOf(qtLibPath))
                packages.append(name);
        }
        return packages;
    }

    static QString qtVersionString(QtVersion *qtVersion)
    {
        return qtVersion ? qtVersion->qtVersion().toString() : QString("None");
    }

    static QString nicerDeviceType(const Id &id)
    {
        if (id == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            return "desktop";
        if (id == Android::Constants::ANDROID_DEVICE_TYPE)
            return "android";
        if (id == BareMetal::Constants::BareMetalOsType)
            return "baremetal";
        if (id == Qdb::Constants::QdbLinuxOsType)
            return "boot2qt";
        if (id == DevContainer::Constants::DEVCONTAINER_DEVICE_TYPE)
            return "devcontainer";
        if (id == Docker::Constants::DOCKER_DEVICE_TYPE)
            return "docker";
        if (id == Ios::Constants::IOS_DEVICE_TYPE)
            return "ios";
        if (id == Ios::Constants::IOS_SIMULATOR_TYPE)
            return "iossimulator";
        if (id == McuSupport::Internal::Constants::DEVICE_TYPE)
            return "mcu";
        if (id == Qnx::Constants::QNX_QNX_OS_TYPE)
            return "qnx";
        if (id == RemoteLinux::Constants::GenericLinuxOsType)
            return "remotelinux";
        if (id == WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE)
            return "webassembly";
        if (id == "VxWorks.Device.Type")
            return "vxworks";
        return id.toString();
    }

    static QString buildDevice(Kit *kit)
    {
        return nicerDeviceType(BuildDeviceTypeKitAspect::deviceTypeId(kit));
    }

    static QString runDevice(Kit *kit)
    {
        return nicerDeviceType(RunDeviceTypeKitAspect::deviceTypeId(kit));
    }

    static QString cppCompiler(Kit *kit)
    {
        Toolchain *tc
            = ToolchainKitAspect::toolchain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (!tc)
            return "None";
        const Id type = tc->typeId();
        if (type == Android::Constants::ANDROID_TOOLCHAIN_TYPEID)
            return "android";
        if (type == BareMetal::Constants::IAREW_TOOLCHAIN_TYPEID)
            return "iarew";
        if (type == BareMetal::Constants::KEIL_TOOLCHAIN_TYPEID)
            return "keil";
        if (type == BareMetal::Constants::SDCC_TOOLCHAIN_TYPEID)
            return "sdcc";
        if (type == ProjectExplorer::Constants::CUSTOM_TOOLCHAIN_TYPEID)
            return "custom";
        if (type == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID)
            return "gcc";
        if (type == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID)
            return "clang";
        if (type == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
            return "mingw";
        if (type == ProjectExplorer::Constants::LINUXICC_TOOLCHAIN_TYPEID)
            return "icc";
        if (type == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            return "msvc";
        if (type == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID)
            return "clangcl";
        if (type == Qnx::Constants::QNX_TOOLCHAIN_ID)
            return "qnx";
        if (type == WebAssembly::Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID)
            return "webassembly";
        if (type == "VxWorks.ToolChain.Id")
            return "vxworks";
        return type.toString();
    }

    static QString cppCompilerVersion(Kit *kit)
    {
        Toolchain *tc
            = ToolchainKitAspect::toolchain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (!tc)
            return "None";
        return tc->version().toString();
    }

    static QString debugger(Kit *kit)
    {
        const DebuggerItem *debugger = DebuggerKitAspect::debugger(kit);
        if (!debugger)
            return "None";
        return debugger->engineTypeName();
    }

    static QString debuggerVersion(Kit *kit)
    {
        const DebuggerItem *debugger = DebuggerKitAspect::debugger(kit);
        if (!debugger)
            return "None";
        return debugger->version();
    }

    BuildConfig(QInsightTracker *tracker)
    {
        connect(
            ProjectManager::instance(),
            &ProjectManager::projectAdded,
            this,
            [this, tracker](Project *project) {
                connect(project, &Project::anyParsingFinished, this, [project, tracker] {
                    if (!project->activeBuildSystem())
                        return;
                    Kit *kit = project->activeBuildSystem()->kit();
                    if (!kit)
                        return;
                    QtVersion *qtVersion = QtKitAspect::qtVersion(kit);
                    const QStringList qtPackages = getQtPackages(project, qtVersion);
                    QJsonObject json;
                    json.insert("projectid", projectId(project));
                    json.insert("qtmodules", QJsonArray::fromStringList(qtPackages));
                    json.insert("qtversion", qtVersionString(qtVersion));
                    json.insert("buildSystem", project->activeBuildSystem()->name());
                    json.insert("buildDeviceType", buildDevice(kit));
                    json.insert("runDeviceType", runDevice(kit));
                    json.insert("cppCompilerType", cppCompiler(kit));
                    json.insert("cppCompilerVersion", cppCompilerVersion(kit));
                    json.insert("debuggerType", debugger(kit));
                    json.insert("debuggerVersion", debuggerVersion(kit));
                    const QString jsonStr = QString::fromUtf8(
                        QJsonDocument(json).toJson(QJsonDocument::Compact));
                    qCDebug(qtmodulesLog) << qPrintable(jsonStr);
                    addEvent(tracker, "BuildConfig", jsonStr);
                });
            });
    }
};

class QmlModules : public QObject
{
    Q_OBJECT

public:
    using ScanStorage = QtTaskTree::Storage<std::unique_ptr<TemporaryFilePath>>;

    QmlModules(QInsightTracker *tracker)
    {
        // Management code for being able to access the project's import paths
        // Would be nice if this was available more directly from the project->activeBuildSystem()
        connect(
            ProjectManager::instance(),
            &ProjectManager::extraProjectInfoChanged,
            this,
            [this](BuildConfiguration *bc, const ProjectExplorer::QmlCodeModelInfo &extra) {
                m_qmlCodeModelInfo.insert(bc, extra);
            });
        connect(
            ProjectManager::instance(),
            &ProjectManager::buildConfigurationRemoved,
            this,
            [this](BuildConfiguration *bc) { m_qmlCodeModelInfo.remove(bc); });
        // The actual retrieval of QML modules
        connect(
            BuildManager::instance(),
            &BuildManager::buildStateChanged,
            this,
            [this, tracker](Project *project) {
                if (!shouldStartCollectingFor(project))
                    return;

                if (!project->activeBuildSystem())
                    return;
                if (!project->activeBuildSystem()->kit())
                    return;
                QtVersion *qtVersion = QtKitAspect::qtVersion(project->activeBuildSystem()->kit());
                if (!qtVersion)
                    return;
                const FilePath qmlimportscanner = qtVersion->hostLibexecPath()
                                                      .pathAppended("qmlimportscanner")
                                                      .withExecutableSuffix();
                const FilePath qtImportPath = qtVersion->qmlPath();
                FilePaths importPaths
                    = m_qmlCodeModelInfo.value(project->activeBuildConfiguration()).qmlImportPaths;
                if (!qtImportPath.isEmpty())
                    importPaths << qtImportPath;
                const FilePaths qmlFiles = project->files([](const Node *n) -> bool {
                    const FilePath filePath = n->filePath();
                    if (!n->asFileNode() || n->isGenerated() || !n->isEnabled()
                        || filePath.isEmpty())
                        return false;
                    const MimeType mimeType
                        = mimeTypeForFile(filePath, MimeMatchMode::MatchExtension);
                    return mimeType.matchesName(Utils::Constants::QML_MIMETYPE)
                           || mimeType.matchesName(Utils::Constants::QMLUI_MIMETYPE);
                });
                if (qmlFiles.isEmpty()) {
                    qCDebug(qmlmodulesLog) << QString("No QML files found for project \"%1\".")
                                                  .arg(project->displayName());
                    return;
                }
                qCDebug(qmlmodulesLog) << QString("Starting \"%1\" for project \"%2\" and files %3")
                                              .arg(
                                                  qmlimportscanner.toUserOutput(),
                                                  project->displayName(),
                                                  qmlFiles.toUserOutput(", "));
                const QString id = projectId(project);
                const QString qtVersionString = qtVersion->qtVersion().toString();

                const ScanStorage storage;

                m_runner.start(
                    project,
                    QtTaskTree::Group{
                        QtTaskTree::sequential,
                        storage,
                        createResponseFile(storage, qmlimportscanner, qmlFiles, importPaths),
                        runQmlImportScanner(storage, qmlimportscanner, id, qtVersionString, tracker)});
            });
    }

    QtTaskTree::ExecutableItem createResponseFile(
        const ScanStorage &storage,
        const FilePath &qmlimportscanner,
        const FilePaths &qmlFiles,
        const FilePaths &importPaths)
    {
        const auto setup = [qmlimportscanner, qmlFiles, importPaths](
                               Async<Result<TemporaryFilePath *>> &async) {
            async.setConcurrentCallData(
                [](const FilePath &qmlimportscanner,
                   const FilePaths &qmlFiles,
                   const FilePaths &importPaths) -> Result<TemporaryFilePath *> {
                    // Remove files that do not exist
                    const FilePaths actualQmlFiles = Utils::filtered(qmlFiles, &FilePath::exists);
                    const Result<FilePath> tmpDir = qmlimportscanner.tmpDir();
                    if (!tmpDir)
                        return ResultError("Failed to determine temporary directory");
                    auto tempPath = TemporaryFilePath::create(
                        tmpDir->pathAppended("qmlimportscanner.rsp"));
                    if (!tempPath) {
                        return ResultError(
                            QString("Failed to create response file: %1").arg(tempPath.error()));
                    }
                    QByteArrayList arguments = {"-qmlFiles"};
                    for (const FilePath &file : actualQmlFiles)
                        arguments << file.nativePath().toUtf8();
                    for (const FilePath &importPath : importPaths)
                        arguments << "-importPath" << importPath.nativePath().toUtf8();
                    if (Result<qint64> writeResult = (*tempPath)->filePath().writeFileContents(
                            arguments.join('\n'));
                        !writeResult) {
                        return ResultError(
                            QString("Failed to create response file: %1").arg(writeResult.error()));
                    }
                    return tempPath->release();
                },
                qmlimportscanner,
                qmlFiles,
                importPaths);
        };
        const auto done = [storage](const Async<Result<TemporaryFilePath *>> &async) {
            Result<TemporaryFilePath *> result = async.result();
            if (!result) {
                qCDebug(qmlmodulesLog) << "Failed to set up qmlimportscanner:" << result.error();
                return QtTaskTree::DoneResult::Error;
            }
            storage->reset(*result);
            return QtTaskTree::DoneResult::Success;
        };
        return AsyncTask<Result<TemporaryFilePath *>>(setup, done);
    }

    QtTaskTree::ExecutableItem runQmlImportScanner(
        const ScanStorage &storage,
        const FilePath &qmlimportscanner,
        const QString &projectId,
        const QString &qtVersionString,
        QInsightTracker *tracker)
    {
        const auto setup = [qmlimportscanner, storage](Process &process) {
            process.setCommand({qmlimportscanner, {"@" + (*storage)->filePath().nativePath()}});
        };
        const auto done = [projectId,
                           qtVersionString,
                           tracker = QPointer<QInsightTracker>(tracker)](const Process &process) {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(process.rawStdOut(), &error);
            if (error.error != QJsonParseError::NoError) {
                qCDebug(qmlmodulesLog) << "Parse error:" << error.errorString();
                qCDebug(qmlmodulesLog) << "At:" << error.offset;
                qCDebug(qmlmodulesLog) << "Stdout:";
                qCDebug(qmlmodulesLog) << qPrintable(process.stdOut());
                qCDebug(qmlmodulesLog) << "Stderr:";
                qCDebug(qmlmodulesLog) << qPrintable(process.stdErr());
                return;
            }
            qCDebug(qmlmodulesLog) << "Response:" << qPrintable(QString::fromUtf8(doc.toJson()));
            if (!doc.isArray()) {
                qCDebug(qmlmodulesLog) << "Unexpected response, not a JSON array";
                return;
            }
            QStringList qmlModules;
            const QJsonArray array = doc.array();
            for (const QJsonValue &v : array) {
                const QJsonObject obj = v.toObject();
                const QString name = obj.value("name").toString();
                if (name.isEmpty()) {
                    qCDebug(qmlmodulesLog) << "Skipping import without name";
                    continue;
                }
                const QString type = obj.value("type").toString();
                if (type != "module") {
                    qCDebug(qmlmodulesLog) << "Skipping import with type \"" + type + "\"";
                    continue;
                }
                qmlModules += name;
            }
            if (qmlModules.isEmpty())
                return;
            // - the list of modules can contain all kinds of user defined modules too, since
            //   we need to add the user import paths to catch the QDS modules
            // - a hardcoded whitelist here would be ugly because older Qt Creator versions would
            //   filter out new QML modules in newer Qt versions
            // - so send a hash of the module "name" to telemetry, and the script that processes
            //   that data has a mapping of hash -> known Qt module
            const QStringList moduleHashes = Utils::transform(qmlModules, hashed);
            QJsonObject json;
            json.insert("projectid", projectId);
            json.insert("qmlmodules", QJsonArray::fromStringList(moduleHashes));
            json.insert("qtversion", qtVersionString);
            const QString jsonStr = QString::fromUtf8(
                QJsonDocument(json).toJson(QJsonDocument::Compact));
            qCDebug(qmlmodulesLog) << qPrintable(jsonStr);
            addEvent(tracker, "QmlModules", jsonStr);
        };
        return ProcessTask(setup, done);
    }

    bool shouldStartCollectingFor(Project *project)
    {
        if (!BuildManager::isBuilding(project)) {
            m_buildingProjects.remove(project);
            return false;
        }
        if (!Utils::insert(m_buildingProjects, project)) // already seen before as "building"
            return false;

        // don't interrupt a running scan
        return !m_runner.isKeyRunning(project);
    }

    QHash<BuildConfiguration *, QmlCodeModelInfo> m_qmlCodeModelInfo;
    QSet<Project *> m_buildingProjects;
    QMappedTaskTreeRunner<Project *> m_runner;
};

class QtExample : public QObject
{
    Q_OBJECT
public:
    QtExample(QInsightTracker *tracker)
    {
        connect(ProjectManager::instance(),
                &ProjectManager::projectAdded,
                this,
                [this, tracker](Project *project) {
                    connect(project, &Project::anyParsingFinished, this, [project, tracker] {
                        const QtVersions versions = QtVersionManager::versions();
                        for (QtVersion *qtVersion : versions) {
                            const FilePath examplesPath = qtVersion->examplesPath();
                            if (examplesPath.isEmpty())
                                continue;
                            if (!project->projectFilePath().isChildOf(examplesPath))
                                continue;
                            const FilePath examplePath = project->projectFilePath()
                                                             .relativeChildPath(examplesPath)
                                                             .parentDir();
                            const QString exampleHash = hashed(examplePath.path());
                            QJsonObject json;
                            json.insert("projectid", projectId(project));
                            json.insert("qtexample", exampleHash);
                            json.insert("qtversion", qtVersion->qtVersion().toString());
                            const QString jsonStr = QString::fromUtf8(
                                QJsonDocument(json).toJson(QJsonDocument::Compact));
                            qCDebug(qtexampleLog) << qPrintable(jsonStr);
                            addEvent(tracker, "QtExample", jsonStr);
                            return;
                        }
                    });
                });
    }
};

static const char licenseKey[] = "QtLicenseSchema";
class QtLicense : public QObject
{
    Q_OBJECT
public:
    static QObject *getLicensechecker()
    {
        return PluginManager::getObjectByName("LicenseCheckerPlugin");
    }

    QtLicense(QInsightTracker *tracker)
        : m_tracker(tracker)
    {
        QObject *licensechecker = getLicensechecker();
        if (!licensechecker) {
            addEvent(tracker, licenseKey, "opensource");
            return;
        }
        connect(
            licensechecker, SIGNAL(licenseInfoAvailableChanged()), this, SLOT(reportLicenseInfo()));
        reportLicenseInfo();
    }

private slots:
    void reportLicenseInfo()
    {
        QObject *licensechecker = getLicensechecker();
        QTC_ASSERT(licensechecker, return);
        bool available = false;
        QTC_CHECK(
            QMetaObject::invokeMethod(
                licensechecker,
                "isLicenseInfoAvailable",
                Qt::DirectConnection,
                Q_RETURN_ARG(bool, available)));
        if (!available)
            return;
        QString schema;
        QTC_CHECK(
            QMetaObject::invokeMethod(
                licensechecker,
                "licenseSchema",
                Qt::DirectConnection,
                Q_RETURN_ARG(QString, schema)));
        addEvent(m_tracker, licenseKey, schema);
    }

private:
    QInsightTracker *m_tracker = nullptr;
};

class Settings : public AspectContainer
{
public:
    Settings()
    {
        setAutoApply(false);
        setSettingsGroup("UsageStatistic");
        trackingEnabled.setDefaultValue(false);
        trackingEnabled.setSettingsKey("TrackingEnabled");
        trackingEnabled.setLabel(UsageStatisticPlugin::tr("Send pseudonymous usage statistics"),
                                 BoolAspect::LabelPlacement::AtCheckBox);
    }

    Utils::BoolAspect trackingEnabled{this};
};

static Settings &theSettings()
{
    static Settings settings;
    return settings;
}

class SettingsWidget : public IOptionsPageWidget
{
public:
    SettingsWidget()
    {
        Settings &s = theSettings();

        using namespace Layouting;
        auto moreInformationLabel = new QLabel(
            "<a "
            "href=\"qthelp://org.qt-project.qtcreator/doc/"
            "creator-how-to-collect-usage-statistics.html\">"
            + UsageStatisticPlugin::tr("More information") + "</a>");
        connect(moreInformationLabel, &QLabel::linkActivated, [this](const QString &link) {
            HelpManager::showHelpUrl(link, HelpManager::ExternalHelpAlways);
        });
        // clang-format off
        Column {
            s.trackingEnabled,
            Group {
                Column {
                    Label {
                        text(UsageStatisticPlugin::tr("%1 collects pseudonymous information about "
                             "your system and the way you use the application. The data is "
                             "associated with a pseudonymous user ID generated only for this "
                             "purpose. The data will be shared with services managed by "
                             "The Qt Company. It does however not contain individual content "
                             "created by you, and will be used by The Qt Company strictly for the "
                             "purposes of improving their products.")
                                .arg(QGuiApplication::applicationDisplayName())),
                        wordWrap(true)
                    },
                    moreInformationLabel,
                    st
                }
            }
        }.attachTo(this);
        // clang-format on

        setOnApply([] {
            theSettings().apply();
            theSettings().writeToSettingsImmediatly(); // write the updated "TrackingEnabled" value to the .ini
            m_instance->configureInsight();
        });
        setOnCancel([] { theSettings().cancel(); });
    }
};

class SettingsPage final : public Core::IOptionsPage
{
public:
    SettingsPage()
    {
        setId(kSettingsPageId);
        setCategory("Telemetry");
        setDisplayName(UsageStatisticPlugin::tr("Usage Statistics"));
        setWidgetCreator([] { return new SettingsWidget; });
    }
};

static void setupSettingsPage()
{
    static SettingsPage settings;
}

UsageStatisticPlugin::UsageStatisticPlugin()
{
    m_instance = this;
    Core::IOptionsPage::registerCategory(
        "Telemetry",
        UsageStatisticPlugin::tr("Telemetry"),
        ":/usagestatistic/images/settingscategory_usagestatistic.png");
}

UsageStatisticPlugin::~UsageStatisticPlugin() = default;

void UsageStatisticPlugin::initialize()
{
    setupSettingsPage();
    theSettings().readSettings();
}

void UsageStatisticPlugin::extensionsInitialized()
{
}

bool UsageStatisticPlugin::delayedInitialize()
{
    if (theSettings().trackingEnabled.value())
        configureInsight();

    showInfoBar();

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag UsageStatisticPlugin::aboutToShutdown()
{
    theSettings().writeSettings();

    return SynchronousShutdown;
}

static constexpr int defaultSubmissionInterval()
{
    using namespace std::literals;
    return std::chrono::hours(1) / 1s;
}

static constexpr int defaultBatchSize()
{
    return 100;
}

static int fromEnvironment(const QString &key, int defaultValue)
{
    bool ok = false;
    const int env = qtcEnvironmentVariableIntValue(key, &ok);
    if (ok)
        return env;
    return defaultValue;
}

void UsageStatisticPlugin::configureInsight()
{
    qCDebug(statLog) << "Configuring insight, enabled:" << theSettings().trackingEnabled.value();
    if (theSettings().trackingEnabled.value()) {
        if (!m_tracker || !m_tracker->isEnabled()) {
            // silence qt.insight.*.info logging category if logging for usagestatistic is not enabled
            // the issue here is, that qt.insight.*.info is enabled by default and spams terminals
            static std::optional<QLoggingCategory::CategoryFilter> previousFilter;
            if (!statLog().isDebugEnabled()) {
                previousFilter = QLoggingCategory::installFilter([](QLoggingCategory *log) {
                    if (previousFilter && previousFilter.has_value())
                        (*previousFilter)(log);
                    if (QString::fromUtf8(log->categoryName()).startsWith("qt.insight"))
                        log->setEnabled(QtInfoMsg, false);
                });
            }

            qCDebug(statLog) << "Creating tracker";
            m_tracker.reset(new QInsightTracker);
            QInsightConfiguration *config = m_tracker->configuration();
            config->setEvents({}); // the default is a big list including key events....
            config->setStoragePath(ICore::cacheResourcePath("insight").path());
            qCDebug(statLog) << "Cache path:" << config->storagePath();
            // TODO provide a button for removing the cache?
            // TODO config->setStorageSize(???); // unlimited by default
            config->setSyncInterval(
                fromEnvironment("QTC_INSIGHT_SUBMISSIONINTERVAL", defaultSubmissionInterval()));
            config->setBatchSize(fromEnvironment("QTC_INSIGHT_BATCHSIZE", defaultBatchSize()));
            config->setDeviceModel(QString("%1 (%2)").arg(QSysInfo::productType(),
                                                          QSysInfo::currentCpuArchitecture()));
            config->setDeviceVariant(QSysInfo::productVersion());
            config->setDeviceScreenType("NON_TOUCH");
            config->setPlatform("app"); // see "Snowplow Tracker Protocol"
            config->setAppBuild(appInfo().displayVersion);
            config->setServer(QTC_INSIGHT_URL);
            config->setToken(QTC_INSIGHT_TOKEN);
            m_tracker->setEnabled(true);
            createProviders();
            m_tracker->startNewSession();

            // reinstall previous logging filter if required
            if (previousFilter)
                QLoggingCategory::installFilter(*previousFilter);
        }
    } else {
        if (m_tracker)
            m_tracker->setEnabled(false);
    }

    Core::Command *cmd = Core::ActionManager::command("Help.GiveFeedback");

    if (cmd) {
        cmd->action()->setEnabled(m_tracker->isEnabled());
        cmd->action()->setVisible(m_tracker->isEnabled());
    }
}

void UsageStatisticPlugin::showInfoBar()
{
    static const char kInfoBarId[] = "UsageStatistic.AskAboutCollectingDataInfoBar";
    InfoBar *infoBar = ICore::popupInfoBar();
    if (!infoBar->canInfoBeAdded(kInfoBarId) || theSettings().trackingEnabled.value())
        return;
    static auto infoText = UsageStatisticPlugin::tr(
                               "We make %1 for you. Would you like to help us make it even better?")
                               .arg(QGuiApplication::applicationDisplayName());
    static auto configureButtonInfoText = UsageStatisticPlugin::tr("Configure Usage Statistics...");
    static auto cancelButtonInfoText = UsageStatisticPlugin::tr("Decide later");

    ::Utils::InfoBarEntry entry(kInfoBarId, infoText, InfoBarEntry::GlobalSuppression::Enabled);
    entry.setTitle("Usage Statistics");
    entry.addCustomButton(configureButtonInfoText, [infoBar] {
        infoBar->removeInfo(kInfoBarId);
        ICore::showSettings(kSettingsPageId);
    });
    entry.setCancelButtonInfo(cancelButtonInfoText, {});
    infoBar->addInfo(entry);
}

void UsageStatisticPlugin::createProviders()
{
    // startup configs first, otherwise they will be attributed to the UI state
    m_providers.push_back(std::make_unique<Theme>(m_tracker.get()));
    // module and example telemetry require QInsightTracker::contextData to
    // work reliably, because the key of QInsightTracker::interaction is limited to 255 characters.
#if QT_VERSION >= QT_WITH_CONTEXTDATA
    m_providers.push_back(std::make_unique<BuildConfig>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QtExample>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QmlModules>(m_tracker.get()));
#endif

    m_providers.push_back(std::make_unique<UILanguage>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QtLicense>(m_tracker.get()));

    // UI state last
    m_providers.push_back(std::make_unique<ModeChanges>(m_tracker.get()));

    //Android Manifest enabled
    m_providers.push_back(std::make_unique<AndroidManifestGuiEnabled>(m_tracker.get()));

    for (const auto &provider : m_providers) {
        qCDebug(statLog) << "Created usage statistics provider"
                         << provider.get()->metaObject()->className();
    }
}

} // namespace UsageStatistic::Internal

#include "usagestatisticplugin.moc"
