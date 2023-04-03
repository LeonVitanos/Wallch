/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define QT_NO_KEYWORDS
#include "websitesnapshot.h"

#include <QDebug>
#include <QPainter>
#include <QWebElement>
#include <QKeyEvent>
#include <QEvent>

const QStringList USUAL_USERNAME_FIELDS = QStringList() << "username" << "user" << "email" << "Email" << "User" << "Username"\
                                                        << "signin-email" << "userid" << "user-id" << "navbar_username" << "login_email" << "loginUsername" << "id_nickname" << "id_email" << "session_key-login"\
                                                        << "ap_email" << "signup_email";
const QStringList USUAL_PASSWORD_FIELDS = QStringList() << "password" << "pass" << "passwd" << "Password" << "Pass" << "Passwd"\
                                                        << "signin-password" << "navbar_password" << "login_password" << "navbar_password_hint" << "passid" << "pass-id" << "loginPassword" << "id_password" << "pwd"\
                                                        << "session_password-login" << "ap_password" << "signup_password";

WebsiteSnapshot::WebsiteSnapshot(){
    simpleAuthAlreadyDone_=false;
    currentlyLoading_=false;
    debug_=false;
    javascriptEnabled_=true;
    javascriptCanAccessClipboard_=false;
    javaEnabled_=false;
    loadImagesEnabled_=true;
    cropImage_=false;

    webPage_=NULL;

    minHeight_=minWidth_=600;
    timeout_=60;
    waitAfterFinish_=3;
    complexAuthInitialWait_=1;

    requestedUrl_=QUrl("");
    lastError_="No error.";
    errorCode_=0;

    authenticationLevel_=NoAuthentication;

    timeoutTimer_.setTimerType(Qt::VeryCoarseTimer);
    timeoutTimer_.setSingleShot(true);
    connect(&timeoutTimer_, SIGNAL(timeout()), this, SLOT(timeoutReached()));

    afterFinishTimer_.setTimerType(Qt::VeryCoarseTimer);
    afterFinishTimer_.setSingleShot(true);
    connect(&afterFinishTimer_, SIGNAL(timeout()), this, SLOT(afterFinishTimedOut()));

    priorComplexTimer_.setTimerType(Qt::VeryCoarseTimer);
    priorComplexTimer_.setSingleShot(true);
    connect(&priorComplexTimer_, SIGNAL(timeout()), this, SLOT(proceedToComplexAuth()));
}

WebsiteSnapshot::~WebsiteSnapshot(){
    if(debug_)
        dbg("Destructor.");
    if(webPage_){
        webPage_->settings()->clearMemoryCaches();
        webPage_->deleteLater();
    }
}

/*PRIVATE SLOTS/FUNCTIONS*/

void WebsiteSnapshot::returnResults(bool result){
    if(!result){
        if(debug_)
            dbg("The page could not be loaded correctly.");
        lastError_="The page could not be loaded correctly.";
        errorCode_=1;
        sendResultActions(NULL);
    }
    else
    {
        if(!webPage_->mainFrame()){
            if(debug_)
                dbg("Could not get a QWebFrame from the QWebPage.");
            lastError_="Could not get a QWebFrame from the QWebPage.";
            errorCode_=1;
            sendResultActions(NULL);
            return;
        }

        if(webPage_->viewportSize().isNull() || webPage_->viewportSize()==QSize(0, 0)){
            if(debug_)
                dbg("Viewport is null. Waiting for another loadFinished() signal. If no other signal is emitted, then the process will timeout.");
            return;
        }
        disconnectWebPage();
        if(debug_)
            dbg("Starting after finish timeout: "+QString::number(waitAfterFinish_)+" seconds");
        afterFinishTimer_.start(waitAfterFinish_*1000);
    }
}

void WebsiteSnapshot::afterAuthNavigation(bool result){
    if(!result){
        if(debug_)
            dbg("The page could not be loaded correctly.");
        lastError_="The page could not be loaded correctly.";
        errorCode_=1;
        sendResultActions(NULL);
    }
    else
    {
        disconnectWebPage();
        connect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(returnResults(bool)));
        webPage_->currentFrame()->load(urlAfterAuth_);
    }
}

void WebsiteSnapshot::afterFinishTimedOut(){
    timeoutTimer_.stop();
    webPage_->setViewportSize(webPage_->mainFrame()->contentsSize());
    QImage *image = new QImage(webPage_->viewportSize(), QImage::Format_ARGB32);
    if(image->isNull()){
        if(debug_)
            dbg("Resulting image is NULL. Waiting for another loadFinished() signal.");
        return;
    }

    QPainter painter(image);
    webPage_->mainFrame()->render(&painter);
    painter.end();

    if(cropImage_){
        *image=image->copy(cropRect_);
    }

    if(debug_) {
        dbg("Finished.");
    }

    sendResultActions(image);
}

void WebsiteSnapshot::parseUrl(QUrl &url){
    QString curUrl=url.url();
    if(!curUrl.startsWith("http://") && !curUrl.startsWith("https://")){
        curUrl="http://"+curUrl;
        url=QUrl(curUrl);
    }
}

void WebsiteSnapshot::authenticateSimple(QNetworkReply *, QAuthenticator *authenticator){
    if(simpleAuthAlreadyDone_){
        if(debug_)
            dbg("Simple authentication failed.");
        lastError_="Simple Authentication failed.";
        errorCode_=2;
        sendResultActions(NULL);
        return;
    }
    if(debug_)
        dbg("Trying to authenticate using simple authentication...");
    authenticator->setUser(authUsername_);
    authenticator->setPassword(authPassword_);
    simpleAuthAlreadyDone_=true;
}

void WebsiteSnapshot::complexAuthenticationRequired(bool success){
    if(debug_)
        dbg("Trying to authenticate using complex authentication...");
    if(!success){
        //the login page didn't load correctly...
        if(debug_)
            dbg("The (login) page could not be loaded correctly.");
        lastError_="The (login) page could not be loaded correctly.";
        errorCode_=1;
        sendResultActions(NULL);
        return;
    }

    if(debug_)
        dbg("Waiting "+QString::number(complexAuthInitialWait_)+" seconds prior to complex authentication...");
    priorComplexTimer_.start(complexAuthInitialWait_*1000);
}

void WebsiteSnapshot::proceedToComplexAuth(){
    //search for username/password fields.
    QWebElement document = webPage_->mainFrame()->documentElement();

    //searching for username...
    if(!searchAndSet(document, "input[type=text]", "id", authUsername_, usernameSearchFields_) && !searchAndSet(document, "input[type=email]", "id", authUsername_, usernameSearchFields_) && !searchAndSet(document, "input[type=username]", "id", authUsername_, usernameSearchFields_)){
        lastError_="The username and/or password fields were not found.";
        errorCode_=3;
        if(debug_)
            dbg("Could not find the username field.");
        sendResultActions(NULL);
        return;
    }
    //searching for password...
    if(!searchAndSet(document, "input[type=password]", "id", authPassword_, passwordSearchFields_)){
        lastError_="The username and/or password fields were not found.";
        errorCode_=3;
        if(debug_)
            dbg("Could not find the password field.");
        sendResultActions(NULL);
        return;
    }

    if(debug_)
        dbg("The username and password fields have been filled.");

    disconnectWebPage();
    webPage_->settings()->setAttribute(QWebSettings::AutoLoadImages, loadImagesEnabled_);
    decideFinalPage();

    if(debug_)
        dbg("Pressing [Return] to attempt login...");

    webPage_->event(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
}

void WebsiteSnapshot::decideFinalPage(){
    if(urlAfterAuth_==webPage_->currentFrame()->url()){
        if(debug_)
            dbg("The final url is the same with the log-in page url. The resulted page will be snapshoted.");
        //stay at the resulted page
        connect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(returnResults(bool)));
    }
    else
    {
        if(debug_)
            dbg("The final url is "+urlAfterAuth_.url()+". This is the url that will be loaded after logging in.");
        //have to go to another page
        connect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(afterAuthNavigation(bool)));
    }
}

void WebsiteSnapshot::dbg(const QString &string){
    qDebug() << "WebsiteSnapshot: "+string;
}

void WebsiteSnapshot::timeoutReached(){
    if(debug_)
        dbg("The timeout has been reached and the page has not finished loading.");
    lastError_="Timeout has been reached.";
    errorCode_=4;
    sendResultActions(NULL);
}

bool WebsiteSnapshot::searchAndSet(const QWebElement &document, const QString &searchFor, const QString &attribute, const QString &value, const QStringList &searchFields){
    if(debug_)
        dbg("Searching for '"+searchFor+"' with attribute '"+attribute+"'");
    QWebElementCollection collection;
    //searching username...
    collection = document.findAll(searchFor);
    short count=collection.count();
    QWebElement curElement;
    for(short i=0;i<count;i++){
        curElement=collection.at(i);
        if(curElement.hasAttribute(attribute)){
            if(searchFields.contains(curElement.attribute(attribute))){
                curElement.setAttribute("value", value);
                curElement.setFocus();
                return true;
                break;
            }
        }
    }
    return false;
}

void WebsiteSnapshot::sendResultActions(QImage *image){
    if(timeoutTimer_.isActive())
        timeoutTimer_.stop();
    if(afterFinishTimer_.isActive())
        afterFinishTimer_.stop();
    if(priorComplexTimer_.isActive())
        priorComplexTimer_.stop();
    disconnectWebPage();
    webPage_->triggerAction(QWebPage::Stop);
    webPage_->deleteLater();
    Q_EMIT resultedImage(image, errorCode_);
    currentlyLoading_=false;
}

void WebsiteSnapshot::webPageDestroyed(){
    webPage_=NULL;
}

void WebsiteSnapshot::initializeWebPage(){
    webPage_ = new CustomWebPage();
    connect(webPage_, SIGNAL(destroyed()), this, SLOT(webPageDestroyed()));

    QWebSettings *settings = webPage_->settings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, javascriptEnabled_);
    settings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, javascriptCanAccessClipboard_);
    settings->setAttribute(QWebSettings::JavaEnabled, javaEnabled_);
    settings->setAttribute(QWebSettings::AutoLoadImages, loadImagesEnabled_);
    settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, false);
    settings->setAttribute(QWebSettings::NotificationsEnabled, false);
    settings->setAttribute(QWebSettings::PluginsEnabled, false);
}

void WebsiteSnapshot::disconnectWebPage(){
    if(webPage_==NULL)
        return;
    disconnect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(returnResults(bool)));
    disconnect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(complexAuthenticationRequired(bool)));
    disconnect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(afterAuthNavigation(bool)));
    disconnect(webPage_->networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
               this, SLOT(authenticateSimple(QNetworkReply*,QAuthenticator*)));
}

/*PUBLIC FUNCTIONS*/

bool WebsiteSnapshot::start(){
    if(currentlyLoading_ || requestedUrl_==QUrl("") || !requestedUrl_.isValid()){
        return false;
    }
    else
    {
        if(debug_)
            dbg("Initializing webpage and starting the process...");
        lastError_="No error.";
        errorCode_=0;
        currentlyLoading_=true;
        if(webPage_!=NULL){
            disconnectWebPage();
            webPage_->deleteLater();
        }
        initializeWebPage();
        switch(authenticationLevel_){
        case NoAuthentication:
            connect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(returnResults(bool)));
            break;
        case SimpleAuthentication:
            simpleAuthAlreadyDone_=false;
            connect(webPage_->networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                    this, SLOT(authenticateSimple(QNetworkReply*,QAuthenticator*)));
            decideFinalPage();
            break;
        case ComplexAuthentication:
            //disable image loading on log-in pages.
            webPage_->settings()->setAttribute(QWebSettings::AutoLoadImages, false);
            connect(webPage_, SIGNAL(loadFinished(bool)), this, SLOT(complexAuthenticationRequired(bool)));
            break;
        default:
            break;
        }
        if(debug_)
            dbg("Setting viewport size to QSize("+QString::number(minWidth_)+", "+QString::number(minHeight_)+"), and loading "+requestedUrl_.url());
        webPage_->setViewportSize(QSize(minWidth_, minHeight_));
        webPage_->setPreferredContentsSize(QSize(minWidth_, minHeight_));
        webPage_->currentFrame()->load(requestedUrl_);
        timeoutTimer_.start(timeout_*1000);
        return true;
    }
}

void WebsiteSnapshot::stop(){
    if(timeoutTimer_.isActive())
        timeoutTimer_.stop();
    if(afterFinishTimer_.isActive())
        afterFinishTimer_.stop();
    if(priorComplexTimer_.isActive())
        priorComplexTimer_.stop();
    if(webPage_!=NULL){
        disconnectWebPage();
        webPage_->triggerAction(QWebPage::Stop);
        webPage_->deleteLater();
    }
    currentlyLoading_=false;
}

void WebsiteSnapshot::setParameters(QUrl url, const short &width, const short &height){
    if(isLoading())
        return;
    parseUrl(url);
    requestedUrl_=url;
    minWidth_=width;
    minHeight_=height;
}

void WebsiteSnapshot::setCrop(bool enabled, const QRect &rectCrop){
    cropImage_=enabled;
    cropRect_=rectCrop;
}

void WebsiteSnapshot::setTimeout(unsigned const int &secondsTimeout){
    if(isLoading())
        return;
    timeout_=secondsTimeout;
}

void WebsiteSnapshot::setJavascriptConfig(bool enabled, bool canAccessClipboard){
    javascriptEnabled_=enabled;
    javascriptCanAccessClipboard_=canAccessClipboard;
}

void WebsiteSnapshot::setJavaEnabled(bool enabled){
    javaEnabled_=enabled;
}

void WebsiteSnapshot::setLoadImagesEnabled(bool enabled){
    loadImagesEnabled_=enabled;
}

void WebsiteSnapshot::setSimpleAuthentication(const QString &username, const QString &password, const QString &finalUrl){
    if(isLoading())
        return;
    authUsername_=username;
    authPassword_=password;
    authenticationLevel_=SimpleAuthentication;
    urlAfterAuth_=QUrl(finalUrl);
}

void WebsiteSnapshot::setComplexAuthentication(const QString &username, const QString &password, const QString &finalUrl){
    if(isLoading())
        return;
    authUsername_=username;
    authPassword_=password;
    authenticationLevel_=ComplexAuthentication;
    urlAfterAuth_=QUrl(finalUrl);
    usernameSearchFields_=USUAL_USERNAME_FIELDS;
    passwordSearchFields_=USUAL_PASSWORD_FIELDS;
}

void WebsiteSnapshot::setComplexAuthenticationWithPossibleFields(const QString &username, const QString &password, const QString &finalUrl, const QStringList &usernameFields, const QStringList &passwordFields, bool tryUsualFields){
    if(isLoading())
        return;
    authUsername_=username;
    authPassword_=password;
    authenticationLevel_=ComplexAuthentication;
    urlAfterAuth_=QUrl(finalUrl);
    usernameSearchFields_ = usernameFields;
    passwordSearchFields_ =  passwordFields;
    if(tryUsualFields){
        usernameSearchFields_ << USUAL_USERNAME_FIELDS;
        passwordSearchFields_ << USUAL_PASSWORD_FIELDS;
    }
    usernameSearchFields_.removeDuplicates();
    passwordSearchFields_.removeDuplicates();
}

void WebsiteSnapshot::disableAuthentication(){
    if(isLoading())
        return;
    authenticationLevel_=NoAuthentication;
}

void WebsiteSnapshot::setComplexAuthenticationInitialWait(const unsigned short &seconds){
    complexAuthInitialWait_=seconds;
}

void WebsiteSnapshot::setWaitAfterFinish(const unsigned short &secondsWait){
    waitAfterFinish_=secondsWait;
}

bool WebsiteSnapshot::isLoading(){
    return currentlyLoading_;
}

void WebsiteSnapshot::setDebugEnabled(bool enabled){
    debug_=enabled;
}

QString WebsiteSnapshot::url(){
    switch(authenticationLevel_){
    default:
    case NoAuthentication:
        return requestedUrl_.url();
        break;
    case SimpleAuthentication:
    case ComplexAuthentication:
        return urlAfterAuth_.url();
        break;
    }
}

QString WebsiteSnapshot::lastErrorString(){
    return lastError_;
}

QObject *WebsiteSnapshot::asQObject(){
    return this;
}
