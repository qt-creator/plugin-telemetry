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

#include <array>

#include <QtCore/QElapsedTimer>

#include <utils/id.h>

//KUserFeedback
#include <AbstractDataSource>

namespace UsageStatistic {
namespace Internal {

//! Tracks which modes were used and for how long
class ModeUsageTimeSource : public QObject, public KUserFeedback::AbstractDataSource
{
    Q_OBJECT

public: // Types
    enum Mode {Welcome, Edit, Design, Debug, Projects, Help, Other, ModesCount};

public:
    ModeUsageTimeSource();
    ~ModeUsageTimeSource() override;

public: // AbstractDataSource interface
    QString name() const override;
    QString description() const override;

    /*! The output data format is:
     *  {
     *      "debug"  : int,
     *      "design" : int,
     *      "edit"   : int,
     *      "help"   : int,
     *      "other"  : int,
     *      "project": int,
     *      "welcome": int
     *  }
     *
     *  The value is duration in milliseconds.
     */
    QVariant data() override;

    void loadImpl(QSettings *settings) override;
    void storeImpl(QSettings *settings) override;
    void resetImpl(QSettings *settings) override;

private: // Data
    Mode m_currentMode = ModesCount;
    QElapsedTimer m_currentTimer;
    std::array<qint64, ModesCount> m_timeByModes{};

private: // Methods
    void setCurrentMode(Mode mode);
    Mode currentMode() const;

    void onCurrentModeIdChanged(const Utils::Id &modeId);

    void storeCurrentTimerValue();

    static constexpr qint64 timeDflt() { return 0; }
};

} // namespace Internal
} // namespace UsageStatistic
