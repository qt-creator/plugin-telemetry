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

#include <memory>

#include "usagestatistic_global.h"

#include <extensionsystem/iplugin.h>

namespace KUserFeedback { class Provider; }

namespace UsageStatistic {
namespace Internal {

class UsageStatisticPage;

//! Plugin for collecting and sending usage statistics
class UsageStatisticPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "UsageStatistic.json")

public:
    UsageStatisticPlugin();
    ~UsageStatisticPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void createUsageStatisticPage();
    void storeSettings();
    void restoreSettings();
    void createProvider();
    void showEncouragementMessage();
    void showFirstTimeMessage();

private:
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    std::unique_ptr<UsageStatisticPage> m_usageStatisticPage;
};

} // namespace Internal
} // namespace UsageStatistic
