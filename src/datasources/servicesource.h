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

#include <QCoreApplication>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>

//KUserFeedback
#include <AbstractDataSource>
#include <Provider>

namespace UsageStatistic {
namespace Internal {

//! Additional technical data
class ServiceSource : public KUserFeedback::AbstractDataSource
{
    Q_DECLARE_TR_FUNCTIONS(ServiceSource)

public:
    ServiceSource(std::shared_ptr<KUserFeedback::Provider> provider);

public: // AbstractDataSource interface
    QString name() const override;

    QString description() const override;

    /*! The output data format is:
     *  {
     *      "createdAt": "2019-10-14T10:22:38",
     *      "documentVersion": "1.0.0",
     *      "telemetryLevel": 4,
     *      "uuid": "049e5987-32de-487c-9e35-39b1a1380329"
     *  }
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;

private:
    std::shared_ptr<KUserFeedback::Provider> m_provider;
    QUuid m_uuid = QUuid::createUuid();
};

} // Internal
} // UsageStatistic
