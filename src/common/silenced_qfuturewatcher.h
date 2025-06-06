#ifndef SILENCED_QFUTUREWATCHER_H
#define SILENCED_QFUTUREWATCHER_H

#include <QtGlobal>

// Workaround for a C++20 style warning (-Wtemplate-id-cdtor) present in the constructor
// of Qt's QFutureInterface<void> in Qt versions prior to 6.7.1.
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtemplate-id-cdtor"
#endif
#endif

#include <QtCore/QFutureWatcher>

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic pop
#endif
#endif

#endif // SILENCED_QFUTUREWATCHER_H
