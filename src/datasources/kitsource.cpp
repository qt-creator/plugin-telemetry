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
#include "kitsource.h"

#include <QtCore/QSettings>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggeritem.h>

#include "common/scopedsettingsgroupsetter.h"

namespace UsageStatistic {
namespace Internal {

using namespace KUserFeedback;
using namespace ProjectExplorer;

KitSource::KitSource()
    : AbstractDataSource(QStringLiteral("kits"), Provider::DetailedUsageStatistics)
{
    QObject::connect(ProjectExplorer::BuildManager::instance(),
                     &ProjectExplorer::BuildManager::buildQueueFinished,
                     [&](bool success) {
        const Project *project = SessionManager::startupProject();
        const Target *target = project ? project->activeTarget() : nullptr;
        const Kit *kit = target ? target->kit() : nullptr;
        const ToolChain *toolChain = ToolChainKitAspect::toolChain(kit, Constants::CXX_LANGUAGE_ID);
        const Abi abi = toolChain ? toolChain->targetAbi() : Abi();
        const QString abiName = abi.toString();
        QVariantMap &bucket = success ? m_buildSuccessesForToolChain : m_buildFailsForToolChain;
        bucket[abiName] = bucket.value(abiName, 0).toInt() + 1;
    });
}

KitSource::~KitSource() = default;

QString KitSource::name() const
{
    return tr("Kits");
}

QString KitSource::description() const
{
    return tr("Kits usage statistics: compilers, debuggers, target architecture, and so on.");
}

static QString kitsInfoKey() { return QStringLiteral("kitsInfo"); }
static QString buildSuccessesKey() { return  QStringLiteral("buildSuccesses"); }
static QString buildFailsKey() { return QStringLiteral("buildFails"); }

static QString extractToolChainVersion(const ToolChain &toolChain)
{
    try {
        return dynamic_cast<const GccToolChain &>(toolChain).version();
    } catch (...) {
        return {};
    }
}

void KitSource::loadImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    m_buildSuccessesForToolChain = settings->value(buildSuccessesKey(), QVariantMap{}).toMap();
    m_buildFailsForToolChain = settings->value(buildFailsKey(), QVariantMap{}).toMap();
}

void KitSource::storeImpl(QSettings *settings)
{
    auto setter = ScopedSettingsGroupSetter::forDataSource(*this, *settings);
    settings->setValue(buildSuccessesKey(), m_buildSuccessesForToolChain);
    settings->setValue(buildFailsKey(), m_buildFailsForToolChain);
}

void KitSource::resetImpl(QSettings *settings)
{
    m_buildSuccessesForToolChain.clear();
    m_buildFailsForToolChain.clear();

    storeImpl(settings);
}

class KitInfo
{
public:
    KitInfo(Kit &kit, const KitSource &source) : m_kit(kit), m_source(source)
    {
        addKitInfo();
    }

    KitInfo &withQtVersionInfo()
    {
        static const QString qtKey = QStringLiteral("qt");
        static const QString qtQmlDebuggingSupportKey = QStringLiteral("qmlDebugging");
        static const QString qtQmlCompilerSupportKey = QStringLiteral("qmlCompiler");
        static const QString qtAbisKey = QStringLiteral("abis");

        if (auto qtVersion = QtSupport::QtKitAspect::qtVersion(&m_kit)) {
            QVariantMap qtData;
            qtData.insert(versionKey(), qtVersion->qtVersionString());
            qtData.insert(qtQmlDebuggingSupportKey, qtVersion->isQmlDebuggingSupported());
            qtData.insert(qtQmlCompilerSupportKey, qtVersion->isQtQuickCompilerSupported());
            qtData.insert(qtAbisKey, abisToVarinatList(qtVersion->qtAbis()));

            m_map.insert(qtKey, qtData);
        }

        return *this;
    }

    KitInfo &withCompilerInfo()
    {
        static const QString compilerKey = QStringLiteral("compiler");

        if (auto toolChain = ToolChainKitAspect::toolChain(&m_kit, Constants::CXX_LANGUAGE_ID)) {
            const QString abiName = toolChain->targetAbi().toString();
            m_map.insert(compilerKey, QVariantMap{
                {nameKey(), toolChain->typeDisplayName()},
                {abiKey(), abiName},
                {versionKey(), extractToolChainVersion(*toolChain)},
                {buildSuccessesKey(), m_source.m_buildSuccessesForToolChain.value(abiName).toInt()},
                {buildFailsKey(), m_source.m_buildFailsForToolChain.value(abiName).toInt()}
            });
        }

        return *this;
    }

    KitInfo &withDebuggerInfo()
    {
        static const QString debuggerKey = QStringLiteral("debugger");

        if (auto debuggerInfo = Debugger::DebuggerKitAspect::debugger(&m_kit)) {
            m_map.insert(debuggerKey,
                         QVariantMap{{nameKey(), debuggerInfo->engineTypeName()},
                                     {versionKey(), debuggerInfo->version()}});
        }

        return *this;
    }

    QVariantMap extract() const { return m_map; }

private: // Methods

    void addKitInfo()
    {
        static QString defaultKitKey = QStringLiteral("default");

        m_map.insert(defaultKitKey, &m_kit == KitManager::defaultKit());
    }

    static QVariantMap abiToVariantMap(const Abi &abi)
    {
        static const QString archKey = QStringLiteral("arch");
        static const QString osKey = QStringLiteral("os");
        static const QString osFlavorKey = QStringLiteral("osFlavor");
        static const QString binaryFormatKey = QStringLiteral("binaryFormat");
        static const QString wordWidthKey = QStringLiteral("wordWidth");

        return QVariantMap{{archKey        , Abi::toString(abi.architecture())},
                           {osKey          , Abi::toString(abi.os())          },
                           {osFlavorKey    , Abi::toString(abi.osFlavor())    },
                           {binaryFormatKey, Abi::toString(abi.binaryFormat())},
                           {wordWidthKey   , Abi::toString(abi.wordWidth())   }};
    }

    template <class Container>
    QVariantList abisToVarinatList(const Container &abis)
    {
        QVariantList result;
        for (auto &&abi : abis) {
            result << abiToVariantMap(abi);
        }

        return result;
    }

    static QString versionKey() { return QStringLiteral("version"); }
    static QString nameKey() { return QStringLiteral("name"); }
    static QString abiKey() { return QStringLiteral("abi"); }

private: // Data
    Kit &m_kit;
    const KitSource &m_source;
    QVariantMap m_map;
};

QVariant KitSource::data()
{
    QVariantList kitsInfoList;
    for (auto &&kit : KitManager::instance()->kits()) {
        if (kit && kit->isValid()) {
            kitsInfoList << KitInfo(*kit, *this)
                            .withCompilerInfo()
                            .withDebuggerInfo()
                            .withQtVersionInfo()
                            .extract();
        }
    }

    return QVariantMap{{kitsInfoKey(), kitsInfoList}};
}

} // namespace Internal
} // namespace UsageStatistic
