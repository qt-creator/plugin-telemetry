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
#include "usagestatisticconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/infobar.h>
#include <utils/algorithm.h>

//KUserFeedback
#include <Provider>
#include <ApplicationVersionSource>
#include <CompilerInfoSource>
#include <CpuInfoSource>
#include <LocaleInfoSource>
#include <OpenGLInfoSource>
#include <PlatformInfoSource>
#include <QPAInfoSource>
#include <QtVersionSource>
#include <ScreenInfoSource>
#include <StartCountSource>
#include <UsageTimeSource>
#include <StyleInfoSource>

#include "datasources/applicationsource.h"
#include "datasources/buildcountsource.h"
#include "datasources/buildsystemsource.h"
#include "datasources/examplesdatasource.h"
#include "datasources/kitsource.h"
#include "datasources/modeusagetimesource.h"
#include "datasources/qmldesignerusageeventsource.h"
#include "datasources/qmldesignerusagetimesource.h"
#include "datasources/qtclicensesource.h"
#include "datasources/servicesource.h"

#include "services/datasubmitter.h"

#include "ui/usagestatisticpage.h"

#include "common/utils.h"

#include <QGuiApplication>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace UsageStatistic {
namespace Internal {

UsageStatisticPlugin::UsageStatisticPlugin() = default;

UsageStatisticPlugin::~UsageStatisticPlugin() = default;

static bool telemetryLevelNotSet(const KUserFeedback::Provider &provider)
{
    return provider.telemetryMode() == KUserFeedback::Provider::NoTelemetry;
}

bool UsageStatisticPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    return true;
}

void UsageStatisticPlugin::extensionsInitialized()
{
}

static void addDefaultDataSources(KUserFeedback::Provider &provider)
{
    provider.addDataSource(new KUserFeedback::ApplicationVersionSource);
    provider.addDataSource(new KUserFeedback::CompilerInfoSource);
    provider.addDataSource(new KUserFeedback::CpuInfoSource);
    provider.addDataSource(new KUserFeedback::LocaleInfoSource);
    provider.addDataSource(new KUserFeedback::OpenGLInfoSource);
    provider.addDataSource(new KUserFeedback::PlatformInfoSource);
    provider.addDataSource(new KUserFeedback::QPAInfoSource);
    provider.addDataSource(new KUserFeedback::QtVersionSource);
    provider.addDataSource(new KUserFeedback::ScreenInfoSource);
    provider.addDataSource(new KUserFeedback::StartCountSource);
    provider.addDataSource(new KUserFeedback::UsageTimeSource);
    provider.addDataSource(new KUserFeedback::StyleInfoSource);
}

static void addQtCreatorDataSources(KUserFeedback::Provider &provider)
{
    provider.addDataSource(new ApplicationSource);
    provider.addDataSource(new QtcLicenseSource);
    provider.addDataSource(new BuildCountSource);
    provider.addDataSource(new BuildSystemSource);
    provider.addDataSource(new ModeUsageTimeSource);
    provider.addDataSource(new ExamplesDataSource);
    provider.addDataSource(new KitSource);
    provider.addDataSource(new QmlDesignerUsageTimeSource);
    provider.addDataSource(new QmlDesignerUsageEventSource(!telemetryLevelNotSet(provider)));
}

static void addServiceDataSource(const std::shared_ptr<KUserFeedback::Provider> &provider)
{
    if (provider) {
        provider->addDataSource(new ServiceSource(provider));
    }
}

bool UsageStatisticPlugin::delayedInitialize()
{
    // We should create the provider only after everything else
    // is initialised (e.g., setting organization name)
    createProvider();

    addDefaultDataSources(*m_provider);
    addQtCreatorDataSources(*m_provider);
    addServiceDataSource(m_provider);

    createUsageStatisticPage();

    restoreSettings();

    showFirstTimeMessage();
    submitDataOnFirstStart();

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag UsageStatisticPlugin::aboutToShutdown()
{
    if (m_provider)
        m_provider->submit();

    storeSettings();

    return SynchronousShutdown;
}

void UsageStatisticPlugin::useSimpleUi(bool val)
{
    m_simplifiedSettings = true;
}

void UsageStatisticPlugin::createUsageStatisticPage()
{
    // check if StudioUsageStatistic has created the settings page
    bool telemetryPageCreated = ::Utils::anyOf(IOptionsPage::allOptionsPages(), [](const auto& page) {
        return page->id() == "UsageStatistic";
    });

    if (telemetryPageCreated)
        return;

    m_usageStatisticPage = std::make_unique<UsageStatisticPage>(m_provider, m_simplifiedSettings);

    connect(m_usageStatisticPage->instance(),
            &SettingsSignals::settingsChanged,
            this,
            &UsageStatisticPlugin::storeSettings);
}

void UsageStatisticPlugin::storeSettings()
{
    if (m_provider) {
        m_provider->store();
    }
}

void UsageStatisticPlugin::restoreSettings()
{
    if (m_provider) {
        m_provider->load();
    }
}

static constexpr int encouragementTimeSec() { return 1800; }
static constexpr int encouragementIntervalDays() { return 1; }
static constexpr int submissionIntervalDays()
{
    return 1;
}

static KUserFeedback::Provider::TelemetryMode getTelemetryStatus()
{
    bool telemetrySettings = ICore::settings()->value("Telemetry", false).toBool();

    if (telemetrySettings)
        return KUserFeedback::Provider::DetailedUsageStatistics;

    return KUserFeedback::Provider::NoTelemetry;
}

void UsageStatisticPlugin::createProvider()
{
    Q_ASSERT(!m_provider);
    m_provider = std::make_shared<KUserFeedback::Provider>();

    m_provider->setFeedbackServer(QString::fromUtf8(Utils::serverUrl()));
    m_provider->setDataSubmitter(new DataSubmitter);

    m_provider->setApplicationUsageTimeUntilEncouragement(encouragementTimeSec());
    m_provider->setEncouragementDelay(encouragementTimeSec());
    m_provider->setEncouragementInterval(encouragementIntervalDays());

    m_provider->setSubmissionInterval(submissionIntervalDays());

    if (m_simplifiedSettings)
        m_provider->setTelemetryMode(getTelemetryStatus());
}

static bool runFirstTime(const KUserFeedback::Provider &provider)
{
    static const auto startCountSourceId = QStringLiteral("startCount");
    if (auto startCountSource = provider.dataSource(startCountSourceId)) {
        auto data = startCountSource->data().toMap();

        static const auto startCountKey = QStringLiteral("value");
        const auto startCountIt = data.find(startCountKey);
        if (startCountIt != data.end()) {
            return startCountIt->toInt() == 1;
        }
    }

    return false;
}

void UsageStatisticPlugin::showFirstTimeMessage()
{
    if (m_provider && runFirstTime(*m_provider) && telemetryLevelNotSet(*m_provider)) {
        showEncouragementMessage();
    }
}

void UsageStatisticPlugin::submitDataOnFirstStart()
{
    /*
     * On first start submit data after 10 minutes.
     */

    if (m_provider && runFirstTime(*m_provider) && !telemetryLevelNotSet(*m_provider))
        QTimer::singleShot(1000 * 60 * 10, this, [this]() { m_provider->submit(); });
}

static ::Utils::InfoBarEntry makeInfoBarEntry()
{
    static auto infoText = UsageStatisticPlugin::tr(
                               "We make %1 for you. Would you like to help us make it even better?")
                               .arg(QGuiApplication::applicationDisplayName());
    static auto customButtonInfoText = UsageStatisticPlugin::tr("Adjust usage statistics settings");
    static auto cancelButtonInfoText = UsageStatisticPlugin::tr("Decide later");

    static auto hideEncouragementMessageCallback = []() {
        if (auto infoBar = Core::ICore::infoBar()) {
            infoBar->removeInfo(Constants::ENC_MSG_INFOBAR_ENTRY_ID);
        }
    };

    static auto showUsageStatisticsSettingsCallback = []() {
        hideEncouragementMessageCallback();
        Core::ICore::showOptionsDialog(Constants::USAGE_STATISTIC_PAGE_ID);
    };

    ::Utils::InfoBarEntry entry(Constants::ENC_MSG_INFOBAR_ENTRY_ID, infoText);
    entry.addCustomButton(customButtonInfoText, showUsageStatisticsSettingsCallback);
    entry.setCancelButtonInfo(cancelButtonInfoText, hideEncouragementMessageCallback);

    return entry;
}

void UsageStatisticPlugin::showEncouragementMessage()
{
    if (auto infoBar = Core::ICore::infoBar()) {
        if (!infoBar->containsInfo(Constants::ENC_MSG_INFOBAR_ENTRY_ID)) {
            static auto infoBarEntry = makeInfoBarEntry();
            infoBar->addInfo(infoBarEntry);
        }
    }
}

} // namespace Internal
} // namespace UsageStatistic
