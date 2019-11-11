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

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>

#include <KUserFeedback/AbstractDataSource>

namespace UsageStatistic {
namespace Internal {

//! Base class for all time trackers
class TimeUsageSourceBase : public QObject, public KUserFeedback::AbstractDataSource
{
    Q_OBJECT

public:
    ~TimeUsageSourceBase() override;

public:  // AbstractDataSource interface
    /*! The output data format is:
     *  {
     *      "startCount": int,
     *      "usageTime" : int
     *  }
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

protected:
    TimeUsageSourceBase(const QString &id);

    void onStarted();
    void onStopped();

    bool isTimeTrackingActive() const;

Q_SIGNALS:
    void start();
    void stop();

private:
    static constexpr qint64 usageTimeDflt() { return 0; }
    static constexpr qint64 startCountDflt() { return 0; }

private:
    QElapsedTimer m_timer;
    qint64 m_usageTime = usageTimeDflt();
    qint64 m_startCount = startCountDflt();
};

} // namespace Internal
} // namespace UsageStatistic
