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

#include <QVariantMap>

#include <KUserFeedback/AbstractDataSource>

namespace UsageStatistic {
namespace Internal {

//! Tracks which toolkits are used
class KitSource : public QObject, public KUserFeedback::AbstractDataSource
{
    Q_OBJECT

public:
    KitSource();
    ~KitSource() override;

public: // AbstractDataSource interface
    QString name() const override;
    QString description() const override;

    /*! The output data format is:
     *  {
     *      "kitsInfo": [
     *         {
     *             "compiler": {
     *                 "name"   : string,
     *                 "version": int,
     *                 "abi": string,
     *                 "buildSuccesses": int,
     *                 "buildFails": int
     *             },
     *             "debugger": {
     *                 "name"   : string,
     *                 "version": string,
     *             },
     *             "default": bool,
     *             "qt": {
     *                 "abis": [
     *                     {
     *                         "arch"        : string,
     *                         "binaryFormat": string,
     *                         "os"          : string,
     *                         "osFlavor"    : string,
     *                         "wordWidth"   : string
     *                     }
     *                 ],
     *                 "qmlCompiler" : bool,
     *                 "qmlDebugging": bool,
     *                 "version"     : string
     *             }
     *         },
     *      ]
     *  }
     *
     *  The version format if "\d+.\d+.\d+".
     */
    QVariant data() override;

private:
    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

    friend class KitInfo;
    QVariantMap m_buildSuccessesForToolchain;
    QVariantMap m_buildFailsForToolchain;
};

} // namespace Internal
} // namespace UsageStatistic
