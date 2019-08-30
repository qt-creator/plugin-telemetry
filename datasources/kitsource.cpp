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

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/gcctoolchain.h>
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
}

KitSource::~KitSource() = default;

QString KitSource::name() const
{
    return tr("Kits");
}

QString KitSource::description() const
{
    return tr("Kits usage statistic: compilers, debuggers, target architecture etc.");
}

static QString kitsInfoKey() { return QStringLiteral("kitsInfo"); }

static QString extractToolChainVersion(const ToolChain &toolChain)
{
    try {
        return dynamic_cast<const GccToolChain &>(toolChain).version();
    } catch (...) {
        return {};
    }
}

class KitInfo
{
public:
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
            m_map.insert(compilerKey,
                         QVariantMap{{nameKey(), toolChain->typeDisplayName()},
                                     {versionKey(), extractToolChainVersion(*toolChain)}});
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

    static KitInfo forKit(Kit &kit) { return KitInfo(kit); }

private: // Methods
    KitInfo(Kit &kit) : m_kit(kit) { addKitInfo(); }

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

private: // Data
    Kit &m_kit;
    QVariantMap m_map;
};

static QVariantList kitsInfo()
{
    QVariantList kitsInfoList;
    for (auto &&kit : KitManager::instance()->kits()) {
        if (kit && kit->isValid()) {
            kitsInfoList << KitInfo::forKit(*kit)
                            .withCompilerInfo()
                            .withDebuggerInfo()
                            .withQtVersionInfo()
                            .extract();
        }
    }

    return kitsInfoList;
}

QVariant KitSource::data()
{
    return QVariantMap{{kitsInfoKey(), kitsInfo()}};
}

} // namespace Internal
} // namespace UsageStatistic
