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

#include <QtCore/QPointer>

#include <coreplugin/dialogs/ioptionspage.h>

namespace KUserFeedback { class Provider; }

namespace UsageStatistic {
namespace Internal {

class UsageStatisticWidget;

//! Settings page
class UsageStatisticPage : public QObject, Core::IOptionsPage
{
    Q_OBJECT

public:
    UsageStatisticPage(std::shared_ptr<KUserFeedback::Provider> provider);
    ~UsageStatisticPage() override;

public: // IOptionsPage interface
    QWidget *widget() override;
    void apply() override;
    void finish() override;

Q_SIGNALS:
    void settingsChanged();

private: // Data
    std::unique_ptr<UsageStatisticWidget> m_feedbackWidget;
    std::shared_ptr<KUserFeedback::Provider> m_provider;

private: // Methods
    void configure();
};

} // namespace Internal
} // namespace UsageStatistic
