// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/QVariantMap>

#include <KUserFeedback/AbstractDataSource>
#include <KUserFeedback/Provider>

namespace UsageStatistic {
namespace Internal {

//! Additional application data
class ApplicationSource : public KUserFeedback::AbstractDataSource
{
    Q_DECLARE_TR_FUNCTIONS(ApplicationSource);

public:
    ApplicationSource();

public: // AbstractDataSource interface
    QString name() const override;

    QString description() const override;

    /*! The output data format is:
     *  {
     *      "applicationName": "Qt Creator",
     *      "applicationVersion": "8.0.0"
     *  }
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
};

} // Internal
} // UsageStatistic
