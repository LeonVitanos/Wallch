/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2025, The Wallch Team.
 Original Authors (2010-2015): Alexandros Solanos, Leon Vitanos
 Modernization & New Code (2025-): Leon Vitanos

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>

class QMenu;
class QAction;
class QTimer;
class FeatureController;
class WallpaperManager;
class DialogHelper;

class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(FeatureController *featureController,
                         WallpaperManager *wallpaperManager,
                         DialogHelper *dialogHelper,
                         QObject *parent = nullptr);

    virtual ~TrayManager();

    void show();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private Q_SLOTS:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowWindowAction();
    void onWallpapersOnceAction();
    void onWallpapersPauseAction();
    void onWallpapersNextAction();
    void onWallpapersPreviousAction();

public Q_SLOTS:
    void updateMenu();
    void uncheckAllActions();

Q_SIGNALS:
    void showMainWindowRequested();
    void hideOrShowMainWindowRequested();
    void featureActionRequested(const QString &action);

private:
    void createActions();
    void createTrayIcon();

    FeatureController *m_featureController;
    WallpaperManager *m_wallpaperManager;
    DialogHelper *m_dialogHelper;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayIconMenu;
    QMenu *m_currentImageMenu;
    QTimer *m_trayWheelTimer;

    QAction *m_showWindowAction;
    QAction *m_openCurrentImageAction;
    QAction *m_openCurrentImageFolderAction;
    QAction *m_copyCurrentImagePathAction;
    QAction *m_copyCurrentImageAction;
    QAction *m_deleteCurrentImageAction;
    QAction *m_openCurrentImagePropertiesAction;
    QAction *m_wallpapersAction;
    QAction *m_wallpapersOnceAction;
    QAction *m_wallpapersPauseAction;
    QAction *m_wallpapersNextAction;
    QAction *m_wallpapersPreviousAction;
    QAction *m_liveEarthAction;
    QAction *m_pictureOfTheDayAction;
    QAction *m_liveWebsiteAction;
    QAction *m_preferencesAction;
    QAction *m_aboutAction;
    QAction *m_quitAction;
};

#endif // TRAYMANAGER_H
