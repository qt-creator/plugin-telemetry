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
#include "timeusagesourcebase.h"

#include <QtCore/QVariant>
#include <QtCore/QSettings>

//KUserFeedback
#include <Provider>

#include <common/scopedsettingsgroupsetter.h>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

TimeUsageSourceBase::TimeUsageSourceBase(const QString &id)
    : AbstractDataSource(id, Provider::DetailedUsageStatistics)
{
    connect(this, &TimeUsageSourceBase::start, this, &TimeUsageSourceBase::onStarted);
    connect(this, &TimeUsageSourceBase::stop, this, &TimeUsageSourceBase::onStopped);
}

void TimeUsageSourceBase::onStarted()
{
    // Restart if required
    if (m_timer.isValid()) {
        onStopped();
    }

    ++m_startCount;
    m_timer.start();
}

void TimeUsageSourceBase::onStopped()
{
    if (m_timer.isValid()) {
        m_usageTime += m_timer.elapsed();
    }

    m_timer.invalidate();
}

bool TimeUsageSourceBase::isTimeTrackingActive() const
{
    return m_timer.isValid();
}

static QString usageTimeKey() { return QStringLiteral("usageTime"); }
static QString startCountKey() { return QStringLiteral("startCount"); }

QVariant TimeUsageSourceBase::data()
{
    return QVariantMap{{usageTimeKey(), m_usageTime}, {startCountKey(), m_startCount}};
}

void TimeUsageSourceBase::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_usageTime = settings->value(usageTimeKey(), usageTimeDflt()).toLongLong();
    m_startCount = settings->value(startCountKey(), startCountDflt()).toLongLong();
}

void TimeUsageSourceBase::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(usageTimeKey(), m_usageTime);
    settings->setValue(startCountKey(), m_startCount);
}

void TimeUsageSourceBase::resetImpl(QSettings *settings)
{
    m_usageTime = usageTimeDflt();
    m_startCount = startCountDflt();
    storeImpl(settings);
}

TimeUsageSourceBase::~TimeUsageSourceBase() = default;

} // namespace Internal
} // namespace UsageStatistic
