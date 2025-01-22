/****************************************************************************
**
** Copyright (C) 2019 The Qt Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of UsageStatistic plugin for Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "usagestatisticplugin.h"

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
#include <utils/theme/theme.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QGuiApplication>
#include <QInsightConfiguration>
#include <QInsightTracker>
#include <QTimer>

using namespace Core;
using namespace ExtensionSystem;
using namespace Utils;

Q_LOGGING_CATEGORY(statLog, "qtc.usagestatistic", QtWarningMsg);

const char kSettingsPageId[] = "UsageStatistic.PreferencesPage";

namespace UsageStatistic::Internal {

static UsageStatisticPlugin *m_instance = nullptr;

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
        const bool isDarkSystem = Utils::Theme::systemUsesDarkMode();
        tracker->interaction(":CONFIG:SystemTheme",
                             isDarkSystem ? QString("dark") : QString("light"),
                             0);
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
        auto moreInformationLabel = new QLabel("<a "
                                               "href=\"qthelp://org.qt-project.qtcreator/doc/"
                                               "creator-how-to-collect-usage-statistics.html\">"
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
        setCategoryIconPath(":/usagestatistic/images/settingscategory_usagestatistic.png");
        setDisplayName(UsageStatisticPlugin::tr("Usage Statistics"));
        setDisplayCategory(UsageStatisticPlugin::tr("Telemetry"));
        setCategoryIconPath(":/autotest/images/settingscategory_autotest.png");
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
        if (!m_tracker) {
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
