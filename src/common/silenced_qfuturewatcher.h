/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2015, Alexandros Solanos, Leon Vitanos
 Copyright © 2025-2026, Leon Vitanos (Modernization)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef SILENCED_QFUTUREWATCHER_H
#define SILENCED_QFUTUREWATCHER_H

#include <QtGlobal>

// Workaround for a C++20 style warning (-Wtemplate-id-cdtor) present in the constructor
// of Qt's QFutureInterface<void> in Qt versions prior to 6.7.1.
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#if (defined(__GNUC__) && __GNUC__ >= 13) || (defined(__clang__) && __clang_major__ >= 18)
#pragma GCC diagnostic ignored "-Wtemplate-id-cdtor"
#endif
#endif
#endif

#include <QtCore/QFutureWatcher>

#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 1))
#if defined(Q_OS_LINUX) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic pop
#endif
#endif

#endif // SILENCED_QFUTUREWATCHER_H
