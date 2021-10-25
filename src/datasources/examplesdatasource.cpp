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
#include "examplesdatasource.h"

#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <common/scopedsettingsgroupsetter.h>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

ExamplesDataSource::ExamplesDataSource()
    : AbstractDataSource(QStringLiteral("examplesData"), Provider::DetailedUsageStatistics)
{
    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this, &ExamplesDataSource::updateOpenedExamples);

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::projectAdded,
            this, &ExamplesDataSource::updateOpenedExamples);

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::sessionLoaded,
            this, &ExamplesDataSource::updateOpenedExamples);
}

ExamplesDataSource::~ExamplesDataSource() = default;

QString ExamplesDataSource::name() const
{
    return tr("Examples");
}

QString ExamplesDataSource::description() const
{
    return tr("The list of examples opened by you. "
              "Only example paths are collected and sent, for example: "
              "widgets/mainwindows/application/application.pro");
}

static QString examplesKey() { return QStringLiteral("examples"); }

QVariant ExamplesDataSource::data()
{
    return QVariantMap{{examplesKey(), QVariant(m_examplePaths)}};
}

void ExamplesDataSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_examplePaths = settings->value(examplesKey()).toStringList();
}

void ExamplesDataSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(examplesKey(), QStringList(m_examplePaths));
}

void ExamplesDataSource::resetImpl(QSettings *settings)
{
    m_examplePaths.clear();
    storeImpl(settings);
}

static QString examplePathGroupName() { return QStringLiteral("examplePathGroup"); }

static QString examplePattern()
{
    return QStringLiteral("/Examples/Qt-(\\d+\\.*){1,3}/(?<%1>.*)$");
}

void ExamplesDataSource::updateOpenedExamples()
{
    QRegularExpression re(examplePattern().arg(examplePathGroupName()));
    for (auto project : ProjectExplorer::SessionManager::projects()) {
        if (project) {
            auto projectPath = QDir::fromNativeSeparators(project->projectFilePath().toString());
            const auto match = re.match(projectPath);
            if (match.hasMatch()) {
                QString sanitizedPath = match.captured(examplePathGroupName());
                if (!m_examplePaths.contains(sanitizedPath))
                    m_examplePaths.append(sanitizedPath);
            }
        }
    }
}

} // namespace Internal
} // namespace UsageStatistic


