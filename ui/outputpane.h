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
#include <QtWidgets/QWidget>

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
QT_END_NAMESPACE

namespace KUserFeedback { class Provider; }

namespace UsageStatistic::Internal {

class EncouragementWidget;

//! Output pane for displayed different different information (telemetry level, surveys, etc.)
class OutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    OutputPane();
    ~OutputPane() override;

public: // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;

    QList<QWidget *> toolBarWidgets() const override;

    QString displayName() const override;

    int priorityInStatusBar() const override;

    void clearContents() override;

    void visibilityChanged(bool visible) override;

    void setFocus() override;
    bool hasFocus() const override;

    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;

    void goToNext() override;
    void goToPrev() override;

    std::shared_ptr<KUserFeedback::Provider> provider() const;
    void setProvider(std::shared_ptr<KUserFeedback::Provider> provider);

Q_SIGNALS:
    void providerChanged(std::shared_ptr<KUserFeedback::Provider>);

private: // Methods
    void createEncouragementWidget(QWidget *parent);

private: // Data
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    QPointer<QWidget> m_outputPaneWidget;
};

} // namespace UsageStatistic::Internal

Q_DECLARE_METATYPE(std::shared_ptr<KUserFeedback::Provider>)
