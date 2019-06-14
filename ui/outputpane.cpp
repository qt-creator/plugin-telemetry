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
#include "outputpane.h"

#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtCore/QDebug>

#include "encouragementwidget.h"

namespace UsageStatistic::Internal {

OutputPane::OutputPane() = default;

OutputPane::~OutputPane() = default;

QWidget *OutputPane::outputWidget(QWidget *parent)
{
    if (!m_outputPaneWidget) {
        createEncouragementWidget(parent);
    }

    return m_outputPaneWidget;
}

QList<QWidget *> OutputPane::toolBarWidgets() const
{
    return {};
}

QString OutputPane::displayName() const
{
    return tr("Feedback");
}

int OutputPane::priorityInStatusBar() const
{
    return 1;
}

void OutputPane::clearContents() {}

void OutputPane::visibilityChanged(bool /*visible*/)
{
}

void OutputPane::setFocus() {}

bool OutputPane::hasFocus() const
{
    return false;
}

bool OutputPane::canFocus() const
{
    return true;
}

bool OutputPane::canNavigate() const
{
    return false;
}

bool OutputPane::canNext() const
{
    return false;
}

bool OutputPane::canPrevious() const
{
    return false;
}

void OutputPane::goToNext() {}

void OutputPane::goToPrev() {}

void OutputPane::createEncouragementWidget(QWidget *parent)
{
    auto wgt = new EncouragementWidget(parent);
    connect(this, &OutputPane::providerChanged, wgt, &EncouragementWidget::setProvider);

    m_outputPaneWidget = wgt;
}

std::shared_ptr<KUserFeedback::Provider> OutputPane::provider() const
{
    return m_provider;
}

void OutputPane::setProvider(std::shared_ptr<KUserFeedback::Provider> provider)
{
    m_provider = std::move(provider);
    Q_EMIT providerChanged(m_provider);
}

} // namespace UsageStatistic::Internal
