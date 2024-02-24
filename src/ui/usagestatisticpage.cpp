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
#include "usagestatisticpage.h"

//KUserFeedback
#include <AbstractDataSource>

#include "usagestatisticwidget.h"
#include "usagestatisticconstants.h"

#include <utils/theme/theme.h>

namespace UsageStatistic {
namespace Internal {

UsageStatisticPage::UsageStatisticPage(std::shared_ptr<KUserFeedback::Provider> provider)
    : m_provider(std::move(provider))
{
    configure();
}

UsageStatisticPage::~UsageStatisticPage() = default;

QWidget *UsageStatisticPage::widget()
{
    if (!m_feedbackWidget) {
        m_feedbackWidget = std::make_unique<UsageStatisticWidget>(m_provider);
        m_feedbackWidget->updateSettings();
    }

    return m_feedbackWidget.get();
}

static void applyDataSourcesActiveStatuses(const QHash<QString, bool> &statuses,
                                           const KUserFeedback::Provider &provider)
{
    for (auto &&ds : provider.dataSources()) {
        if (ds) {
            const auto it = statuses.find(ds->id());
            if (it != std::end(statuses)) {
                ds->setActive(*it);
            }
        }
    }
}

void UsageStatisticPage::apply()
{
    auto settings = m_feedbackWidget->settings();

    m_provider->setTelemetryMode(settings.telemetryMode);
    applyDataSourcesActiveStatuses(settings.activeStatusesById, *m_provider);

    Q_EMIT m_signals.settingsChanged();
}

void UsageStatisticPage::finish()
{
    m_feedbackWidget.reset();
}

void UsageStatisticPage::configure()
{
    setId(Constants::USAGE_STATISTIC_PAGE_ID);
    setCategory(Constants::TELEMETRY_SETTINGS_CATEGORY_ID);
    setCategoryIconPath(":/usagestatistic/images/settingscategory_usagestatistic.png");

    setDisplayName(tr("Usage Statistics"));
    setDisplayCategory(tr("Telemetry"));
}

} // namespace Internal
} // namespace UsageStatistic
