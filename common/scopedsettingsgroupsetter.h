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

#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace KUserFeedback { class AbstractDataSource; }

namespace UsageStatistic {
namespace Internal {

//! Used to set the specific set of group prefixes. Everything will be unset in the destructor
class ScopedSettingsGroupSetter
{
public:
    ScopedSettingsGroupSetter(QSettings &settings, const std::initializer_list<QString> &prefixes);

    ScopedSettingsGroupSetter(const ScopedSettingsGroupSetter &) = delete;
    ScopedSettingsGroupSetter(ScopedSettingsGroupSetter &&) = delete;

    ScopedSettingsGroupSetter &operator =(const ScopedSettingsGroupSetter &) = delete;
    ScopedSettingsGroupSetter &operator =(ScopedSettingsGroupSetter &&) = delete;

    ~ScopedSettingsGroupSetter();

    static ScopedSettingsGroupSetter forDataSource(const KUserFeedback::AbstractDataSource &ds,
                                                   QSettings &settings);

private:
    QSettings &m_settings;
    int m_prefixesCount;
};

} // namespace Internal
} // namespace UsageStatistic
