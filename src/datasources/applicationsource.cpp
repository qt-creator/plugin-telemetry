// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "applicationsource.h"

#include <coreplugin/coreconstants.h>

#include "common/scopedsettingsgroupsetter.h"
#include "common/utils.h"

#include <QGuiApplication>
#include <QSettings>

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;

ApplicationSource::ApplicationSource()
    : AbstractDataSource("applicationData", Provider::BasicSystemInformation)
{}

QString ApplicationSource::name() const
{
    return tr("Application data");
}

QString ApplicationSource::description() const
{
    return tr("The name and version of the application.");
}

QVariant ApplicationSource::data()
{
    return QVariantMap{
        {"applicationName", QGuiApplication::applicationDisplayName()},
        {"applicationVersion", QGuiApplication::applicationVersion()},

    };
}

void ApplicationSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
}

void ApplicationSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
}

} // namespace Internal
} // namespace UsageStatistic
