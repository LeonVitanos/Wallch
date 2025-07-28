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

#include "traymanager.h"

#include <QMenu>
#include <QTimer>
#include <QWheelEvent>
#include <QActionGroup>

#include "featurecontroller.h"
#include "wallpapermanager.h"
#include "dialoghelper.h"

TrayManager::TrayManager(FeatureController *featureController,
                         WallpaperManager *wallpaperManager,
                         DialogHelper *dialogHelper,
                         QObject *parent)
    : QObject(parent)
    , m_featureController(featureController)
    , m_wallpaperManager(wallpaperManager)
    , m_dialogHelper(dialogHelper)
    , m_trayIcon(nullptr)
{
    // A timer to not allow constant scrolling on the tray
    // TODO: CHECK THIS FEATURE
    m_trayWheelTimer = new QTimer(this);
    m_trayWheelTimer->setSingleShot(true);
}

TrayManager::~TrayManager()
{
}

void TrayManager::show()
{
    createActions();
    createTrayIcon();
    updateMenu();
    m_trayIcon->show();
}

void TrayManager::createActions()
{
    m_showWindowAction = new QAction(tr("Show"), this);
    connect(m_showWindowAction, &QAction::triggered, this, &TrayManager::onShowWindowAction);

    m_openCurrentImageAction = new QAction(tr("Open Image"), this);
    connect(m_openCurrentImageAction, &QAction::triggered, m_wallpaperManager, &WallpaperManager::openCurrentBackgroundImage);

    m_openCurrentImageFolderAction = new QAction(tr("Open Folder"), this);
    connect(m_openCurrentImageFolderAction, &QAction::triggered, m_wallpaperManager, &WallpaperManager::openCurrentBackgroundFolder);

    m_copyCurrentImageAction = new QAction(tr("Copy Image"), this);
    connect(m_copyCurrentImageAction, &QAction::triggered, m_wallpaperManager, &WallpaperManager::copyCurrentBackgroundImage);

    m_copyCurrentImagePathAction = new QAction(tr("Copy Path"), this);
    connect(m_copyCurrentImagePathAction, &QAction::triggered, m_wallpaperManager, &WallpaperManager::copyCurrentBackgroundPath);

    m_deleteCurrentImageAction = new QAction(tr("Delete"), this);
    connect(m_deleteCurrentImageAction, &QAction::triggered, m_wallpaperManager, &WallpaperManager::deleteCurrentBackgroundImage);

    m_openCurrentImagePropertiesAction = new QAction(tr("Properties"), this);
    connect(m_openCurrentImagePropertiesAction, &QAction::triggered, this, [=] { m_dialogHelper->showPropertiesDialog(); });

    m_wallpapersAction = new QAction(tr("Wallpapers"), this);
    connect(m_wallpapersAction, &QAction::triggered, this, [this]() { m_featureController->toggleFeature(FeatureController::Feature::Wallpapers); });

    m_wallpapersOnceAction = new QAction("    " + tr("Change wallpaper once"), this);
    connect(m_wallpapersOnceAction, &QAction::triggered, this, &TrayManager::onWallpapersOnceAction);

    m_wallpapersPauseAction = new QAction("    " + tr("Pause"), this);
    connect(m_wallpapersPauseAction, &QAction::triggered, this, &TrayManager::onWallpapersPauseAction);

    m_wallpapersNextAction = new QAction("    " + tr("Next"), this);
    connect(m_wallpapersNextAction, &QAction::triggered, this, &TrayManager::onWallpapersNextAction);

    m_wallpapersPreviousAction = new QAction("    " + tr("Previous"), this);
    connect(m_wallpapersPreviousAction, &QAction::triggered, this, &TrayManager::onWallpapersPreviousAction);

    m_liveEarthAction = new QAction(tr("Live Earth"), this);
    connect(m_liveEarthAction, &QAction::triggered, this, [this](){ m_featureController->toggleFeature(FeatureController::Feature::LiveEarth); });

    m_pictureOfTheDayAction = new QAction(tr("Picture Of The Day"), this);
    connect(m_pictureOfTheDayAction, &QAction::triggered, this, [this](){ m_featureController->toggleFeature(FeatureController::Feature::PictureOfTheDay); });

    m_liveWebsiteAction = new QAction(tr("Live Website"), this);
    connect(m_liveWebsiteAction, &QAction::triggered, this, [this](){ m_featureController->toggleFeature(FeatureController::Feature::Website); });

    m_preferencesAction = new QAction(tr("Preferences"), this);
    connect(m_preferencesAction, &QAction::triggered, m_dialogHelper, &DialogHelper::showPreferencesDialog);

    m_aboutAction = new QAction(tr("About"), this);
    connect(m_aboutAction, &QAction::triggered, m_dialogHelper, &DialogHelper::showAboutDialog);

    m_quitAction = new QAction(tr("Exit"), this);
    connect(m_quitAction, &QAction::triggered, this, [this]() { Q_EMIT featureActionRequested("--quit"); });
}

void TrayManager::createTrayIcon()
{
    m_trayIconMenu = new QMenu();
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayActivated);
    m_trayIcon->installEventFilter(this);
    m_trayIcon->setIcon(QIcon(":/images/wallch.png"));
    m_trayIcon->setToolTip("Wallch\nWallpaper changer");
}

void TrayManager::updateMenu()
{
    m_trayIconMenu->clear();
    m_trayIconMenu->addAction(m_showWindowAction);
    m_currentImageMenu = new QMenu("Current Image");
    m_currentImageMenu->addAction(m_openCurrentImageAction);
    m_currentImageMenu->addAction(m_openCurrentImageFolderAction);
    m_currentImageMenu->addAction(m_copyCurrentImagePathAction);
    m_currentImageMenu->addAction(m_copyCurrentImageAction);
    m_currentImageMenu->addAction(m_deleteCurrentImageAction);
    m_currentImageMenu->addAction(m_openCurrentImagePropertiesAction);
    m_trayIconMenu->addMenu(m_currentImageMenu);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_wallpapersAction);

    if (!m_featureController->isWallpapersRunning())
        m_trayIconMenu->addAction(m_wallpapersOnceAction);

    if (m_featureController->isWallpapersRunning()) {
        m_wallpapersAction->setCheckable(true);
        m_wallpapersAction->setChecked(true);
        if (m_featureController->wallpapersState() == WallpapersFeature::State::Paused) {
            m_wallpapersPauseAction->setText("    " + tr("Start"));
            m_trayIconMenu->addAction(m_wallpapersPauseAction);
        } else {
            m_wallpapersPauseAction->setText("    " + tr("Pause"));
            m_trayIconMenu->addAction(m_wallpapersPauseAction);
            m_trayIconMenu->addAction(m_wallpapersNextAction);
            m_trayIconMenu->addAction(m_wallpapersPreviousAction);
        }
    } else if (m_featureController->isLiveEarthRunning()) {
        m_liveEarthAction->setCheckable(true);
        m_liveEarthAction->setChecked(true);
    } else if (m_featureController->isPotdRunning()) {
        m_pictureOfTheDayAction->setCheckable(true);
        m_pictureOfTheDayAction->setChecked(true);
    } else if (m_featureController->isWebsiteRunning()) {
        m_liveWebsiteAction->setCheckable(true);
        m_liveWebsiteAction->setChecked(true);
    } else {
        m_wallpapersAction->setCheckable(false);
        m_liveEarthAction->setCheckable(false);
        m_pictureOfTheDayAction->setCheckable(false);
        m_liveWebsiteAction->setCheckable(false);
    }

    m_trayIconMenu->addAction(m_liveEarthAction);
    m_trayIconMenu->addAction(m_pictureOfTheDayAction);
    m_trayIconMenu->addAction(m_liveWebsiteAction);

    auto *myGroup = new QActionGroup(this);
    myGroup->addAction(m_wallpapersAction);
    myGroup->addAction(m_liveEarthAction);
    myGroup->addAction(m_pictureOfTheDayAction);
    myGroup->addAction(m_liveWebsiteAction);

    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_preferencesAction);
    m_trayIconMenu->addAction(m_aboutAction);
    m_trayIconMenu->addAction(m_quitAction);
}

bool TrayManager::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_trayIcon && event->type() == QEvent::Wheel) {
        if (m_trayWheelTimer->isActive()) {
            return false;
        }
        m_trayWheelTimer->start(500);

        bool scrolledUp = static_cast<QWheelEvent *>(event)->angleDelta().y() > 0;
        if (scrolledUp) {
            Q_EMIT featureActionRequested("--previous");
        } else {
            Q_EMIT featureActionRequested("--next");
        }
        return true;
    }
    return QObject::eventFilter(object, event);
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        Q_EMIT hideOrShowMainWindowRequested();
    }
}

// --- Action Handlers ---

void TrayManager::onShowWindowAction() { Q_EMIT showMainWindowRequested(); }
void TrayManager::onWallpapersOnceAction() { Q_EMIT featureActionRequested("--change"); }
void TrayManager::onWallpapersPauseAction() { Q_EMIT featureActionRequested("--pause"); }
void TrayManager::onWallpapersNextAction() { Q_EMIT featureActionRequested("--next"); }
void TrayManager::onWallpapersPreviousAction() { Q_EMIT featureActionRequested("--previous"); }
