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
#include "qmldesignerusageeventsource.h"

#include "common/scopedsettingsgroupsetter.h"

#include <coreplugin/icore.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <KUserFeedback/Provider>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

static bool isQmlDesigner(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return false;

    return spec->name().contains("QmlDesigner");
}

const char qmlDesignerEventsKey[] = "qmlDesignerEvents";
const char qmlDesignerTimesKey[] = "qmlDesignerTimes";

QmlDesignerUsageEventSource::QmlDesignerUsageEventSource()
    : KUserFeedback::AbstractDataSource("qmlDesignerUsageEvents", Provider::DetailedUsageStatistics)
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    const auto it = std::find_if(plugins.begin(), plugins.end(), &isQmlDesigner);
    if (it != plugins.end()) {
        const QObject *qmlDesignerPlugin = (*it)->plugin();
        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsNotifier(const QString)),
                this,
                SLOT(handleUsageStatisticsNotifier(const QString)));
        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsUsageTimer(const QString, int)),
                this,
                SLOT(handleUsageStatisticsUsageTimer(const QString, int)));
    }
}

QString QmlDesignerUsageEventSource::name() const
{
    return tr("Qt Quick Designer usage of views and actions");
}

QString QmlDesignerUsageEventSource::description() const
{
    return tr("What views and actions are used in QML Design mode.");
}

void QmlDesignerUsageEventSource::handleUsageStatisticsNotifier(const QString &identifier)
{
    auto it = m_eventData.find(identifier);

    if (it != m_eventData.end())
        it.value() = it.value().toInt() + 1;
    else
        m_eventData.insert(identifier, 1);
}

void QmlDesignerUsageEventSource::handleUsageStatisticsUsageTimer(const QString &identifier, int elapsed)
{
    auto it = m_timeData.find(identifier);

    if (it != m_timeData.end())
        it.value() = it.value().toInt() + elapsed;
    else
        m_timeData.insert(identifier, elapsed);
}

QVariant QmlDesignerUsageEventSource::data()
{
    return QVariantMap{{qmlDesignerEventsKey, m_eventData}, {qmlDesignerTimesKey, m_timeData}};
}

void QmlDesignerUsageEventSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_eventData = settings->value(qmlDesignerEventsKey).toMap();
    m_timeData = settings->value(qmlDesignerTimesKey).toMap();
}

void QmlDesignerUsageEventSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(qmlDesignerEventsKey, m_eventData);
    settings->setValue(qmlDesignerTimesKey, m_timeData);
}

void QmlDesignerUsageEventSource::resetImpl(QSettings *settings)
{
    m_eventData.clear();
    storeImpl(settings);
}

} // namespace Internal
} // namespace UsageStatistic
