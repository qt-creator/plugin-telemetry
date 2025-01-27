// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "qdseventshandler.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <qmldesigner/qmldesignerplugin.h>

namespace UsageStatistic::Internal {

static bool isQmlDesigner(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return false;

    return spec->name().contains("QmlDesigner");
}

QDSEventsHandler::QDSEventsHandler(QInsightTracker* tracker)
{
    using QmlDesigner::QmlDesignerPlugin;

    const auto plugins = ExtensionSystem::PluginManager::plugins();
    const auto it = std::find_if(plugins.begin(), plugins.end(), &isQmlDesigner);
    const QmlDesignerPlugin* qmlDesignerPlugin = qobject_cast<QmlDesignerPlugin*>((*it)->plugin());

    connect(qmlDesignerPlugin,
            &QmlDesignerPlugin::usageStatisticsNotifier,
            this,
            [&](QString identifier){
                tracker->interaction(identifier);
    });

    connect(qmlDesignerPlugin,
            &QmlDesignerPlugin::usageStatisticsUsageTimer,
            this,
            [&](QString identifier, int elapsed){
                tracker->transition(identifier);
    });

    connect(qmlDesignerPlugin,
            &QmlDesignerPlugin::usageStatisticsUsageDuration,
            this,
            [&](QString identifier){
                tracker->interaction(identifier);
    });

    connect(qmlDesignerPlugin,
            &QmlDesignerPlugin::usageStatisticsInsertFeedback,
            this,
            [&](QString identifier, QString feedback, int rating){
                QString textFeedback = "Feedback: ";

                if (feedback.isEmpty())
                    textFeedback += "empty";
                else
                    textFeedback += feedback;

                tracker->interaction(identifier, textFeedback, rating);
    });
}

} // namespace UsageStatistic::Internal
