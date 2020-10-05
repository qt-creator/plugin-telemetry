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
#include "qmldesignerusagetimesource.h"

#include <QtCore/QFileInfo>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>

namespace UsageStatistic {
namespace Internal {

static QString currentModeName()
{
    if (auto modeManager = Core::ModeManager::instance()) {
        if (auto mode = modeManager->currentMode()) {
            return mode->displayName().toLower();
        }
    }

    return {};
}

QmlDesignerUsageTimeSource::QmlDesignerUsageTimeSource()
    : TimeUsageSourceBase(QStringLiteral("qmlDesignerUsageTime"))
{
    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
            this, [this](Utils::Id modeId){
                      updateTrackingState(QString::fromUtf8(modeId.name().toLower())); });

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, [this](){ updateTrackingState(currentModeName()); });
}

QmlDesignerUsageTimeSource::~QmlDesignerUsageTimeSource() = default;

QString QmlDesignerUsageTimeSource::name() const
{
    return tr("Qt Quick Designer usage time");
}

QString QmlDesignerUsageTimeSource::description() const
{
    return tr("How much time you spent editing QML files in Design mode.");
}

static bool isDesignMode(const QString &modeName)
{
    static const QString designModeName = QStringLiteral("design");
    return modeName == designModeName;
}

static bool editingQmlFile()
{
    static const QList<QString> validExtentions = {"qml", "ui.qml"};
    static const QString validMimeType = "qml";

    if (auto document = Core::EditorManager::currentDocument()) {
        QString mimeType = document->mimeType().toLower();
        QString extension = document->filePath().toFileInfo().completeSuffix().toLower();

        return validExtentions.contains(extension) || mimeType.contains(validMimeType);
    }

    return false;
}

void QmlDesignerUsageTimeSource::updateTrackingState(const QString &modeName)
{
    if (isDesignMode(modeName) && editingQmlFile() && !isTimeTrackingActive()) {
        Q_EMIT start();
    } else {
        Q_EMIT stop();
    }
}

} // namespace Internal
} // namespace UsageStatistic
