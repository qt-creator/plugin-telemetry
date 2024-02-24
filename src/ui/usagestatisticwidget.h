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
#pragma once

#include <memory>

#include <QtWidgets/QWidget>
#include <QtCore/QHash>

//KUserFeedback
#include <Provider>

namespace KUserFeedback { class FeedbackConfigUiController; }

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace UsageStatistic {
namespace Internal {

namespace Ui { class UsageStatisticWidget; }

//! Telemetry settings widget
class UsageStatisticWidget : public QWidget
{
    Q_OBJECT

public: // Types
    using ActiveStatusesById = QHash<QString, bool>;

    struct Settings
    {
        KUserFeedback::Provider::TelemetryMode telemetryMode;
        ActiveStatusesById activeStatusesById;
    };

public:
    explicit UsageStatisticWidget(std::shared_ptr<KUserFeedback::Provider> provider);
    ~UsageStatisticWidget() override;

    void updateSettings();

    Settings settings() const;

private: // Methods
    void configure();

    void setupTelemetryModeUi();
    void updateTelemetryModeDescription(int modeIndex);

    void updateDataSources(int modeIndex);

    void updateDataSource(QListWidgetItem *item);
    void updateActiveStatus(QListWidgetItem *item);

    void preserveCurrentActiveStatuses();

    void setupActiveStatuses();

    void addPrivacyPolicyLink();

private: // Data
    std::unique_ptr<Ui::UsageStatisticWidget> ui;
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    std::unique_ptr<KUserFeedback::FeedbackConfigUiController> m_controller;

    QString m_currentItemId;
    ActiveStatusesById m_activeStatusesById;
};

} // namespace Internal
} // namespace UsageStatistic
