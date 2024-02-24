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
#include "scopedsettingsgroupsetter.h"

#include <QSettings>

//KUserFeedback
#include <AbstractDataSource>

#include "usagestatisticconstants.h"

namespace UsageStatistic {
namespace Internal {

ScopedSettingsGroupSetter::ScopedSettingsGroupSetter(QSettings &settings,
                                                     const std::initializer_list<QString> &prefixes)
    : m_settings(settings)
    , m_prefixesCount(int(prefixes.size()))
{
    for (auto &&prefix : prefixes) {
        m_settings.beginGroup(prefix);
    }
}

ScopedSettingsGroupSetter::~ScopedSettingsGroupSetter()
{
    while (m_prefixesCount-- > 0) {
        m_settings.endGroup();
    }
}

ScopedSettingsGroupSetter ScopedSettingsGroupSetter::forDataSource(
    const KUserFeedback::AbstractDataSource &ds, QSettings &settings)
{
    return ScopedSettingsGroupSetter(settings, {Constants::DATA_SOURCES_SETTINGS_GROUP, ds.id()});
}

} // namespace Internal
} // namespace UsageStatistic
