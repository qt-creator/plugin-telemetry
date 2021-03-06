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

#include <QtCore/QString>

namespace UsageStatistic {
namespace Internal {
namespace Utils {

//! Secret key for authentication defined during building
constexpr auto secret() { return USP_AUTH_KEY; }

//! Base server URL defined during building
constexpr auto serverUrl() { return USP_SERVER_URL; }

/*! Data scheme version for the JSON document
 *
 *  Should be changed if you change the output data format,
 *  for example, change a key or add a new data source.
 */
struct DocumentVersion
{
    int major = 1;
    int minor = 0;
    int patch = 0;
};

} // namespace Utils
} // namespace Internal
} // namespace UsageStatistic
