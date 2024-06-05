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
#include <QtQuick/QQuickItem>

//KUserFeedback
#include <Provider>

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

// For feedback popup
const char qmlDesignerFeedbackTextKey[] = "qmlDesignerFeedbackTextKey";
const char qmlDesignerFeedbackRatingKey[] = "qmlDesignerFeedbackRatingKey";
const char qmlDesignerFeedbackPoppedKey[] = "qmlDesignerFeedbackPoppedKey";

QmlDesignerUsageEventSource::QmlDesignerUsageEventSource(bool enabled)
    : KUserFeedback::AbstractDataSource("qmlDesignerUsageEvents", Provider::DetailedUsageStatistics)
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    const auto it = std::find_if(plugins.begin(), plugins.end(), &isQmlDesigner);
    if (it != plugins.end()) {
        const QObject *qmlDesignerPlugin = (*it)->plugin();
        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsNotifier(QString)),
                this,
                SLOT(handleUsageStatisticsNotifier(QString)));

        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsUsageTimer(QString, int)),
                this,
                SLOT(handleUsageStatisticsUsageTimer(QString, int)));

        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsUsageDuration(QString, int)),
                this,
                SLOT(handleUsageStatisticsUsageDuration(QString, int)));

        connect(qmlDesignerPlugin,
                SIGNAL(usageStatisticsInsertFeedback(QString, QString, int)),
                this,
                SLOT(insertFeedback(QString, QString, int)));

        connect(this,
                SIGNAL(launchPopup(QString)),
                qmlDesignerPlugin,
                SLOT(lauchFeedbackPopup(QString)));
    }
    m_enabled = enabled;
}

QString QmlDesignerUsageEventSource::name() const
{
    return tr("Qt Quick Designer usage of views and actions");
}

QString QmlDesignerUsageEventSource::description() const
{
    return tr("What views and actions are used in QML Design mode.");
}

void QmlDesignerUsageEventSource::insertFeedback(const QString &identifier,
                                                 const QString &feedback,
                                                 int rating)
{
    if (!feedback.isEmpty())
        m_feedbackTextData.insert(identifier, feedback);

    m_feedbackRatingData.insert(identifier, rating);
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

    if (it != m_timeData.end()) {
        it.value() = it.value().toInt() + elapsed;

        static const int timeLimit = 14400000; // 4 hours
        if (m_enabled && !m_feedbackPoppedData[identifier].toBool()
            && m_timeData.value(identifier).toInt() >= timeLimit) {
            m_feedbackPoppedData[identifier] = QVariant(true);
            emit launchPopup(identifier);
        }
    } else {
        m_timeData.insert(identifier, elapsed);
    }
}

void QmlDesignerUsageEventSource::handleUsageStatisticsUsageDuration(const QString &identifier,
                                                                     int elapsed)
{
    auto it = m_eventData.find(identifier);

    if (it != m_eventData.end()) {
        QVariantList list = it.value().toList();
        list.append(elapsed);
        it.value() = list;
    } else {
        QVariantList list;
        list.append(elapsed);
        m_eventData.insert(identifier, list);
    }
}

QVariant QmlDesignerUsageEventSource::data()
{
    return QVariantMap{{qmlDesignerEventsKey, m_eventData}, {qmlDesignerTimesKey, m_timeData},
           {qmlDesignerFeedbackTextKey, m_feedbackTextData}, {qmlDesignerFeedbackRatingKey, m_feedbackRatingData},
           {qmlDesignerFeedbackPoppedKey, m_feedbackPoppedData}};
}

void QmlDesignerUsageEventSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_eventData = settings->value(qmlDesignerEventsKey).toMap();
    m_timeData = settings->value(qmlDesignerTimesKey).toMap();
    m_feedbackTextData = settings->value(qmlDesignerFeedbackTextKey).toHash();
    m_feedbackRatingData = settings->value(qmlDesignerFeedbackRatingKey).toHash();
    m_feedbackPoppedData = settings->value(qmlDesignerFeedbackPoppedKey).toHash();
}

void QmlDesignerUsageEventSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(qmlDesignerEventsKey, m_eventData);
    settings->setValue(qmlDesignerTimesKey, m_timeData);
    settings->setValue(qmlDesignerFeedbackTextKey, m_feedbackTextData);
    settings->setValue(qmlDesignerFeedbackRatingKey, m_feedbackRatingData);
    settings->setValue(qmlDesignerFeedbackPoppedKey, m_feedbackPoppedData);
}

void QmlDesignerUsageEventSource::resetImpl(QSettings *settings)
{
    m_eventData.clear();
    storeImpl(settings);
}

} // namespace Internal
} // namespace UsageStatistic
