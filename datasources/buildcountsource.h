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

#include <QCoreApplication>

#include <KUserFeedback/AbstractDataSource>

namespace UsageStatistic {
namespace Internal {

//! Tracks succeeded, failed, and total builds count
class BuildCountSource : public KUserFeedback::AbstractDataSource
{
    Q_DECLARE_TR_FUNCTIONS(BuildCountDataSource)

public:
    BuildCountSource();

public: // AbstractDataSource interface
    QString name() const override;
    QString description() const override;

    /*! The output data format is:
     *  {
     *      "succeeded": int,
     *      "failed"   : int,
     *      "total"    : int
     *  }
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

private:
    static constexpr quint64 succeededCountDflt() { return 0; }
    static constexpr quint64 failedCountDflt() { return 0; }

private:
    quint64 m_succeededBuildsCount = succeededCountDflt();
    quint64 m_failedBuildsCount    = failedCountDflt();
};

} // namespace Internal
} // namespace UsageStatistic
