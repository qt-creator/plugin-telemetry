// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icore.h>
#include <memory>

#include <QAbstractButton>
#include <QtCore/QHash>

//KUserFeedback
#include <Provider>

namespace KUserFeedback { class FeedbackConfigUiController; }

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE
using namespace Core;

namespace UsageStatistic {
namespace Internal {

namespace Ui { class UsageStatisticSimpleWidget; }

// Telemetry simplified settings widget
class UsageStatisticSimpleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UsageStatisticSimpleWidget(std::shared_ptr<KUserFeedback::Provider> provider);
    ~UsageStatisticSimpleWidget() override;

private: // Methods
    void configure();
    void addPrivacyPolicyLink();

private: // Data
    std::unique_ptr<Ui::UsageStatisticSimpleWidget> ui;
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    std::unique_ptr<KUserFeedback::FeedbackConfigUiController> m_controller;
};

class SwitchButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit SwitchButton(QWidget *parent = nullptr);

    void toggle();
    void setActivated(bool activated);
    bool isActivated() const;

    QSize sizeHint() const override;

signals:
    void activated(bool activated);
    void toggled(bool activated);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_isOn = false;
    bool m_isPressed = false;

    const int m_switchHeight = 24;
    const int m_switchInnerMargin = 5;
    const int m_switchHandleSize = m_switchHeight - 2 * m_switchInnerMargin;
    const int m_switchWidth = 2 * m_switchHandleSize + 3 * m_switchInnerMargin;
};

} // namespace Internal
} // namespace UsageStatistic
