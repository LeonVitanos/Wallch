#ifndef SILENCED_QTCONCURRENTRUN_H
#define SILENCED_QTCONCURRENTRUN_H

#include <QtGlobal>

// Workaround for a C++20 style warning (-Wtemplate-id-cdtor) present in the constructor
// of Qt's QFutureInterface<void> in Qt versions prior to 6.7.1.
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic push
#if (defined(__GNUC__) && __GNUC__ >= 13) || (defined(__clang__) && __clang_major__ >= 18)
#pragma GCC diagnostic ignored "-Wtemplate-id-cdtor"
#endif
#endif
#endif

#include <QtConcurrent/QtConcurrentRun>

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic pop
#endif
#endif

#endif // SILENCED_QTCONCURRENTRUN_H
