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
#include "servicesource.h"

#include <QtCore/QDateTime>
#include <QtCore/QSettings>

#include "common/utils.h"
#include "common/scopedsettingsgroupsetter.h"

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

ServiceSource::ServiceSource(std::shared_ptr<KUserFeedback::Provider> provider)
    : AbstractDataSource(QStringLiteral("serviceData"), Provider::BasicSystemInformation)
    , m_provider(std::move(provider))
{
}

QString ServiceSource::name() const
{
    return tr("Service data");
}

QString ServiceSource::description() const
{
    return tr("Additional technical things to make data processing more reliable and useful");
}

static QString documentVersionKey() { return QStringLiteral("documentVersion"); }
static QString telemetryLevelKey() { return QStringLiteral("telemetryLevel"); }
static QString createdAtKey() { return QStringLiteral("createdAt"); }
static QString uuidKey() { return QStringLiteral("uuid"); }

static QString documentVersionString()
{
    static const auto versionString = [](){
        const Utils::DocumentVersion v;
        return QStringLiteral("%1.%2.%3").arg(v.major).arg(v.minor).arg(v.patch);
    }();
    return versionString;
}

static int telemetryLevel(const std::shared_ptr<KUserFeedback::Provider> &provider)
{
    if (!provider) {
        return -1;
    }

    switch (provider->telemetryMode()) {
        case Provider::BasicSystemInformation:
            return 1;
        case Provider::BasicUsageStatistics:
            return 2;
        case Provider::DetailedSystemInformation:
            return 3;
        case Provider::DetailedUsageStatistics:
            return 4;
        default:
            return -1;
    }
}

static QString createdAtString()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

QVariant ServiceSource::data()
{
    return QVariantMap{{documentVersionKey(), documentVersionString()},
                       {telemetryLevelKey(), telemetryLevel(m_provider)},
                       {createdAtKey(), createdAtString()},
                       {uuidKey(), m_uuid.toString(QUuid::WithoutBraces)}};
}

void ServiceSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_uuid = qvariant_cast<QUuid>(settings->value(uuidKey(), m_uuid));
}

void ServiceSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(uuidKey(), m_uuid);
}

} // Internal
} // UsageStatistic
