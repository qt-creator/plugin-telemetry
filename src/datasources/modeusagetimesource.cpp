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
#include "modeusagetimesource.h"

#include <QtCore/QSettings>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>

#include <KUserFeedback/Provider>

#include "common/scopedsettingsgroupsetter.h"

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

ModeUsageTimeSource::ModeUsageTimeSource()
    : AbstractDataSource(QStringLiteral("modeUsageTime"), Provider::DetailedUsageStatistics)
{
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
            this, &ModeUsageTimeSource::onCurrentModeIdChanged);
    onCurrentModeIdChanged(Core::ModeManager::currentModeId());
}

ModeUsageTimeSource::~ModeUsageTimeSource() = default;

QString ModeUsageTimeSource::name() const
{
    return tr("Mode usage time");
}

QString ModeUsageTimeSource::description() const
{
    return tr("How much time you spent working in different modes.");
}

// Keep synced with ModeUsageTimeSource::Mode {
static const auto &modeMarks()
{
    static const QString marks[] = {"welcome", "edit", "design", "debug", "project", "help", "other"};
    return marks;
}
// }

static ModeUsageTimeSource::Mode modeFromString(const QString &modeString)
{
    auto begin = std::begin(modeMarks());
    auto end = std::end(modeMarks());

    auto it = std::find_if(begin, end, [&](const QString &mark) { return modeString.contains(mark); });

    return it != end ? ModeUsageTimeSource::Mode(std::distance(begin, it))
                     : ModeUsageTimeSource::Other;
}

QVariant ModeUsageTimeSource::data()
{
    QVariantMap result;

    for (std::size_t i = Welcome; i < ModesCount; ++i) {
        result[modeMarks()[i]] = m_timeByModes[i];
    }

    return result;
}

void ModeUsageTimeSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    for (std::size_t i = Welcome; i < ModesCount; ++i) {
        m_timeByModes[i] = qvariant_cast<qint64>(settings->value(modeMarks()[i], timeDflt()));
    }
}

void ModeUsageTimeSource::storeImpl(QSettings *settings)
{
    storeCurrentTimerValue();

    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    for (std::size_t i = Welcome; i < ModesCount; ++i) {
        settings->setValue(modeMarks()[i], m_timeByModes[i]);
    }
}

void ModeUsageTimeSource::resetImpl(QSettings *settings)
{
    std::fill(std::begin(m_timeByModes), std::end(m_timeByModes), timeDflt());
    storeImpl(settings);
}

void ModeUsageTimeSource::setCurrentMode(ModeUsageTimeSource::Mode mode)
{
    m_currentMode = mode;
}

ModeUsageTimeSource::Mode ModeUsageTimeSource::currentMode() const
{
    return m_currentMode;
}

void ModeUsageTimeSource::onCurrentModeIdChanged(const Utils::Id &modeId)
{
    auto mode = modeFromString(QString::fromUtf8(modeId.name()).toLower().simplified());

    if (m_currentMode == mode) {
        return;
    }

    if (m_currentTimer.isValid() && m_currentMode < ModesCount) {
        m_timeByModes[m_currentMode] += m_currentTimer.elapsed();
        m_currentTimer.invalidate();
    }

    m_currentMode = mode;

    if (m_currentMode != ModesCount) {
        m_currentTimer.start();
    }
}

void ModeUsageTimeSource::storeCurrentTimerValue()
{
    if (m_currentTimer.isValid() && m_currentMode < ModesCount) {
        m_timeByModes[m_currentMode] += m_currentTimer.restart();
    }
}

} // namespace Internal
} // namespace UsageStatistic
