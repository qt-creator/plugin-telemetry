#pragma once

#include <QtGlobal>

#if defined(USAGESTATISTIC_LIBRARY)
#  define USAGESTATISTICSHARED_EXPORT Q_DECL_EXPORT
#else
#  define USAGESTATISTICSHARED_EXPORT Q_DECL_IMPORT
#endif