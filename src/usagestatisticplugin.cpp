// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usagestatisticplugin.h"
#include "coreplugin/actionmanager/actionmanager.h"

#ifdef BUILD_DESIGNSTUDIO
#include "qdseventshandler.h"
#endif

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/aspects.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/theme/theme.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

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
#include <QMetaEnum>
#include <QTimer>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

Q_LOGGING_CATEGORY(statLog, "qtc.usagestatistic", QtWarningMsg);
Q_LOGGING_CATEGORY(qtexampleLog, "qtc.usagestatistic.qtexample", QtWarningMsg);

const char kSettingsPageId[] = "UsageStatistic.PreferencesPage";

namespace UsageStatistic::Internal {

static UsageStatisticPlugin *m_instance = nullptr;

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
        tracker->interaction(":CONFIG:UILanguage", ICore::userInterfaceLanguage(), 0);
        const QStringList languages = QLocale::system().uiLanguages();
        tracker->interaction(":CONFIG:SystemLanguage",
                             languages.isEmpty() ? QString("Unknown") : languages.first(),
                             0);
    }
};

class Theme : public QObject
{
    Q_OBJECT
public:
    Theme(QInsightTracker *tracker)
    {
        tracker->interaction(":CONFIG:Theme",
                             creatorTheme() ? creatorTheme()->id() : QString("Unknown"),
                             0);
        const QString systemTheme = QString::fromUtf8(QMetaEnum::fromType<Qt::ColorScheme>().valueToKey(
                                                          int(Utils::Theme::systemColorScheme())))
                                        .toLower();
        tracker->interaction(":CONFIG:SystemTheme", systemTheme, 0);
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
                        tracker->interaction("QtModules", json, 0);
                    });
                });
    }
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
                            tracker->interaction("QtExample", json, 0);
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
        const QString helpUrl = ICore::isQtDesignStudio() ?
                          QString("qtdesignstudio/doc/studio-collecting-usage-statistics.html\">")
                        : QString("qtcreator/doc/creator-how-to-collect-usage-statistics.html\">");

        auto moreInformationLabel = new QLabel("<a href=\"qthelp://org.qt-project."
                                               + helpUrl
                                               + UsageStatisticPlugin::tr("More information")
                                               + "</a>");
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

static constexpr int submissionInterval()
{
    using namespace std::literals;
    return std::chrono::hours(1) / 1s;
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
            config->setSyncInterval(submissionInterval());
            config->setBatchSize(100);
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
    m_providers.push_back(std::make_unique<QtModules>(m_tracker.get()));
    m_providers.push_back(std::make_unique<QtExample>(m_tracker.get()));

    // not needed for QDS
    if (!ICore::isQtDesignStudio()) {
        m_providers.push_back(std::make_unique<UILanguage>(m_tracker.get()));

        // UI state last
        m_providers.push_back(std::make_unique<ModeChanges>(m_tracker.get()));
    }

#ifdef BUILD_DESIGNSTUDIO
    // handle events emitted from QDS
    m_providers.push_back(std::make_unique<QDSEventsHandler>(m_tracker.get()));
#endif

    for (const auto &provider : m_providers) {
        qCDebug(statLog) << "Created usage statistics provider"
                         << provider.get()->metaObject()->className();
    }
}

} // namespace UsageStatistic::Internal

#include "usagestatisticplugin.moc"
