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

#include <QtCore/QSet>

#include <KUserFeedback/AbstractDataSource>

namespace UsageStatistic {
namespace Internal {

//! Tracks which examples were opened by the user
class ExamplesDataSource : public QObject, public KUserFeedback::AbstractDataSource
{
    Q_OBJECT

public:
    ExamplesDataSource();
    ~ExamplesDataSource() override;

public: // AbstractDataSource interface
    QString name() const override;
    QString description() const override;

    /*! The output data format is:
     *  {
     *      "examples": []
     *  }
     *
     *  The array contains paths like "widgets/mainwindows/application/application.pro".
     *  Full paths aren't collected.
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

private: // Methods
    void updateOpenedExamples();

private: // Data
    QSet<QString> m_examplePaths;
};

} // namespace Internal
} // namespace UsageStatistic
