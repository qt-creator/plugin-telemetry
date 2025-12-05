// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QInsightTracker>

namespace UsageStatistic::Internal {

class QDSEventsHandler : public QObject
{
public:
    QDSEventsHandler(QInsightTracker* tracker);

private:
    QString m_lastIdentifier;
};

} // namespace UsageStatistic::Internal

