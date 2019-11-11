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
#include <memory>

#include <QWidget>

namespace KUserFeedback {
class Provider;
class FeedbackConfigUiController;
}

namespace UsageStatistic {
namespace Internal {

namespace Ui { class EncouragementWidget; }

//! Widget for displaying current telemetry level at the output pane
class EncouragementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EncouragementWidget(QWidget *parent = nullptr);
    ~EncouragementWidget() override;

    std::shared_ptr<KUserFeedback::Provider> provider() const;
    void setProvider(std::shared_ptr<KUserFeedback::Provider> provider);

protected: // QWidget interface
    void showEvent(QShowEvent *event) override;

Q_SIGNALS:
    void providerChanged(const std::shared_ptr<KUserFeedback::Provider> &);

private Q_SLOTS:
    void showSettingsDialog();
    void updateMessage();
    void onProviderChanged(const std::shared_ptr<KUserFeedback::Provider> &p);

private:
    QScopedPointer<Ui::EncouragementWidget> ui;
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    std::unique_ptr<KUserFeedback::FeedbackConfigUiController> m_controller;
};

} // namespace Internal
} // namespace UsageStatistic
