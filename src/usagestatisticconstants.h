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

#include <QString>

namespace UsageStatistic {
namespace Constants {

static const QString DATA_SOURCES_SETTINGS_GROUP = QStringLiteral("DataSources");

static constexpr char TELEMETRY_SETTINGS_CATEGORY_ID[] = "Telemetry";

static constexpr char USAGE_STATISTIC_PAGE_ID[] = "UsageStatistic";

static constexpr char PRIVACY_POLICY_URL[] = "https://www.qt.io/terms-conditions/#privacy";

static constexpr char ENC_MSG_INFOBAR_ENTRY_ID[] = "UsageStatisticPlugin_EncMsg_InfoBarEntry_Id";

} // namespace UsageStatistic
} // namespace Constants
