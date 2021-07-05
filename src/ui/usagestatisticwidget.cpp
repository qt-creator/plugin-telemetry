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
#include "usagestatisticwidget.h"
#include "ui_usagestatisticwidget.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <KUserFeedback/FeedbackConfigUiController>
#include <KUserFeedback/AbstractDataSource>

#include "usagestatisticconstants.h"

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

UsageStatisticWidget::UsageStatisticWidget(std::shared_ptr<KUserFeedback::Provider> provider)
   : ui(std::make_unique<Ui::UsageStatisticWidget>())
   , m_provider(std::move(provider))
   , m_controller(std::make_unique<FeedbackConfigUiController>())
{
    ui->setupUi(this);

    configure();

    addPrivacyPolicyLink();
}

void UsageStatisticWidget::updateSettings()
{
    setupActiveStatuses();
    setupTelemetryModeUi();
}

UsageStatisticWidget::Settings UsageStatisticWidget::settings() const
{
    return {m_controller->telemetryIndexToMode(ui->cbMode->currentIndex()), m_activeStatusesById};
}

static constexpr int dataSourceIDRole = Qt::UserRole + 1;

void UsageStatisticWidget::configure()
{
    m_controller->setFeedbackProvider(m_provider.get());

    connect(ui->cbMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UsageStatisticWidget::updateTelemetryModeDescription);
    connect(ui->cbMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) { preserveCurrentActiveStatuses(); updateDataSources(index); });

    connect(ui->lvDataSources, &QListWidget::currentItemChanged,
            this, &UsageStatisticWidget::updateDataSource);
    connect(ui->lvDataSources, &QListWidget::itemChanged,
            this, &UsageStatisticWidget::updateActiveStatus);
}

void UsageStatisticWidget::setupTelemetryModeUi()
{
    ui->cbMode->clear();
    for (int i = 0; i < m_controller->telemetryModeCount(); ++i) {
        ui->cbMode->addItem(QString("%1 - %2").arg(i).arg(m_controller->telemetryModeName(i)));
    }

    ui->cbMode->setCurrentIndex(m_controller->telemetryModeToIndex(m_provider->telemetryMode()));
}

void UsageStatisticWidget::updateTelemetryModeDescription(int modeIndex)
{
   ui->pteTelemetryLvlDescription->setPlainText(m_controller->telemetryModeDescription(modeIndex));
}

static auto dataSourceToItem(const AbstractDataSource &ds,
                             const UsageStatisticWidget::ActiveStatusesById &activeStatusesById)
{
    auto item = std::make_unique<QListWidgetItem>(ds.name());

    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    const auto it = activeStatusesById.find(ds.id());
    if (it != std::end(activeStatusesById)) {
        item->setCheckState(*it ? Qt::Checked : Qt::Unchecked);
    } else {
        item->setCheckState(Qt::Checked);
    }

    item->setData(dataSourceIDRole, ds.id());

    return item;
}

void UsageStatisticWidget::updateDataSources(int modeIndex)
{
    ui->lvDataSources->clear();

    for (auto &&ds : m_provider->dataSources()) {
        if (ds && ds->telemetryMode() <= m_controller->telemetryIndexToMode(modeIndex)) {
            auto item = dataSourceToItem(*ds, m_activeStatusesById).release();
            ui->lvDataSources->addItem(item);

            if (m_currentItemId == ds->id()) {
                ui->lvDataSources->setCurrentItem(item);
            }
        }
    }
}

static QString collectedData(AbstractDataSource &ds)
{
    QByteArray result;
    auto variantData = ds.data();

    if (variantData.canConvert<QVariantMap>()) {
        result = QJsonDocument(QJsonObject::fromVariantMap(variantData.toMap())).toJson();
    }

    if (variantData.canConvert<QVariantList>()) {
        result = QJsonDocument(QJsonArray::fromVariantList(variantData.value<QVariantList>())).toJson();
    }

    return QString::fromUtf8(result);
}

void UsageStatisticWidget::updateDataSource(QListWidgetItem *item)
{
    if (item) {
        m_currentItemId = item->data(dataSourceIDRole).toString();

        if (auto ds = m_provider->dataSource(m_currentItemId)) {
            ui->pteDataSourceDescription->setPlainText(ds->description());
            ui->pteCollectedData->setPlainText(collectedData(*ds));
        }
    } else {
        ui->pteDataSourceDescription->clear();
        ui->pteCollectedData->clear();
    }
}

void UsageStatisticWidget::updateActiveStatus(QListWidgetItem *item)
{
    if (item) {
        auto id = item->data(dataSourceIDRole).toString();
        m_activeStatusesById[id] = item->checkState() == Qt::Checked;
    }
}

void UsageStatisticWidget::preserveCurrentActiveStatuses()
{
    for (int row = 0; row < ui->lvDataSources->count(); ++row) {
        if (auto item = ui->lvDataSources->item(row)) {
            auto id = item->data(dataSourceIDRole).toString();
            m_activeStatusesById[id] = item->checkState() == Qt::Checked;
        }
    }
}

void UsageStatisticWidget::setupActiveStatuses()
{
    m_activeStatusesById.clear();

    for (auto &&ds : m_provider->dataSources()) {
        if (ds) {
            m_activeStatusesById[ds->id()] = ds->isActive();
        }
    }
}

void UsageStatisticWidget::addPrivacyPolicyLink()
{
    const auto currentText = ui->lblPrivacyPolicy->text();
    const auto linkTemplate = QString("<a href=\"%1\">%2<\\a>");
    ui->lblPrivacyPolicy->setText(linkTemplate.arg(QLatin1String(Constants::PRIVACY_POLICY_URL), currentText));

    const auto tooltipTemplate = tr("Open %1");
    ui->lblPrivacyPolicy->setToolTip(tooltipTemplate.arg(Constants::PRIVACY_POLICY_URL));
}

UsageStatisticWidget::~UsageStatisticWidget() = default;

} // namespace Internal
} // namespace UsageStatistic
