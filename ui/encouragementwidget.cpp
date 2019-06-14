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
#include "encouragementwidget.h"
#include "ui_encouragementwidget.h"

#include "coreplugin/icore.h"

#include <KUserFeedback/FeedbackConfigUiController>

#include "usagestatisticconstants.h"

namespace UsageStatistic::Internal {

using namespace KUserFeedback;

EncouragementWidget::EncouragementWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EncouragementWidget)
    , m_controller(std::make_unique<FeedbackConfigUiController>())
{
    ui->setupUi(this);

    connect(this, &EncouragementWidget::providerChanged,
            this, &EncouragementWidget::onProviderChanged);
    connect(ui->pbOptions, &QPushButton::clicked,
            this, &EncouragementWidget::showSettingsDialog);
}

std::shared_ptr<Provider> EncouragementWidget::provider() const
{
    return m_provider;
}

void EncouragementWidget::setProvider(std::shared_ptr<Provider> provider)
{
    m_provider = std::move(provider);

    Q_EMIT providerChanged(m_provider);
}

void EncouragementWidget::showEvent(QShowEvent *event)
{
    updateMessage();
    QWidget::showEvent(event);
}

void EncouragementWidget::showSettingsDialog()
{
    Core::ICore::showOptionsDialog(Constants::USAGE_STATISTIC_PAGE_ID, this);
}

void EncouragementWidget::updateMessage()
{
    if (m_provider) {
        auto modeIndex = m_controller->telemetryModeToIndex(m_provider->telemetryMode());

        auto description = m_controller->telemetryModeDescription(modeIndex);

        auto modeName = m_controller->telemetryModeName(modeIndex);
        description.prepend(tr("Telemetry mode: %1.\n").arg(modeName));

        ui->lblMsg->setText(description);
    }
}

void EncouragementWidget::onProviderChanged(const std::shared_ptr<Provider> &p)
{
    m_controller->setFeedbackProvider(p.get());

    connect(m_provider.get(), &Provider::telemetryModeChanged,
            this, &EncouragementWidget::updateMessage);
    updateMessage();
}

EncouragementWidget::~EncouragementWidget() = default;

} // namespace UsageStatistic::Internal
