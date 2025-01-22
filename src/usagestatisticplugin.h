// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>

#include <extensionsystem/iplugin.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QInsightTracker;
QT_END_NAMESPACE

namespace UsageStatistic::Internal {

class UsageStatisticPage;

//! Plugin for collecting and sending usage statistics
class UsageStatisticPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "UsageStatistic.json")

public:
    UsageStatisticPlugin();
    ~UsageStatisticPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;

    void configureInsight();

private:
    void showInfoBar();

    void createProviders();

private:
    std::unique_ptr<QInsightTracker> m_tracker;
    std::vector<std::unique_ptr<QObject>> m_providers;
};

} // namespace UsageStatistic::Internal
