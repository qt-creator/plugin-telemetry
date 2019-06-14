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
#include "qtclicensesource.h"

#include <coreplugin/icore.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <KUserFeedback/Provider>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

QtcLicenseSource::QtcLicenseSource()
    : AbstractDataSource(QStringLiteral("qtcLicense"), Provider::DetailedUsageStatistics)
{}

QString QtcLicenseSource::description() const
{
    return tr("Qt Creator license type string: opensource, evaluation, commercial");
}

QString QtcLicenseSource::name() const
{
    return tr("Qt Creator license");
}

static const auto evaluationStr = QStringLiteral("evaluation");
static const auto opensourceStr = QStringLiteral("opensource");
static const auto commercialStr = QStringLiteral("commercial");

static bool isLicenseChecker(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec) {
        return false;
    }

    static const auto pluginName = QStringLiteral("LicenseChecker");
    return spec->name().contains(pluginName);
}

static bool hasEvaluationLicense(ExtensionSystem::IPlugin *plugin)
{
    if (!plugin) {
        return false;
    }

    bool isEvaluation = false;

    static const auto checkMethodName = "evaluationLicense";
    QMetaObject::invokeMethod(plugin, checkMethodName, Q_RETURN_ARG(bool, isEvaluation));

    return isEvaluation;
}

static QString licenseString()
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    if (auto it = std::find_if(plugins.begin(), plugins.end(), &isLicenseChecker); it != plugins.end()) {
        return hasEvaluationLicense((*it)->plugin()) ? evaluationStr: commercialStr;
    }

    return opensourceStr;
}

QVariant QtcLicenseSource::data()
{
    return QVariantMap{{"value", licenseString()}};
}

} // namespace Internal
} // namespace UsageStatistic
