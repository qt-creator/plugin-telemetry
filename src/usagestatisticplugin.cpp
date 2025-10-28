// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usagestatisticplugin.h"
#include "coreplugin/actionmanager/actionmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <solutions/tasking/tasktreerunner.h>
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

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <QCryptographicHash>
#include <QGuiApplication>
#include <QInsightConfiguration>
#include <QInsightTracker>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QTimer>

using namespace Core;
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
            return ":MODE:" + QString::fromUtf8(modeId.name());
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

class QtModules : public QObject
{
    Q_OBJECT
public:
    QtModules(QInsightTracker *tracker)
    {
        connect(ProjectManager::instance(),
                &ProjectManager::projectAdded,
                this,
                [this, tracker](Project *project) {
                    connect(project, &Project::anyParsingFinished, this, [project, tracker] {
                        if (!project->activeBuildSystem())
                            return;
                        if (!project->activeBuildSystem()->kit())
                            return;
                        QtVersion *qtVersion = QtKitAspect::qtVersion(
                            project->activeBuildSystem()->kit());
                        if (!qtVersion)
                            return;
                        const FilePath qtLibPath = qtVersion->libraryPath();
                        using ModuleHash = QHash<QString, Utils::Link>;
                        const ModuleHash all = project->activeBuildSystem()
                                                   ->additionalData("FoundPackages")
                                                   .value<ModuleHash>();
                        QStringList qtPackages;
                        for (auto it = all.begin(); it != all.end(); ++it) {
                            const QString name = it.key();
                            const FilePath cmakePath = it.value().targetFilePath;
                            if (name.size() > 4 && name.startsWith("Qt") && name[2].isDigit()
                                && name[3].isUpper() && !name.endsWith("plugin", Qt::CaseInsensitive)
                                && cmakePath.isChildOf(qtLibPath))
                                qtPackages.append(name);
                        }
                        if (qtPackages.isEmpty())
                            return;
                        const QString json = "{\"projectid\":\"" + projectId(project)
                                             + "\",\"qtmodules\":[\"" + qtPackages.join("\",\"")
                                             + "\"],\"qtversion\":\""
                                             + qtVersion->qtVersion().toString() + "\"}";
                        qCDebug(qtmodulesLog) << qPrintable(json);
                        addEvent(tracker, "QtModules", json);
                    });
                });
    }
};

class QmlModules : public QObject
{
    Q_OBJECT

public:
    using ScanStorage = Tasking::Storage<std::unique_ptr<TemporaryFilePath>>;

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
                    Tasking::Group{
                        Tasking::sequential,
                        storage,
                        createResponseFile(storage, qmlimportscanner, qmlFiles, importPaths),
                        runQmlImportScanner(storage, qmlimportscanner, id, qtVersionString, tracker)});
            });
    }

    Tasking::ExecutableItem createResponseFile(
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
                return Tasking::DoneResult::Error;
            }
            storage->reset(*result);
            return Tasking::DoneResult::Success;
        };
        return AsyncTask<Result<TemporaryFilePath *>>(setup, done);
    }

    Tasking::ExecutableItem runQmlImportScanner(
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
            const QString json = "{\"projectid\":\"" + projectId + "\",\"qmlmodules\":[\""
                                 + moduleHashes.join("\",\"") + "\"],\"qtversion\":\""
                                 + qtVersionString + "\"}";
            qCDebug(qmlmodulesLog) << qPrintable(json);
            addEvent(tracker, "QmlModules", json);
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
    Tasking::MappedTaskTreeRunner<Project *> m_runner;
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
                            const QString json = "{\"projectid\":\"" + projectId(project)
                                                 + "\",\"qtexample\":\"" + exampleHash
                                                 + "\",\"qtversion\":\""
                                                 + qtVersion->qtVersion().toString() + "\"}";
                            qCDebug(qtexampleLog) << qPrintable(json);
                            addEvent(tracker, "QtExample", json);
                            return;
                        }
                    });
                });
    }
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
        setOnFinish([] { theSettings().finish(); });
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
    if (!ICore::infoBar()->canInfoBeAdded(kInfoBarId) || theSettings().trackingEnabled.value())
        return;
    static auto infoText = UsageStatisticPlugin::tr(
                               "We make %1 for you. Would you like to help us make it even better?")
                               .arg(QGuiApplication::applicationDisplayName());
    static auto configureButtonInfoText = UsageStatisticPlugin::tr("Configure usage statistics...");
    static auto cancelButtonInfoText = UsageStatisticPlugin::tr("Decide later");

    ::Utils::InfoBarEntry entry(kInfoBarId, infoText, InfoBarEntry::GlobalSuppression::Enabled);
    entry.addCustomButton(configureButtonInfoText, [] {
        ICore::infoBar()->removeInfo(kInfoBarId);
        ICore::showOptionsDialog(kSettingsPageId);
    });
    entry.setCancelButtonInfo(cancelButtonInfoText, {});
    ICore::infoBar()->addInfo(entry);
}

void UsageStatisticPlugin::createProviders()
{
    // startup configs first, otherwise they will be attributed to the UI state
    m_providers.push_back(std::make_unique<Theme>(m_tracker.get()));
    // module and example telemetry require QInsightTracker::contextData to
    // work reliably, because the key of QInsightTracker::interaction is limited to 255 characters.
#if QT_VERSION >= QT_WITH_CONTEXTDATA
    m_providers.push_back(std::make_unique<QtModules>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QtExample>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QmlModules>(m_tracker.get()));
#endif

    m_providers.push_back(std::make_unique<UILanguage>(m_tracker.get()));

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
