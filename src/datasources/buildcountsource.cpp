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
#include "buildcountsource.h"

#include <QtCore/QSettings>

#include "common/scopedsettingsgroupsetter.h"

#include <projectexplorer/buildmanager.h>

//KUserFeedback
#include <Provider>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

BuildCountSource::BuildCountSource()
    : AbstractDataSource(QStringLiteral("buildsCount"), Provider::DetailedUsageStatistics)
{
    QObject::connect(ProjectExplorer::BuildManager::instance(),
                     &ProjectExplorer::BuildManager::buildQueueFinished,
                     [&](bool success){ ++(success ? m_succeededBuildsCount : m_failedBuildsCount); });
}

QString BuildCountSource::name() const
{
    return tr("Builds count");
}

QString BuildCountSource::description() const
{
    return tr("Count of succeeded and failed builds.");
}

static QString succeededKey() { return QStringLiteral("succeeded"); }
static QString failedKey() { return QStringLiteral("failed"); }
static QString totalKey() { return QStringLiteral("total"); }

QVariant BuildCountSource::data()
{
    return QVariantMap{{succeededKey(), m_succeededBuildsCount},
                       {failedKey(), m_failedBuildsCount},
                       {totalKey(), m_succeededBuildsCount + m_failedBuildsCount}};
}

void BuildCountSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_succeededBuildsCount = qvariant_cast<quint64>(settings->value(succeededKey(), succeededCountDflt()));
    m_failedBuildsCount = qvariant_cast<quint64>(settings->value(failedKey(), failedCountDflt()));
}

void BuildCountSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(succeededKey(), m_succeededBuildsCount);
    settings->setValue(failedKey(), m_failedBuildsCount);
}

void BuildCountSource::resetImpl(QSettings *settings)
{
    m_succeededBuildsCount = succeededCountDflt();
    m_failedBuildsCount = failedCountDflt();

    storeImpl(settings);
}

} // namespace Internal
} // namespace UsageStatistic
