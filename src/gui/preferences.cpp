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

#include "preferences.h"
#include "ui_preferences.h"
#include "cachemanager.h"
#include "glob.h"

#include "math.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QProcess>
#include <QKeyEvent>

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preferences)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);
    setupShortcuts();

    /*
     * On the constructor all we need to do is to take the saved values from our .conf files (on Linux)
     * or from registry (on Windows) and put them accordingly to the checkboxes etc
    */

    //'General' Page
    ui->desktop_notification_checkbox->setChecked(settings->value("notification", false).toBool());
    ui->independent_interval_checkbox->setChecked(settings->value("independent_interval_enabled", true).toBool());
    ui->pause_battery->setChecked(settings->value("pause_on_battery", false).toBool());
    ui->language_combo->setCurrentIndex(settings->value("language", 0).toInt());
    ui->startupCheckBox->setChecked(settings->value("Startup", true).toBool());
    ui->onceCheckBox->setChecked(settings->value("Once", false).toBool());
    ui->hiddenTray_checkBox->setChecked(settings->value("start_hidden", false).toBool());
    on_startupCheckBox_clicked(ui->startupCheckBox->isChecked());
    ui->showPreview_checkBox->setChecked(gv.previewImagesOnScreen);
    ui->startup_timeout_spinbox->setValue(settings->value("startup_timeout", 3).toInt());

#ifdef Q_OS_UNIX
    short curDe = settings->value("de", 0).toInt();
    if(curDe>=ui->de_combo->count()){
        curDe=0;
    }
    ui->de_combo->setCurrentIndex(curDe);
    on_de_combo_currentIndexChanged(curDe);
    #ifdef UNITY
        ui->unity_prog_checkbox->setChecked(settings->value("unity_progressbar_enabled", false).toBool());
    #endif //ifdef UNITY
#else
    ui->integrationgroupBox->hide();
#endif
    QString curTheme = settings->value("theme", "autodetect").toString();
    if(curTheme=="ambiance")
        ui->theme_combo->setCurrentIndex(0);
    else if(curTheme=="radiance")
        ui->theme_combo->setCurrentIndex(1);
    else
        ui->theme_combo->setCurrentIndex(2);
    oldTheme = ui->theme_combo->currentIndex();

    //'Wallpapers' Page
    ui->rotate_checkBox->setChecked(settings->value("rotation", false).toBool());

    ui->first_timeout_checkbox->setChecked(settings->value("first_timeout", false).toBool());
    ui->symlinks->setChecked(settings->value("symlinks", false).toBool());
    if(settings->value("icon_style", true).toBool()==false){
        ui->paths_radioButton->setChecked(true);
    }
    listIsAlreadyIcons_=ui->icons_radioButton->isChecked();

    if(settings->value("typeOfIntervals", 0).toInt()==0)
        ui->intervalradioButton->setChecked(true);
    else
        ui->intervalradioButton_2->setChecked(true);

    //'Live Website' Page
    ui->simple_authentication->setChecked(settings->value("website_simple_auth", false).toBool());
    ui->wait_after_finish_spinbox->setValue(settings->value("website_wait_after_finish", 3).toInt());
    ui->js_enabled->setChecked(settings->value("website_js_enabled", true).toBool());
    ui->js_can_read_clipboard->setEnabled(ui->js_enabled->isChecked());
    ui->js_can_read_clipboard->setChecked(settings->value("website_js_can_read_clipboard", false).toBool());
    ui->java_enabled->setChecked(settings->value("website_java_enabled", false).toBool());
    ui->load_images->setChecked(settings->value("website_load_images", true).toBool());
    ui->extra_username_fields->setText(settings->value("website_extra_usernames", QStringList()).toStringList().join(", "));
    ui->extra_password_fields->setText(settings->value("website_extra_passwords", QStringList()).toStringList().join(", "));

    //'Advanced' Page
    int maxCacheSize = settings->value("max_cache_size", 2).toInt();
    int oldMaxCacheSliderValue = ui->max_cache_slider->value();
    if(maxCacheSize > ui->max_cache_slider->maximum() || maxCacheSize < ui->max_cache_slider->minimum()){
        ui->max_cache_slider->setValue(2);
    }
    else
    {
        ui->max_cache_slider->setValue(maxCacheSize);
    }
    if(oldMaxCacheSliderValue == ui->max_cache_slider->value()){
        on_max_cache_slider_valueChanged(ui->max_cache_slider->value());
    }

    ui->thumbnails_size_label->setText(dataToNiceString(CacheManager::getCurrentCacheSize()));
}

Preferences::~Preferences()
{
    if(oldTheme!=ui->theme_combo->currentIndex())
        on_theme_combo_currentIndexChanged(oldTheme);

    delete ui;
}

void Preferences::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Preferences::setupShortcuts(){
    (void) new QShortcut(Qt::Key_Escape, this, SLOT(close()));
    (void) new QShortcut(Qt::ALT + Qt::Key_1, this, SLOT(on_page_0_general_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_2, this, SLOT(on_page_1_wallpapers_page_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_3, this, SLOT(on_page_2_live_website_clicked()));
    (void) new QShortcut(Qt::ALT + Qt::Key_4, this, SLOT(on_page_3_advanced_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_1, this, SLOT(on_page_0_general_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_2, this, SLOT(on_page_1_wallpapers_page_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_3, this, SLOT(on_page_2_live_website_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_4, this, SLOT(on_page_3_advanced_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousPage()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextPage()));
}

void Preferences::previousPage(){
    short currentPage = ui->stackedWidget->currentIndex()-1;
    if(currentPage < 0){
        currentPage = 3;
    }
    switch(currentPage){
    default:
    case 0:
        on_page_0_general_clicked();
        break;
    case 1:
        on_page_1_wallpapers_page_clicked();
        break;
    case 2:
        on_page_2_live_website_clicked();
        break;
    case 3:
        on_page_3_advanced_clicked();
        break;
    }
}

void Preferences::nextPage(){
    short currentPage = ui->stackedWidget->currentIndex()+1;
    if(currentPage > 3){
        currentPage = 0;
    }
    switch(currentPage){
    default:
    case 0:
        on_page_0_general_clicked();
        break;
    case 1:
        on_page_1_wallpapers_page_clicked();
        break;
    case 2:
        on_page_2_live_website_clicked();
        break;
    case 3:
        on_page_3_advanced_clicked();
        break;
    }
}

void Preferences::on_closeButton_clicked()
{
    this->close();
}

void handler (const char *, void *)
{
}

void Preferences::on_saveButton_clicked()
{
    /*
     * On this function all we need to do is to update the .conf files(on Linux)
     * or registryies (on Windows) with the values of Preferences checkboxes etc
    */

    //'General' Page
    gv.independentIntervalEnabled=ui->independent_interval_checkbox->isChecked();

    gv.pauseOnBattery = ui->pause_battery->isChecked();

    QString languageFileString;
    switch (ui->language_combo->currentIndex()){
    default:
    case 0:
        languageFileString = "system_default";
        break;
    case 1:
        languageFileString = "en";
        break;
    case 2:
        languageFileString = "el";
    }
    settings->setValue("independent_interval_enabled", gv.independentIntervalEnabled);
    settings->setValue("pause_on_battery", gv.pauseOnBattery);
    settings->setValue("language", ui->language_combo->currentIndex());
    settings->setValue("language_file", languageFileString);
    settings->setValue("Startup", ui->startupCheckBox->isChecked());
    settings->setValue("startup_timeout", ui->startup_timeout_spinbox->value());
    settings->setValue("Once", ui->onceCheckBox->isChecked());
    settings->setValue("start_hidden", ui->hiddenTray_checkBox->isChecked());

    if(ui->startupCheckBox->isChecked()){
        Global::updateStartup();
    }
    else{
#ifdef Q_OS_WIN
        QSettings settings2("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        settings2.remove("Wallch");
        settings2.sync();
#else
        if(QFile::exists(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE)){
            Global::remove(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE);
        }
#endif
    }

    if(ui->showPreview_checkBox->isChecked()!=gv.previewImagesOnScreen)
    {
        gv.previewImagesOnScreen = ui->showPreview_checkBox->isChecked();
        settings->setValue("preview_images_on_screen", gv.previewImagesOnScreen);
        Q_EMIT previewChanged();
    }

#ifdef UNITY
    if(gv.unityProgressbarEnabled!=ui->unity_prog_checkbox->isChecked()){
        gv.unityProgressbarEnabled=ui->unity_prog_checkbox->isChecked();
        Q_EMIT unityProgressbarChanged(gv.unityProgressbarEnabled);
    }
    settings->setValue("unity_progressbar_enabled", gv.unityProgressbarEnabled);
    settings->setValue("de", ui->de_combo->currentIndex());

#endif
    oldTheme = ui->theme_combo->currentIndex();

    //'Wallpapers' Page
    gv.showNotification = ui->desktop_notification_checkbox->isChecked();
    gv.rotateImages=ui->rotate_checkBox->isChecked();
    gv.firstTimeout=ui->first_timeout_checkbox->isChecked();
    if(gv.symlinks!=ui->symlinks->isChecked())
    {
        gv.symlinks=ui->symlinks->isChecked();
        Q_EMIT researchFolders();
    }
    if(ui->icons_radioButton->isChecked() && !listIsAlreadyIcons_){
        Q_EMIT changePathsToIcons();
    }
    else if(ui->paths_radioButton->isChecked() && listIsAlreadyIcons_){
        Q_EMIT changeIconsToPaths();
    }

    if(ui->intervalradioButton->isChecked() && settings->value("typeOfIntervals",0)!=0){
        settings->setValue("typeOfIntervals", 0);
        Q_EMIT intervalTypeChanged();
    }
    else if(ui->intervalradioButton_2->isChecked() && settings->value("typeOfIntervals",0)!=1){
        settings->setValue("typeOfIntervals", 1);
        Q_EMIT intervalTypeChanged();
    }

    settings->setValue("notification", gv.showNotification);
    settings->setValue("rotation", gv.rotateImages);
    settings->setValue("first_timeout", gv.firstTimeout);
    settings->setValue("symlinks", gv.symlinks);
    settings->setValue("icon_style", ui->icons_radioButton->isChecked());

    //'Live Website' Page
    gv.websiteSimpleAuthEnabled=ui->simple_authentication->isChecked();
    gv.websiteWaitAfterFinishSeconds=ui->wait_after_finish_spinbox->value();
    gv.websiteJavascriptEnabled=ui->js_enabled->isChecked();
    gv.websiteJavascriptCanReadClipboard=ui->js_can_read_clipboard->isChecked();
    gv.websiteJavaEnabled=ui->java_enabled->isChecked();
    gv.websiteLoadImages=ui->load_images->isChecked();
    gv.websiteExtraUsernames=ui->extra_username_fields->text().split(',').replaceInStrings(" ", "");
    gv.websiteExtraPasswords=ui->extra_password_fields->text().split(',').replaceInStrings(" ", "");
    settings->setValue("website_simple_auth", gv.websiteSimpleAuthEnabled);
    settings->setValue("website_wait_after_finish", gv.websiteWaitAfterFinishSeconds);
    settings->setValue("website_js_enabled", gv.websiteJavascriptEnabled);
    settings->setValue("website_js_can_read_clipboard", gv.websiteJavascriptCanReadClipboard);
    settings->setValue("website_java_enabled", gv.websiteJavaEnabled);
    settings->setValue("website_load_images", gv.websiteLoadImages);
    settings->setValue("website_extra_usernames", gv.websiteExtraUsernames);
    settings->setValue("website_extra_passwords", gv.websiteExtraPasswords);

    //'Advanced' page
    settings->setValue("max_cache_size", ui->max_cache_slider->value());

    if(maxCacheChanged_){
        Q_EMIT maxCacheChanged(CacheManager().maxCacheIndexes.value(ui->max_cache_slider->value()).second);
    }

    settings->sync();
    this->close();
}

void Preferences::on_reset_clicked()
{
    if (QMessageBox::question(this, tr("Reset Preferences"), tr("Are you sure you want to reset your Wallch\npreferences?")) == QMessageBox::Yes)
    {
        //General
        ui->desktop_notification_checkbox->setChecked(false);
        ui->independent_interval_checkbox->setChecked(true);
        ui->language_combo->setCurrentIndex(0);
        ui->startupCheckBox->setChecked(true);
        ui->onceCheckBox->setChecked(false);
        ui->hiddenTray_checkBox->setChecked(false);
        ui->startup_timeout_spinbox->setValue(3);

        //Wallpapers
        ui->rotate_checkBox->setChecked(false);
        ui->first_timeout_checkbox->setChecked(false);
        ui->icons_radioButton->setChecked(true);

        //Live Website
        ui->simple_authentication->setChecked(false);
        ui->wait_after_finish_spinbox->setValue(3);
        ui->js_enabled->setChecked(true);
        ui->js_can_read_clipboard->setChecked(false);
        ui->java_enabled->setChecked(false);
        ui->load_images->setChecked(true);
        ui->extra_username_fields->clear();
        ui->extra_password_fields->clear();

        //Integration
        ui->theme_combo->setCurrentIndex(2);
        ui->de_combo->setCurrentIndex(0);
#ifdef UNITY
        if(gv.currentDE == DesktopEnvironment::UnityGnome){
            ui->unity_prog_checkbox->setChecked(false);
        }
#endif

        //applying these settings and closing the dialog...
        on_saveButton_clicked();
    }
}

void Preferences::on_page_0_general_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->page_0_general->setChecked(true);
    ui->page_0_general->raise();
}

void Preferences::on_page_1_wallpapers_page_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->page_1_wallpapers_page->setChecked(true);
    ui->page_1_wallpapers_page->raise();
}

void Preferences::on_page_2_live_website_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    ui->page_2_live_website->setChecked(true);
    ui->page_2_live_website->raise();
}

void Preferences::on_page_3_advanced_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
    ui->page_3_advanced->setChecked(true);
    ui->page_3_advanced->raise();
}

void Preferences::on_theme_combo_currentIndexChanged(int index)
{
    if(index == 0)
        settings->setValue("theme", "ambiance");
    else if (index == 1)
        settings->setValue("theme", "radiance");
    else
        settings->setValue("theme", "autodetect");

    settings->sync();

    Q_EMIT changeTheme();
}

void Preferences::on_rotate_checkBox_clicked(bool checked)
{
    if(checked){
        QMessageBox::warning(this, tr("Warning"), tr("This option will alter your images. You can also rotate your images manually via the right click menu."));
    }
}

QString Preferences::getCommandOfDesktopFile(const QString &file){
    QFile desktopFile(file);
    if(!desktopFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        return QString();
    }

    QTextStream in(&desktopFile);
    QString curLine;
    bool found=false;
    while(!in.atEnd()){
        curLine=in.readLine();
        if(curLine.startsWith("Exec=")){
            found=true;
            break;
        }
    }
    if(found){
        return curLine.right(curLine.count()-5);
    }
    else
    {
        return QString();
    }
}

QString Preferences::dataToNiceString(qint64 data){
    QStringList dataTypes = QStringList() << "bytes" << "KiB" << "MiB" << "GiB" << "TiB";
    int maxValue = dataTypes.length()-1;
    double niceData = data;
    int counter = 0;
    while(niceData >= 1024){
        counter++;
        niceData /= 1024.0;
        if(counter == maxValue){
            break;
        }
    }
    return QString::number(niceData, 'f', 2)+" "+dataTypes.at(counter);
}

#ifdef Q_OS_UNIX
void Preferences::on_de_combo_currentIndexChanged(int index)
{
    DesktopEnvironment::Value curEnv = static_cast<DesktopEnvironment::Value>(index);
    if(curEnv != DesktopEnvironment::UnityGnome)
        ui->unity_prog_checkbox->hide();
    else
        ui->unity_prog_checkbox->show();
}
#endif

void Preferences::on_startupCheckBox_clicked(bool checked)
{
    ui->onceCheckBox->setEnabled(checked);
    ui->hiddenTray_checkBox->setEnabled(checked);
    ui->label_6->setEnabled(checked);
    ui->label_7->setEnabled(checked);
    ui->startup_timeout_spinbox->setEnabled(checked);
}

void Preferences::on_help_clicked()
{
    Global::openUrl(HELP_URL);
}

void Preferences::on_max_cache_slider_valueChanged(int value)
{
    maxCacheChanged_ = true;
    ui->thumbnails_max_size_label->setText(CacheManager().maxCacheIndexes.value(value).first);
}

void Preferences::on_clear_thumbnails_button_clicked()
{
    Global::remove(gv.cachePath+"/*");
    ui->thumbnails_size_label->setText("0.00 bytes");
}
