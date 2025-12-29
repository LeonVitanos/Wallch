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

#include "websitefeature.h"
#include "timermanager.h"
#include "wallpapermanager.h"
#include "glob.h"
#include <QImage>
#include <QDateTime>

WebsiteFeature::WebsiteFeature(TimerManager *timerManager,
                               WallpaperManager *wallpaperManager,
                               QObject *parent)
    : QObject(parent)
    , m_timerManager(timerManager)
    , m_wallpaperManager(wallpaperManager)
{
}

void WebsiteFeature::start()
{
    //getting the required values from the settings...

    /*
    timerManager_->secondsRemaining_=0;
    websiteSnapshot_ = new WebsiteSnapshot();

    gv.websiteWebpageToLoad=settings->value("website", "http://google.com").toString();
    gv.websiteInterval=settings->value("website_interval", 6).toInt();
    gv.websiteCropEnabled=settings->value("website_crop", false).toBool();
    gv.websiteCropArea=settings->value("website_crop_area", QRect(0, 0, gv.screenAvailableWidth, gv.screenAvailableHeight)).toRect();
    gv.websiteLoginEnabled=settings->value("website_login", false).toBool();
    gv.websiteLoginUsername=settings->value("website_username", "").toString();
    gv.websiteLoginPasswd=settings->value("website_password", "").toString();
    if(!gv.websiteLoginPasswd.isEmpty()){
        gv.websiteLoginPasswd=Global::base64Decode(gv.websiteLoginPasswd);
    }
    gv.websiteRedirect=settings->value("website_redirect", false).toBool();
    gv.websiteFinalPageToLoad=settings->value("website_final_webpage", "").toString();
    gv.websiteSimpleAuthEnabled=settings->value("website_simple_auth", false).toBool();
    gv.websiteWaitAfterFinishSeconds=settings->value("website_wait_after_finish", 3).toInt();
    gv.websiteJavascriptEnabled=settings->value("website_js_enabled", true).toBool();
    gv.websiteJavascriptCanReadClipboard=settings->value("website_js_can_read_clipboard", false).toBool();
    gv.websiteJavaEnabled=settings->value("website_java_enabled", false).toBool();
    gv.websiteLoadImages=settings->value("website_load_images", true).toBool();
    gv.websiteExtraUsernames=settings->value("website_extra_usernames", QStringList()).toStringList();
    gv.websiteExtraPasswords=settings->value("website_extra_passwords", QStringList()).toStringList();


    disconnect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));
    connect(websiteSnapshot_->asQObject(), SIGNAL(resultedImage(QImage*,short)), this, SLOT(liveWebsiteImageReady(QImage*,short)));

    websiteSnapshot_->setParameters(QUrl(gv.websiteWebpageToLoad), gv.screenAvailableWidth, gv.screenAvailableHeight);
    websiteSnapshot_->setWaitAfterFinish(gv.websiteWaitAfterFinishSeconds);
    websiteSnapshot_->setJavascriptConfig(gv.websiteJavascriptEnabled, gv.websiteJavascriptCanReadClipboard);
    websiteSnapshot_->setJavaEnabled(gv.websiteJavaEnabled);
    websiteSnapshot_->setLoadImagesEnabled(gv.websiteLoadImages);
    websiteSnapshot_->setCrop(gv.websiteCropEnabled, gv.websiteCropArea);

    if(gv.websiteLoginEnabled){
        if(!gv.websiteRedirect || gv.websiteFinalPageToLoad.isEmpty()){
            gv.websiteFinalPageToLoad=gv.websiteWebpageToLoad;
        }
        if(gv.websiteSimpleAuthEnabled){
            websiteSnapshot_->setSimpleAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
        }
        else
        {
            if(gv.websiteExtraUsernames.count()>0 || gv.websiteExtraPasswords.count()>0)
            {
                websiteSnapshot_->setComplexAuthenticationWithPossibleFields(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad, gv.websiteExtraUsernames, gv.websiteExtraPasswords, true);
            }
            else
            {
                websiteSnapshot_->setComplexAuthentication(gv.websiteLoginUsername, gv.websiteLoginPasswd, gv.websiteFinalPageToLoad);
            }
        }
    }
    else
    {
        websiteSnapshot_->disableAuthentication();
    }

    websiteSnapshot_->setTimeout(WEBSITE_TIMEOUT);

    timerManager_->totalSeconds_=Global::websiteSliderValueToSeconds(gv.websiteInterval);
    if(!gv.firstTimeout){
        websiteSnapshot_->start();
    }
    if(!timerManager_->secondsRemaining_){
        timerManager_->secondsRemaining_=timerManager_->totalSeconds_;
    }
    Global::resetSleepProtection(timerManager_->secondsRemaining_);
    timerManager_->saveSecondsLeftNow();
    timerManager_->start();*/
}

void WebsiteFeature::stop()
{
    /*
    m_timerManager->secondsRemaining_ = 0;
    if (m_timerManager->isActive()) {
        m_timerManager->stop();
    }
    m_timerManager->saveSecondsLeftNow(false);
    m_websiteSnapshot->stop();
    */
}

void WebsiteFeature::onImageReady(QImage *image, short errorCode)
{
    if (errorCode == 0) {
        // no error!
        Global::remove(gv.wallchHomePath + LW_IMAGE + "*");
        QString filename = gv.wallchHomePath + LW_IMAGE + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";

        image->save(filename);
        delete image;

        m_wallpaperManager->setBackground(filename, true, true, 5);
        QFile::remove(gv.wallchHomePath + LW_PREVIEW_IMAGE);
        QFile(filename).link(gv.wallchHomePath + LW_PREVIEW_IMAGE);
    } else {
        switch (errorCode) {
        case 1:
            Global::error("Some of the requested pages failed to load successfully.");
            break;
        case 2:
            Global::error("Simple authentication failed. Please check your username and/or password.");
            break;
        case 3:
            Global::error("Username and/or password fields are not found. Please check that you are pointing at the login page.");
            break;
        case 4:
            Global::error("The timeout has been reached and the image has yet to be created!");
            break;
        default:
            Global::error("Unknown error! Please try with a different web page.");
            break;
        }
    }
}
