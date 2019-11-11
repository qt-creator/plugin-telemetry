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
#include "buildsystemsource.h"

#include <QtCore/QSettings>
#include <QtCore/QCryptographicHash>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <KUserFeedback/Provider>

#include "common/scopedsettingsgroupsetter.h"

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

// Keep synchronized with BuildSystemSource::BuildSystem {
static const auto &buildSystemMarks()
{
    static const QString marks[] = {".qt4", ".cmake", ".qbs", ".autotools"};
    return marks;
}

static const auto &buildSystemKeys()
{
    static const QString keys[] = {"qmake", "cmake", "qbs", "autotools", "other"};
    return keys;
}
// }

static BuildSystemSource::BuildSystem extractBuildSystemType(const QString &name)
{
    auto begin = std::begin(buildSystemMarks());
    auto end = std::end(buildSystemMarks());

    auto it = std::find_if(begin, end, [&](const QString &m){ return name.contains(m); });
    return it != end ? BuildSystemSource::BuildSystem(std::distance(begin, it))
                     : BuildSystemSource::Other;
}

BuildSystemSource::BuildSystemSource()
    : AbstractDataSource(QStringLiteral("buildSystem"), Provider::DetailedUsageStatistics)
{
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectAdded,
            this, &BuildSystemSource::updateProjects);

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::sessionLoaded,
            this, &BuildSystemSource::updateProjects);
}

BuildSystemSource::~BuildSystemSource() = default;

QString BuildSystemSource::name() const
{
    return tr("Build system type");
}

QString BuildSystemSource::description() const
{
    return tr("Count of projects configured for a particular build system.");
}

QVariant BuildSystemSource::data()
{
    QVariantMap result;
    for (int i = QMake; i < Count; ++i) {
        result[buildSystemKeys()[i]] = m_projectsByBuildSystem[size_t(i)].count();
    }

    return result;
}

static QSet<QByteArray> fromVariantList(const QVariantList &vl)
{
    QSet<QByteArray> result;
    for (auto &&v : vl) {
        result << v.toByteArray();
    }

    return result;
}

void BuildSystemSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    for (int i = QMake; i < Count; ++i) {
        m_projectsByBuildSystem[size_t(i)] =
            fromVariantList(settings->value(buildSystemKeys()[i]).toList());
    }
}

static QVariantList toVariantList(const QSet<QByteArray> &set)
{
    QVariantList result;
    result.reserve(set.size());

    std::transform(set.begin(), set.end(), std::back_inserter(result),
                   [](const QByteArray &ba) { return QVariant::fromValue(ba); });

    return result;
}

void BuildSystemSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    for (int i = QMake; i < Count; ++i) {
        settings->setValue(
            buildSystemKeys()[i], toVariantList(m_projectsByBuildSystem[size_t(i)]));
    }
}

void BuildSystemSource::resetImpl(QSettings *settings)
{
    ProjectsByBuildSystem().swap(m_projectsByBuildSystem);
    storeImpl(settings);
}

static QByteArray hashPath(const Utils::FilePath& name)
{
    return QCryptographicHash::hash(name.toString().toUtf8(), QCryptographicHash::Md5);
}

void BuildSystemSource::updateProjects()
{
    for (auto project : ProjectExplorer::SessionManager::projects()) {
        if (project) {
            const auto projectName = QString::fromUtf8(project->id().name()).toLower();
            const auto projectPath = project->projectFilePath();
            m_projectsByBuildSystem[extractBuildSystemType(projectName)] << hashPath(projectPath);
        }
    }
}

} // namespace Internal
} // namespace UsageStatistic
