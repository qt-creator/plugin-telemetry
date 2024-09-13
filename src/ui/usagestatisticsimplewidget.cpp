// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usagestatisticsimplewidget.h"
#include "ui_usagestatisticsimplewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

// KUserFeedback
#include <FeedbackConfigUiController>
#include <AbstractDataSource>

#include "usagestatisticconstants.h"
#include "utils/icon.h"

using namespace Core;
using namespace KUserFeedback;

namespace UsageStatistic {
namespace Internal {

UsageStatisticSimpleWidget::UsageStatisticSimpleWidget(std::shared_ptr<KUserFeedback::Provider> provider)
    : ui(std::make_unique<Ui::UsageStatisticSimpleWidget>())
    , m_provider(std::move(provider))
    , m_controller(std::make_unique<FeedbackConfigUiController>())
{
    ui->setupUi(this);

    configure();

    addPrivacyPolicyLink();
}

void UsageStatisticSimpleWidget::configure()
{
    m_controller->setFeedbackProvider(m_provider.get());

    bool originalTelemetryStatus = ICore::settings()->value("Telemetry", false).toBool();
    SwitchButton* switchButton = new SwitchButton(this);
    QLabel* enableTelemetry = new QLabel(this);

    enableTelemetry->setText(tr("Enable telemetry"));

    ui->toggleRow->layout()->addWidget(switchButton);
    ui->toggleRow->layout()->addWidget(enableTelemetry);

    switchButton->setActivated(originalTelemetryStatus);

    connect(switchButton, &SwitchButton::activated, this, [this, switchButton, originalTelemetryStatus](bool checked) {
        bool restartPending = ICore::askForRestart(tr("A restart is required for the changes to take effect."), tr("Cancel"));

        if (restartPending) {
            ICore::settings()->setValue("Telemetry", checked);
            ICore::restart();
        } else {
            switchButton->setActivated(originalTelemetryStatus);
        }
    });
}

void UsageStatisticSimpleWidget::addPrivacyPolicyLink()
{
    const auto currentText = ui->lblPrivacyPolicy->text();
    const auto linkTemplate = QString("<a href=\"%1\">%2</a>");  // Corrected the closing tag.
    ui->lblPrivacyPolicy->setText(linkTemplate.arg(QLatin1String(Constants::PRIVACY_POLICY_URL), currentText));

    const auto tooltipTemplate = tr("Open %1");
    ui->lblPrivacyPolicy->setToolTip(tooltipTemplate.arg(Constants::PRIVACY_POLICY_URL));
    ui->lblPrivacyPolicy->setOpenExternalLinks(true);
}

UsageStatisticSimpleWidget::~UsageStatisticSimpleWidget() = default;

SwitchButton::SwitchButton(QWidget *parent)
    : QAbstractButton(parent), m_isOn(false), m_isPressed(false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover); // Enables hover events for smoother UI transitions
}

void SwitchButton::toggle()
{
    setActivated(!m_isOn);
}

void SwitchButton::setActivated(bool activated)
{
    if (m_isOn != activated) {
        m_isOn = activated;
        emit toggled(m_isOn);
        update();
    }
}

bool SwitchButton::isActivated() const
{
    return m_isOn;
}

QSize SwitchButton::sizeHint() const
{
    return QSize(m_switchWidth, m_switchHeight);
}

void SwitchButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);

    QColor backgroundColor;
    qreal opacity = 1.0;

    if ((!m_isOn && m_isPressed) || (m_isOn && !m_isPressed))
        backgroundColor = Utils::creatorTheme()->color(Utils::Theme::DSinteraction);
    else
        backgroundColor = Utils::creatorTheme()->color(Utils::Theme::DSdisabledColor);

    painter.setBrush(backgroundColor);
    painter.setOpacity(opacity);

    QRectF backgroundRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.drawRoundedRect(backgroundRect, m_switchHeight / 2.0, m_switchHeight / 2.0);

    QRect handleRect = rect().adjusted(m_switchInnerMargin, m_switchInnerMargin, -m_switchInnerMargin, -m_switchInnerMargin);
    handleRect.setWidth(handleRect.height());

    if (isActivated()) {
        handleRect.moveRight(width() - m_switchInnerMargin);
        if (m_isPressed)
            handleRect.moveLeft(m_switchInnerMargin);
    } else {
        handleRect.moveLeft(m_switchInnerMargin);
        if (m_isPressed)
            handleRect.moveRight(width() - m_switchInnerMargin);
    }

    painter.setBrush(Qt::white);
    painter.setOpacity(1.0); // reset opacity for the handle
    painter.drawEllipse(handleRect);
}

void SwitchButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPressed = true;
        update();
    }
}

void SwitchButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isPressed) {
        m_isPressed = false;
        toggle();
        emit activated(m_isOn);
    }
}

} // namespace Internal
} // namespace UsageStatistic
