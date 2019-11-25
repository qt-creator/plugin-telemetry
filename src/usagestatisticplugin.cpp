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

#include <KUserFeedback/Provider>
#include <KUserFeedback/ApplicationVersionSource>
#include <KUserFeedback/CompilerInfoSource>
#include <KUserFeedback/CpuInfoSource>
#include <KUserFeedback/LocaleInfoSource>
#include <KUserFeedback/OpenGLInfoSource>
#include <KUserFeedback/PlatformInfoSource>
#include <KUserFeedback/QPAInfoSource>
#include <KUserFeedback/QtVersionSource>
#include <KUserFeedback/ScreenInfoSource>
#include <KUserFeedback/StartCountSource>
#include <KUserFeedback/UsageTimeSource>
#include <KUserFeedback/StyleInfoSource>

#include "datasources/qtclicensesource.h"
#include "datasources/buildcountsource.h"
#include "datasources/buildsystemsource.h"
#include "datasources/modeusagetimesource.h"
#include "datasources/examplesdatasource.h"
#include "datasources/kitsource.h"
#include "datasources/qmldesignerusagetimesource.h"
#include "datasources/servicesource.h"

#include "services/datasubmitter.h"

#include "ui/usagestatisticpage.h"
#include "ui/outputpane.h"

#include "common/utils.h"

namespace UsageStatistic {
namespace Internal {

UsageStatisticPlugin::UsageStatisticPlugin() = default;

UsageStatisticPlugin::~UsageStatisticPlugin() = default;

bool UsageStatisticPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // We have to create a pane here because of OutputPaneManager internal initialization order
    createOutputPane();

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
    provider.addDataSource(new QtcLicenseSource);
    provider.addDataSource(new BuildCountSource);
    provider.addDataSource(new BuildSystemSource);
    provider.addDataSource(new ModeUsageTimeSource);
    provider.addDataSource(new ExamplesDataSource);
    provider.addDataSource(new KitSource);
    provider.addDataSource(new QmlDesignerUsageTimeSource);
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

    configureOutputPane();

    showFirstTimeMessage();

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag UsageStatisticPlugin::aboutToShutdown()
{
    storeSettings();

    return SynchronousShutdown;
}

void UsageStatisticPlugin::createUsageStatisticPage()
{
    m_usageStatisticPage = std::make_unique<UsageStatisticPage>(m_provider);

    connect(m_usageStatisticPage.get(), &UsageStatisticPage::settingsChanged,
            this, &UsageStatisticPlugin::storeSettings);
}

void UsageStatisticPlugin::createOutputPane()
{
    m_outputPane = std::make_unique<OutputPane>();
}

void UsageStatisticPlugin::configureOutputPane()
{
    Q_ASSERT(m_outputPane);

    m_outputPane->setProvider(m_provider);

    connect(m_provider.get(), &KUserFeedback::Provider::showEncouragementMessage,
            this, &UsageStatisticPlugin::showEncouragementMessage);
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
static constexpr int submissionIntervalDays() { return 10; }

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
    if (m_provider && runFirstTime(*m_provider)) {
        showEncouragementMessage();
    }
}

void UsageStatisticPlugin::showEncouragementMessage()
{
    if (m_outputPane) {
        m_outputPane->flash();
    }
}

} // namespace Internal
} // namespace UsageStatistic
