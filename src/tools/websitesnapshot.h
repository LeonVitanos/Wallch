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

#ifndef WEBSITESNAPSHOT_H
#define WEBSITESNAPSHOT_H
#define QT_NO_KEYWORDS

#include <QObject>
#include <QtPlugin>
//#include <QWebFrame>
#include <QTimer>
#include <QAuthenticator>

#include "customwebpage.h"

/*!
 * \brief A class for creating image snapshots of websites.
 *
 * The WebsiteSnapshot class gives you the ability to
 * create snapshots of web pages. It also supports
 * log-in, so as to take a snapshot of a page which needs
 * special access (cookies).
 *
 * WebsiteSnapshot creates a custom QWebPage object which
 * does not allow prompts created from javascript, for example.
 *
 * If no user authentication (log-in) is required, WebsiteSnapshot
 * loads the specified URL and then renders an image using the main
 * frame of the custom QWebPage.
 *
 * If user authentication is required, WebsiteSnapshot loads through
 * its custom QWebPage the log-in page, searches for and fills the
 * username and password fields and calls a QEvent equivalent to
 * pressing the Return key. This causes a log-in attempt to the website.
 * After the page has loaded, WebsiteSnapshot loads the final page
 * that needs to be snapshoted and takes the snapshot of it. If the final
 * page is equal to the log-in page, then WebsiteSnapshot will use the page
 * where it is redirected after the log-in, for taking the snapshot.
 */

class WebsiteSnapshot : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Constructor.
     *
     * The constructor does not take arguments because the library is designed as a Qt Plugin.
     */
    WebsiteSnapshot();
    /*!
     * \brief Destructor.
     *
     * Destroys the website snapshot.
    */
    ~WebsiteSnapshot();
    /*!
     * \brief Starts the image creation process.
     *
     * This function should not be called again, until the resultedImage()
     * signal has been emitted. Please call @a setParameters() prior to calling this function.
     *
     * Returns true if the process has started successfully, otherwise it returns false. If the process
     * has started successfully (which means that there isn't an already running process and the requested url
     * is valid), then the @a resultedImage() signal will be emitted when the process has ended. If the process
     * has not started successfully, then no signal will be emitted.
     *
     * \return True if the process has started successfully, otherwise it returns false.
     *
     * \sa resultedImage(), setParameters(), setTimeout(),
     *     setJavascriptConfig(), setJavaEnabled(), setLoadImagesEnabled(), setSimpleAuthentication(),
     *     setComplexAuthentication(), setComplexAuthenticationWithPossibleFields(),
     *     setComplexAuthenticationInitialWait(), setWaitAfterFinish()
     */
    bool start();
    /*!
     * \brief Stops the image creation process.
     *
     * It does nothing if there isn't any process running. Calling
     * this function will not call the resultedImage() signal, because it automatically means failure.
     * \sa start(), resultedImage()
     */
    void stop();
    /*!
     * \brief Sets the necessary parameters for the image creation.
     *
     * \param url The url of the page to be snapshotted. If authentication is required, then it should be the url pointing to the login page.
     * \param width The minimum width that the image should have.
     * \param height The minimum height that the image should have.
     *
     * The corresponding screen dimensions need to be passed as width and height, if a snapshot has to
     * be produced for a specific monitor.
     *
     * If authentication is required and you need to set a different URL for snapshot after the log-in,
     * then you have to specify here the url of the log-in page, not the page where the snapshot will be
     * taken. You can set a final URL in the functions where you set the log-in credentials.
     *
     * \sa setSimpleAuthentication(), setComplexAuthentication(), setComplexAuthenticationWithPossibleFields()
     */
    void setParameters(QUrl url, const short &width, const short &height);
    /*!
     * \brief Crops the final image.
     *
     * \param rectCrop The rectangle of the image that will be cropped.
     * \param enabled If true, then cropping is enabled, if false, then it is disabled.
     *
     * Crops the resulted image using @a rectCrop as (x, y, width, height).
     */
    void setCrop(bool enabled, const QRect &rectCrop);
    /*!
     * \brief Sets the total timeout.
     *
     * \param secondsTimeout The timeout to wait in seconds.
     *
     * Sets a timeout for the image creation process. If the timeout passes and the image has yet
     * to be created, then the @a resultedImage() signal is being emitted with a null QImage pointer and
     * the image creation process stops.
     *
     * By default, the timeout is set to 1 minute.
     *
     * \sa resultedImage(), start()
     */
    void setTimeout(unsigned const int &secondsTimeout);
    /*!
     * \brief Sets the javascript configuration.
     *
     * \param enabled Controls if javascript is generally enabled.
     * \param canAccessClipboard Controls whether javascript has access to user clipboard.
     *
     * Please note that even if javascript is enabled, javascript pop-ups and confirmation
     * boxes are disabled and they will not be visible at the final image.
     *
     * By default, javascript is enabled, but it cannot access the clipboard.
     */
    void setJavascriptConfig(bool enabled, bool canAccessClipboard);
    /*!
     * \brief Controls whether java is enabled or not.
     *
     * \param enabled If true, then java is enabled, otherwise it is disabled.
     *
     * By default, java is disabled.
     */
    void setJavaEnabled(bool enabled);
    /*!
     * \brief Controls whether the images inside the webpages are enabled or not.
     *
     * \param enabled If true, then images are enabled, otherwise they are disabled.
     *
     * By default, they are enabled.
     */
    void setLoadImagesEnabled(bool enabled);
    /*!
     * \brief Does a simple pop-up authentication.
     *
     * \param username The username to use for the authentication.
     * \param password The password to use for the authentication.
     * \param finalUrl The url to navigate to after the authentication is successful.
     *
     * After the page has loaded, a simple authentication will be attempted in order
     * to get to a final url and acquire the final image. If the @a finalUrl is the same
     * as the @a url specified at @a setParameters(), then the final image will be captured
     * at the url after the log-in redirection.
     *
     * Simple authentication includes authorisation pop-up. A common example
     * of such authentication is the authorization pop-up for accessing the
     * settings of most of the routers/modems.
     *
     * This function should not be used for all websites, because most of them require
     * complex authentication.
     *
     * \sa setParameters(), setComplexAuthentication()
     */
    void setSimpleAuthentication(const QString &username, const QString &password,
                                 const QString &finalUrl);
    /*!
     * \brief Does a complex username-password fields authentication.
     *
     * \param username The username to use for the authentication.
     * \param password The password to use for the authentication.
     * \param finalUrl The url to navigate to after the authentication is successful.
     *
     * After the page is loaded, a complex authentication will be attempted in order
     * to get to a final url and acquire the final image. If the @a finalUrl is the same
     * as the @a url specified at @a setParameters(), then the final image will be captured
     * at the url after the log-in redirection.
     *
     * Complex authentication includes most of the websites that have common username-password
     * authentication. WebsiteSnapshot will scan the webpage for common username-password fields
     * and will fill them with the provided username and password, correspondingly.
     *
     * Note that complex authentication will fail in the following cases:
     *   - The website does not use common username-password authentication.
     *   - The log-in page does not use common input username-password ids.
     *
     * When searching the log-in page for the appropriate username-password input fields, WebsiteSnapshot
     * actually searches for the id attribute of <input> tags.
     *
     * This is a list with the ids that WebsiteSnapshot uses (listed by the order that they are searched):
     *   - For usernames:
     *     "username", "user", "email", "Email", "User", "Username", "signin-email", "userid", "user-id",
     *     "navbar_username", "login_email", "loginUsername", "id_nickname", "id_email", "session_key-login",
     *     "ap_email", "signup_email".
     *
     *   - For passwords:
     *     "password", "pass", "passwd", "Password", "Pass", "Passwd", "signin-password", "navbar_password",
     *     "login_password", "navbar_password_hint", "passid", "pass-id", "loginPassword", "id_password",
     *     "pwd", "session_password-login", "ap_password", "signup_password".
     *
     * If you want to use specific ids, or if you want to enrich the ids that WebsiteSnapshot uses,
     * then please refer to @a setComplexAuthenticationWithPossibleFields().
     *
     * For the username field, input tags of type 'text', 'email' and 'username' are searched.
     * For the password field, input tags of type 'password' are searched.
     *
     * \sa setComplexAuthenticationWithPossibleFields()
     */
    void setComplexAuthentication(const QString &username, const QString &password,
                                  const QString &finalUrl);
    /*!
     * \brief Does a complex username-password fields authentication with custom fields.
     *
     * \param username The username to use for the authentication.
     * \param password The password to use for the authentication.
     * \param finalUrl The url to navigate to after the authentication is successful.
     * \param usernameFields The strings to search for the username field.
     * \param passwordFields The strings to search for the password field.
     * \param tryUsualFields If true, then aside from @a usernameFields and @a passwordFields,
     *                       the library's default search strings will be used. If false, then
     *                       only the @a usernameFields and @a passwordFields will be used.
     *
     * This function is the same as @a setComplexAuthentication(), but it allows you to set your
     * own username-password ids for searching, either exclusively or by enriching the current search
     * values.
     *
     *\sa setComplexAuthentication()
     */
    void setComplexAuthenticationWithPossibleFields(const QString &username,
                                                    const QString &password,
                                                    const QString &finalUrl,
                                                    const QStringList &usernameFields,
                                                    const QStringList &passwordFields,
                                                    bool tryUsualFields = false);
    /*!
     * \brief Disables a previously declared authentication type (either simple or complex).
     *
     * This function will disable the previously set authentication attempts. This is usually
     * useful when you've called start() on a page that requires authentication and then
     * you need to call start() on a page that does not require authentication using the
     * same WebsiteSnapshot object.
     *
     *\sa start()
     */
    void disableAuthentication();
    /*!
     * \brief Sets a wait timeout prior to complex authentication.
     *
     * \param seconds The seconds to wait prior attempting a complex authentication.
     *
     * This is done in order to ensure that the log-in page has loaded completely (e.g. Javascript
     * has finished creating the page) and the username and password fields are available for filling.
     *
     * By default, it is set to 1 second.
     * \sa setComplexAuthentication(), setComplexAuthenticationWithPossibleFields()
     */
    void setComplexAuthenticationInitialWait(unsigned const short &seconds);
    /*!
     * \brief Sets a wait timeout after loading the final page.
     *
     * \param secondsWait The seconds to wait.
     *
     * This is done in order to let the final web page to load completely, because in many
     * cases there are internal calls that have to be completed in order the web page to
     * take its final form.
     *
     * By default, it is set to 3 seconds.
     */
    void setWaitAfterFinish(unsigned const short &secondsWait);
    /*!
     * \brief Returns whether the process is running.
     *
     * \return True if start() has been called and the resultedImage() signal is not emitted yet.
     * In any other case it returns false.
     * \sa start(), resultedImage()
     */
    bool isLoading();
    /*!
     * \brief Controls debug messages.
     *
     * \param enabled If true, then debug messages are enabled, otherwise they are disabled.
     */
    void setDebugEnabled(bool enabled);
    /*!
     * \brief Returns the final url.
     *
     * \return The url where the website snapshot is to be taken.
     */
    QString url();
    /*!
     * \brief Returns the last error.
     *
     * The value of the string is resetted if @a start() is called anew.
     *
     * \return The last error that occurred and resulted in a @a resultedImage() signal
     * with a null QImage pointer.
     *
     *\sa resultedImage(), start()
     */
    QString lastErrorString();
    /*!
     * \brief Returns a QObject pointer of the WebsiteSnapshot instance.
     *
     * This function is useful when using the instance in a signal-slot mechanism.
     *
     * \return Α QObject pointer of the WebsiteSnapshot instance.
     */
    QObject *asQObject();

/*
private:
    enum AuthenticationLevel
    {
      NoAuthentication,
      SimpleAuthentication,
      ComplexAuthentication
    };
    AuthenticationLevel authenticationLevel_;
    bool currentlyLoading_;
    bool debug_;
    bool javascriptEnabled_;
    bool javascriptCanAccessClipboard_;
    bool javaEnabled_;
    bool loadImagesEnabled_;
    bool simpleAuthAlreadyDone_;
    bool cropImage_;
    short minWidth_;
    short minHeight_;
    short errorCode_;
    unsigned int timeout_;
    unsigned short waitAfterFinish_;
    unsigned short complexAuthInitialWait_;
    QRect cropRect_;
    QString authUsername_;
    QString authPassword_;
    QUrl requestedUrl_;
    QUrl urlAfterAuth_;
    QStringList usernameSearchFields_;
    QStringList passwordSearchFields_;
    CustomWebPage *webPage_;
    QTimer timeoutTimer_;
    QTimer afterFinishTimer_;
    QTimer priorComplexTimer_;
    QString lastError_;
    void parseUrl(QUrl &url);
    void disconnectWebPage();
    //bool searchAndSet(const QWebElement &document, const QString &searchFor, const QString &attribute, const QString &value, const QStringList &searchFields);
    void sendResultActions(QImage *image);
    void initializeWebPage();
    void decideFinalPage();
    void dbg(const QString &string);

private Q_SLOTS:
    void returnResults(bool result);
    void afterAuthNavigation(bool result);
    void authenticateSimple(QNetworkReply*, QAuthenticator* authenticator);
    void complexAuthenticationRequired(bool success);
    void webPageDestroyed();
    void timeoutReached();
    void afterFinishTimedOut();
    void proceedToComplexAuth();*/

Q_SIGNALS:
    /*!
     * \brief Emitted when the process is complete.
     *
     * \param image A pointer of the QImage data containing the final image. Null on unsuccessful attempt.
     * \param errorCode Indicates the result state. It can take the following values:
     *   - 0 No error.
     *   - 1 Some page required for the image creation could not be loaded for some reason.
     *   - 2 Simple authentication failed
     *   - 3 Username and/or password fields are not found
     *   - 4 The timeout has been reached and the image has yet to be created.
     *
     * This signal is always called when the process is complete.
     */
    void resultedImage(QImage *image, short errorCode);
};

#endif
