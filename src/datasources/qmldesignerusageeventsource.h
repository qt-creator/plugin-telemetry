/****************************************************************************
**
** Copyright (C) 2020 The Qt Company
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

#include <KUserFeedback/AbstractDataSource>

#include <QHash>
#include <QMap>
#include <QQuickWidget>

namespace UsageStatistic {
namespace Internal {

class QmlDesignerUsageEventSource : public QObject, public KUserFeedback::AbstractDataSource
{
    Q_OBJECT

public:
    QmlDesignerUsageEventSource();

public:
    QString name() const override;
    QString description() const override;
    QString currentIdentifier; // feedback popup

    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

    void launchPopup(const QString &identifier); // launch user feedback popup

public slots:
    void handleUsageStatisticsNotifier(const QString &identifier);
    void handleUsageStatisticsUsageTimer(const QString &identifier, int elapsed);
    void insertFeedback(const QString &feedback, int rating);
    void closeFeedbackPopup();

private:
    QMap<QString, QVariant> m_eventData;
    QMap<QString, QVariant> m_timeData;

    // user feedback data
    QHash<QString, QVariant> m_feedbackTextData;
    QHash<QString, QVariant> m_feedbackRatingData;
    QHash<QString, QVariant> m_feedbackPoppedData;
    QPointer<QQuickWidget> m_feedbackWidget;
};

} // namespace Internal
} // namespace UsageStatistic
