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
#include "glob.h"

#ifdef ON_LINUX
#include "keybinder.h"
#endif

#include "math.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QProcess>
#include <QKeyEvent>
#include <QDesktopWidget>

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preferences)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    setupShortcuts();
    //move dialog to center of the screen
    this->move(QApplication::desktop()->availableGeometry().center() - this->rect().center());

    /*
     * On the constructor all we need to do is to take the saved values from our .conf files (on Linux)
     * or from registry (on Windows) and put them accordingly to the checkboxes etc
    */

    //'General' Page
    ui->desktop_notification_checkbox->setChecked(settings->value("notification", false).toBool());
    ui->clocksNotifyCheckbox->setChecked(settings->value("clocks_notification", true).toBool());
    ui->clocksNotifyCheckbox->setEnabled(ui->desktop_notification_checkbox->isChecked());
    ui->history_checkbox->setChecked(settings->value( "history", true ).toBool());
    ui->independent_interval_checkbox->setChecked(settings->value("independent_interval_enabled", true).toBool());
    ui->preview_images_checkbox->setChecked(settings->value("preview_images_on_screen", true).toBool());
    ui->language_combo->setCurrentIndex(settings->value("language", 0).toInt());
    ui->startupCheckBox->setChecked(settings->value("Startup", true).toBool());
    ui->onceCheckBox->setChecked(settings->value("Once", false).toBool());
    on_startupCheckBox_clicked(ui->startupCheckBox->isChecked());
#ifdef ON_LINUX
    ui->startup_timeout_spinbox->setValue(settings->value("startup_timeout", 3).toInt());
#else
    ui->startup_timeout_spinbox->setValue(settings->value("startup_timeout", 0).toInt());
#endif

    //'Wallpapers' Page
    ui->rotate_checkBox->setChecked(settings->value("rotation", false).toBool());
    ui->shortcut_checkbox->setChecked(settings->value("use_shortcut_next", false).toBool());
    lineEditShortcut = new LineEditShortCut(this);
    lineEditShortcut->setPlaceholderText(tr("Press here and give a combination"));
    lineEditShortcut->setText(settings->value("shortcut", "").toString());
    if(!ui->shortcut_checkbox->isChecked()){
        lineEditShortcut->setEnabled(false);
    }
    buttonsLayout_ = new QHBoxLayout;
    buttonsLayout_->setContentsMargins(QMargins(0,0,0,0));
    buttonsLayout_->addWidget(ui->shortcut_checkbox);
    buttonsLayout_->addWidget(lineEditShortcut);
    ui->widget->setLayout(buttonsLayout_);
    connect(lineEditShortcut, SIGNAL(textChanged(QString)), this, SLOT(shortcutEdited()));
    shortcutWasCheckedAtFirst_=ui->shortcut_checkbox->isChecked();
    textOfShortcutChanged_=false;
    textOfShortcutInitially_=lineEditShortcut->text();
#ifdef ON_LINUX
    keybinder_init();
#endif
    ui->first_timeout_checkbox->setChecked(settings->value("first_timeout", false).toBool());
    if(settings->value("icon_style", true).toBool()==false){
        ui->paths_radioButton->setChecked(true);
    }
    listIsAlreadyIcons_=ui->icons_radioButton->isChecked();
    customIntervalsChanged_=false;
    addNewIntervalIsShown_=false;
    short size=settings->beginReadArray("custom_intervals_in_seconds");
    for (short i=0; i<size; i++)
    {
        settings->setArrayIndex(i);
        ui->listWidget->addItem(Global::secondsToMinutesHoursDays(settings->value("item").toInt()));
    }
    settings->endArray();
    customIntervalsInSeconds_temp=gv.customIntervalsInSeconds;
    ui->random_time_checkbox->setChecked(gv.randomTimeEnabled);
    ui->random_time_from_combobox->setCurrentIndex(settings->value("from_combo", 1).toInt());
    ui->random_time_to_combobox->setCurrentIndex(settings->value("till_combo", 1).toInt());
    on_random_time_checkbox_clicked(gv.randomTimeEnabled);
    if(ui->random_time_from_combobox->currentIndex()==0){
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt());
    }
    else if(ui->random_time_from_combobox->currentIndex()==1){
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt()/60);
    }
    else
    {
        ui->random_from->setValue(settings->value("random_time_from", 10).toInt()/3600);
    }
    if(ui->random_time_to_combobox->currentIndex()==0){
        ui->random_to->setValue(settings->value("random_time_to", 10).toInt());
    }
    else if(ui->random_time_to_combobox->currentIndex()==1){
        ui->random_to->setValue(settings->value("random_time_to", 10).toInt()/60);
    }
    else
    {
        ui->random_to->setValue(settings->value("random_time_to", 10).toInt()/3600);
    }
    on_random_time_from_combobox_currentIndexChanged(ui->random_time_from_combobox->currentIndex());

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

    //'Integration' Page
#ifdef ON_LINUX
    ui->unity_prog_checkbox->setChecked(settings->value("unity_progressbar_enabled", true).toBool());
    short curDe = settings->value("de", 0).toInt();
    if(curDe>=ui->de_combo->count()){
        curDe=0;
    }
    ui->de_combo->setCurrentIndex(curDe);
    on_de_combo_currentIndexChanged(curDe);
#else
    ui->unity_prog_checkbox->hide();
    ui->de_combo->hide();
    ui->label_16->hide();
#endif
    QString curTheme = settings->value("theme", "autodetect").toString();
    if(curTheme=="ambiance"){
        ui->theme_combo->setCurrentIndex(0);
    }
    else if(curTheme=="radiance"){
        ui->theme_combo->setCurrentIndex(1);
    }
    else{
        ui->theme_combo->setCurrentIndex(2);
    }
    on_theme_combo_currentIndexChanged(ui->theme_combo->currentIndex());
    setupThemeFallbacks();
}

Preferences::~Preferences()
{
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
    (void) new QShortcut(Qt::ALT + Qt::Key_4, this, SLOT(on_page_3_integration_clicked()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousPage()));
    (void) new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextPage()));
}

void Preferences::setupThemeFallbacks(){
    ui->add_custom_interval->setIcon(QIcon::fromTheme("list-add", QIcon(":/icons/Pictures/list-add.svg")));
    ui->remove_custom_interval->setIcon(QIcon::fromTheme("list-remove", QIcon(":/icons/Pictures/list-remove.svg")));
}

void Preferences::previousPage(){
    short currentPage =  ui->stackedWidget->currentIndex()-1;
    if(currentPage<0){
        currentPage=4;
    }
    switch(currentPage){
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
            on_page_3_integration_clicked();
            break;
        default:
            on_page_0_general_clicked();
    }
}

void Preferences::nextPage(){
    short currentPage =  ui->stackedWidget->currentIndex()+1;
    if(currentPage>4){
        currentPage=0;
    }
    switch(currentPage){
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
            on_page_3_integration_clicked();
            break;
        default:
            on_page_0_general_clicked();
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
     * or registrys(on Windows) with the values of Preferences checkboxes etc
    */

    //'General' Page
    gv.saveHistory=ui->history_checkbox->isChecked();
    gv.independentIntervalEnabled=ui->independent_interval_checkbox->isChecked();
    if(gv.previewImagesOnScreen!=ui->preview_images_checkbox->isChecked())
    {
        gv.previewImagesOnScreen=ui->preview_images_checkbox->isChecked();
        Q_EMIT tvPreviewChanged(gv.previewImagesOnScreen);
    }
    QString lang_file_string;
    switch (ui->language_combo->currentIndex()){
        default:
        case 0: lang_file_string="en"; break;
        case 1: lang_file_string="el"; break;
    }
    settings->setValue("history", gv.saveHistory);
    settings->setValue("independent_interval_enabled", gv.independentIntervalEnabled);
    settings->setValue("preview_images_on_screen", gv.previewImagesOnScreen);
    settings->setValue("language", ui->language_combo->currentIndex());
    settings->setValue("language_file", lang_file_string);
    settings->setValue("Startup", ui->startupCheckBox->isChecked());
    settings->setValue("startup_timeout", ui->startup_timeout_spinbox->value());
    settings->setValue("Once", ui->onceCheckBox->isChecked());

    if(ui->startupCheckBox->isChecked()){
        Global::updateStartup();
    }
    else{
#ifdef ON_WIN32
        QSettings settings2("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        settings2.remove("Wallch");
        settings2.sync();
#else
        if(QFile::exists(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE)){
            Global::remove(gv.homePath+AUTOSTART_DIR+"/"+BOOT_DESKTOP_FILE);
        }
#endif
    }

    //'Wallpapers' Page
    gv.showNotification = ui->desktop_notification_checkbox->isChecked();
    gv.rotateImages=ui->rotate_checkBox->isChecked();
    if(shortcutWasCheckedAtFirst_){
        if(!ui->shortcut_checkbox->isChecked()){
            //unbind the current key
            Q_EMIT unbindKeySignal(gv.nextShortcut);
        }
        else
        {
            //it was and it is checked, check if text has changed...
            //if it has, then update with the new button
            if(textOfShortcutChanged_){
                //unbind the old key and bind the new one...
                Q_EMIT unbindKeySignal(gv.nextShortcut);
                if(! (lineEditShortcut->text().endsWith(">") || lineEditShortcut->text().isEmpty()) ){
                    //if the entry is ok, then send command to the mainwindow to bind the key
                    gv.nextShortcut=lineEditShortcut->text();
                    Q_EMIT bindKeySignal(gv.nextShortcut);
                }
                else
                {
                    gv.useShortcutNext=false;
                    gv.nextShortcut="";
                }
            }
        }
    }
    else
    {
        if(ui->shortcut_checkbox->isChecked()){
            if(! (lineEditShortcut->text().endsWith(">") || lineEditShortcut->text().isEmpty()) ){
                //if the entry is ok, then send command to the mainwindow to bind the key and update the
                gv.nextShortcut=lineEditShortcut->text();
                Q_EMIT bindKeySignal(gv.nextShortcut);
            }
            else
            {
                gv.useShortcutNext=false;
                gv.nextShortcut="";
            }
        }
    }
    gv.useShortcutNext=ui->shortcut_checkbox->isChecked();
    gv.useShortcutNext=ui->shortcut_checkbox->isChecked();
    gv.firstTimeout=ui->first_timeout_checkbox->isChecked();
    if(ui->icons_radioButton->isChecked() && !listIsAlreadyIcons_){
        Q_EMIT changePathsToIcons();
    }
    else if(ui->paths_radioButton->isChecked() && listIsAlreadyIcons_){
        Q_EMIT changeIconsToPaths();
    }
    if(customIntervalsChanged_)
    {
        gv.customIntervalsInSeconds=customIntervalsInSeconds_temp;
        if(gv.customIntervalsInSeconds.count()>0)
        {
            settings->beginWriteArray("custom_intervals_in_seconds");
            short listCount = ui->listWidget->count();
            for (short i = 0; i < listCount; ++i) {
                settings->setArrayIndex(i);
                settings->setValue("item", gv.customIntervalsInSeconds.at(i));
            }
            settings->endArray();
        }
        Q_EMIT refreshCustomIntervals();
    }
    if(gv.randomTimeEnabled!=ui->random_time_checkbox->isChecked())
    {
        gv.randomTimeEnabled=ui->random_time_checkbox->isChecked();
        Q_EMIT changeRandomTime(gv.randomTimeEnabled);
    }
    gv.randomTimeFrom=ui->random_from->value() * pow(60, ui->random_time_from_combobox->currentIndex());
    if(gv.randomTimeFrom==0){
        gv.randomTimeFrom=1;
    }
    gv.randomTimeTo=ui->random_to->value() * pow(60, ui->random_time_to_combobox->currentIndex());
    settings->setValue("notification", gv.showNotification);
    settings->setValue("clocks_notification", ui->clocksNotifyCheckbox->isChecked());
    settings->setValue("rotation", gv.rotateImages);
    settings->setValue("use_shortcut_next", gv.useShortcutNext);
    settings->setValue("shortcut", gv.nextShortcut);
    settings->setValue("first_timeout", gv.firstTimeout);
    settings->setValue("icon_style", ui->icons_radioButton->isChecked());
    settings->setValue("random_time_from", gv.randomTimeFrom);
    settings->setValue("random_time_to", gv.randomTimeTo);
    settings->setValue("random_time_enabled", gv.randomTimeEnabled);
    settings->setValue("from_combo", ui->random_time_from_combobox->currentIndex());
    settings->setValue("till_combo", ui->random_time_to_combobox->currentIndex());

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

    //'Integration' Page
#ifdef ON_LINUX
    if(gv.unityProgressbarEnabled!=ui->unity_prog_checkbox->isChecked()){
        gv.unityProgressbarEnabled=ui->unity_prog_checkbox->isChecked();
        Q_EMIT unityProgressbarChanged(gv.unityProgressbarEnabled);
    }
    settings->setValue("unity_progressbar_enabled", gv.unityProgressbarEnabled);
    settings->setValue("de", ui->de_combo->currentIndex());

#endif
    switch(ui->theme_combo->currentIndex()){
        case 0:
            settings->setValue("theme", "ambiance");
            break;
        case 1:
            settings->setValue("theme", "radiance");
            break;
        case 2:
            settings->setValue("theme", "autodetect");
            break;
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
        ui->clocksNotifyCheckbox->setChecked(false);
        ui->history_checkbox->setChecked(true);
        ui->independent_interval_checkbox->setChecked(true);
        ui->preview_images_checkbox->setChecked(true);
        ui->language_combo->setCurrentIndex(3);
        ui->startupCheckBox->setChecked(true);
        ui->onceCheckBox->setChecked(false);
#ifdef ON_LINUX
        ui->startup_timeout_spinbox->setValue(3);
#else
        ui->startup_timeout_spinbox->setValue(0);
#endif

        //Wallpapers
        ui->rotate_checkBox->setChecked(false);
        ui->shortcut_checkbox->setChecked(false);
        ui->first_timeout_checkbox->setChecked(false);
        ui->icons_radioButton->setChecked(true);
        ui->listWidget->clear();
        Q_EMIT refreshCustomIntervals();
        ui->listWidget->addItems(gv.defaultIntervals);
        ui->random_time_checkbox->setChecked(false);
        ui->random_time_from_combobox->setCurrentIndex(1);
        ui->random_from->setValue(0);
        ui->random_time_to_combobox->setCurrentIndex(1);
        ui->random_to->setValue(1);

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
#ifdef ON_LINUX
        if(gv.currentDE==UnityGnome){
            ui->unity_prog_checkbox->setChecked(true);
        }
#endif

        //applying these settings and closing the dialog...
        on_saveButton_clicked();
    }
}

//Random time
void Preferences::on_random_time_from_combobox_currentIndexChanged(int index)
{
    ui->random_from->setMinimum(0);

    if(ui->random_time_to_combobox->currentIndex()<index){
        ui->random_time_to_combobox->setCurrentIndex(index);
    }
    on_random_from_valueChanged(ui->random_from->value());
}

void Preferences::on_random_time_to_combobox_currentIndexChanged(int index)
{
    if(index==0){
        ui->random_time_from_combobox->setCurrentIndex(0);
    }
    else if (index==1 && ui->random_time_from_combobox->currentIndex()==2){
        ui->random_time_from_combobox->setCurrentIndex(1);
    }
    on_random_to_valueChanged(ui->random_to->value());
}

void Preferences::on_random_from_valueChanged(int value)
{
    short combo_from=ui->random_time_from_combobox->currentIndex();
    short combo_till=ui->random_time_to_combobox->currentIndex();
    short value2=ui->random_to->value();

    if(combo_from==0 && combo_till==0 && value>=value2-2){
        ui->random_to->setValue(value+3);
    }
    else if(((combo_from==1 && combo_till==1) || (combo_from==2 && combo_till==2)) && value>=value2){
        ui->random_to->setValue(value+1);
    }
}

void Preferences::on_random_to_valueChanged(int value2)
{
    short combo_from=ui->random_time_from_combobox->currentIndex();
    short combo_till=ui->random_time_to_combobox->currentIndex();
    short value=ui->random_from->value();

    if(combo_from==0 && combo_till==0 && value>=value2-2){
        ui->random_from->setValue(value2-3);
    }
    else if(((combo_from==1 && combo_till==1) || (combo_from==2 && combo_till==2)) && value>=value2){
        ui->random_from->setValue(value2-1);
    }
}
//End of random time

void LineEditShortCut::keyPressEvent(QKeyEvent *event)
{

    int modifiers = event->modifiers();
    if( !(gotCtrlKey_ || gotAltKey_) )
    this->clear();

    if(!gotCtrlKey_ && (modifiers & Qt::ControlModifier)){
      gotCtrlKey_=true;
      this->insert("<Ctrl>");
    }
    else if(!gotAltKey_ && (modifiers & Qt::AltModifier)){
      gotAltKey_=true;
      this->insert("<Alt>");
    }
    else
    {
      //normal key!
      if(! (gotAltKey_ || gotCtrlKey_) ){
          return;
      }
      gotSomeKey_=true;
      switch(event->key()){
          case Qt::Key_A:
              this->insertAtEnd("A");
              break;
          case Qt::Key_B:
              this->insertAtEnd("B");
              break;
          case Qt::Key_C:
              this->insertAtEnd("C");
              break;
          case Qt::Key_D:
              this->insertAtEnd("D");
              break;
          case Qt::Key_E:
              this->insertAtEnd("E");
              break;
          case Qt::Key_F:
              this->insertAtEnd("F");
              break;
          case Qt::Key_G:
              this->insertAtEnd("G");
              break;
          case Qt::Key_H:
              this->insertAtEnd("H");
              break;
          case Qt::Key_I:
              this->insertAtEnd("I");
              break;
          case Qt::Key_J:
              this->insertAtEnd("J");
              break;
          case Qt::Key_K:
              this->insertAtEnd("K");
              break;
          case Qt::Key_L:
              this->insertAtEnd("L");
              break;
          case Qt::Key_M:
              this->insertAtEnd("M");
              break;
          case Qt::Key_N:
              this->insertAtEnd("N");
              break;
          case Qt::Key_O:
              this->insertAtEnd("O");
              break;
          case Qt::Key_P:
              this->insertAtEnd("P");
              break;
          case Qt::Key_Q:
              this->insertAtEnd("Q");
              break;
          case Qt::Key_R:
              this->insertAtEnd("R");
              break;
          case Qt::Key_S:
              this->insertAtEnd("S");
              break;
          case Qt::Key_T:
              this->insertAtEnd("T");
              break;
          case Qt::Key_U:
              this->insertAtEnd("U");
              break;
          case Qt::Key_V:
              this->insertAtEnd("V");
              break;
          case Qt::Key_W:
              this->insertAtEnd("W");
              break;
          case Qt::Key_X:
              this->insertAtEnd("X");
              break;
          case Qt::Key_Y:
              this->insertAtEnd("Y");
              break;
          case Qt::Key_Z:
              this->insertAtEnd("Z");
              break;
          case Qt::Key_1:
              this->insertAtEnd("1");
              break;
          case Qt::Key_2:
              this->insertAtEnd("2");
              break;
          case Qt::Key_3:
              this->insertAtEnd("3");
              break;
          case Qt::Key_4:
              this->insertAtEnd("4");
              break;
          case Qt::Key_5:
              this->insertAtEnd("5");
              break;
          case Qt::Key_6:
              this->insertAtEnd("6");
              break;
          case Qt::Key_7:
              this->insertAtEnd("7");
              break;
          case Qt::Key_8:
              this->insertAtEnd("8");
              break;
          case Qt::Key_9:
              this->insertAtEnd("9");
              break;
          case Qt::Key_0:
              this->insertAtEnd("0");
              break;
          case Qt::Key_Space:
              this->insertAtEnd("space");
              break;
          case Qt::Key_F1:
              this->insertAtEnd("F1");
              break;
          case Qt::Key_F2:
              this->insertAtEnd("F2");
              break;
          case Qt::Key_F3:
              this->insertAtEnd("F3");
              break;
          case Qt::Key_F4:
              this->insertAtEnd("F4");
              break;
          case Qt::Key_F5:
              this->insertAtEnd("F5");
              break;
          case Qt::Key_F6:
              this->insertAtEnd("F6");
              break;
          case Qt::Key_F7:
              this->insertAtEnd("F7");
              break;
          case Qt::Key_F8:
              this->insertAtEnd("F8");
              break;
          case Qt::Key_F9:
              this->insertAtEnd("F9");
              break;
          case Qt::Key_F10:
              this->insertAtEnd("F10");
              break;
          case Qt::Key_F11:
              this->insertAtEnd("F11");
              break;
          case Qt::Key_F12:
              this->insertAtEnd("F12");
              break;
          case Qt::Key_Agrave:
              this->insertAtEnd("grave");
              break;
          case Qt::Key_AsciiTilde:
              this->insertAtEnd("asciitilde");
              break;
          case Qt::Key_Exclam:
              this->insertAtEnd("exclam");
              break;
          case Qt::Key_At:
              this->insertAtEnd("at");
              break;
          case Qt::Key_NumberSign:
              this->insertAtEnd("numbersign");
              break;
          case Qt::Key_Dollar:
              this->insertAtEnd("dollar");
              break;
          case Qt::Key_Percent:
              this->insertAtEnd("percent");
              break;
          case Qt::Key_AsciiCircum:
              this->insertAtEnd("asciicircum");
              break;
          case Qt::Key_Ampersand:
              this->insertAtEnd("ampersand");
              break;
          case Qt::Key_Asterisk:
              this->insertAtEnd("asterisk");
              break;
          case Qt::Key_ParenLeft:
              this->insertAtEnd("parenleft");
              break;
          case Qt::Key_ParenRight:
              this->insertAtEnd("parenright");
              break;
          case Qt::Key_Minus:
              this->insertAtEnd("minus");
              break;
          case Qt::Key_Underscore:
              this->insertAtEnd("underscore");
              break;
          case Qt::Key_Plus:
              this->insertAtEnd("plus");
              break;
          case Qt::Key_Equal:
              this->insertAtEnd("equal");
              break;
          case Qt::Key_Backspace:
              this->insertAtEnd("BackSpace");
              break;
          case Qt::Key_Tab:
              this->insertAtEnd("Tab");
              break;
          case Qt::Key_BracketLeft:
              this->insertAtEnd("bracketleft");
              break;
          case Qt::Key_BracketRight:
              this->insertAtEnd("bracketright");
              break;
          case Qt::Key_BraceLeft:
              this->insertAtEnd("braceleft");
              break;
          case Qt::Key_BraceRight:
              this->insertAtEnd("braceright");
              break;
          case Qt::Key_Backslash:
              this->insertAtEnd("backslash");
              break;
          case Qt::Key_Bar:
              this->insertAtEnd("bar");
              break;
          case Qt::Key_Colon:
              this->insertAtEnd("colon");
              break;
          case Qt::Key_Semicolon:
              this->insertAtEnd("semicolon");
              break;
          case Qt::Key_Apostrophe:
              this->insertAtEnd("apostrophe");
              break;
          case Qt::Key_QuoteDbl:
              this->insertAtEnd("quotedbl");
              break;
          case Qt::Key_Return:
              this->insertAtEnd("Return");
              break;
          case Qt::Key_Comma:
              this->insertAtEnd("comma");
              break;
          case Qt::Key_Less:
              this->insertAtEnd("less");
              break;
          case Qt::Key_Period:
              this->insertAtEnd("period");
              break;
          case Qt::Key_Greater:
              this->insertAtEnd("greater");
              break;
          case Qt::Key_Slash:
              this->insertAtEnd("slash");
              break;
          case Qt::Key_Question:
              this->insertAtEnd("question");
              break;
          case Qt::Key_PageUp:
              this->insertAtEnd("Page_Up");
              break;
          case Qt::Key_PageDown:
              this->insertAtEnd("Page_Down");
              break;
          case Qt::Key_Left:
              this->insertAtEnd("leftarrow");
              break;
          case Qt::Key_Right:
              this->insertAtEnd("rightarrow");
              break;
          case Qt::Key_Up:
              this->insertAtEnd("uparrow");
              break;
          case Qt::Key_Down:
              this->insertAtEnd("downarrow");
              break;
          case Qt::Key_Escape:
              this->insertAtEnd("Escape");
              break;
          case Qt::Key_Insert:
              this->insertAtEnd("Insert");
              break;
          case Qt::Key_Delete:
              this->insertAtEnd("Delete");
              break;
          case Qt::Key_Print:
              this->insertAtEnd("Print");
              break;
          case Qt::Key_Pause:
              this->insertAtEnd("Pause");
              break;
          case Qt::Key_ScrollLock:
              this->insertAtEnd("Scroll_Lock");
              break;
          case Qt::Key_NumLock:
              this->insertAtEnd("Num_Lock");
              break;
          case Qt::Key_SysReq:
              this->insertAtEnd("Sys_Req");
              break;
          case Qt::Key_Home:
              this->insertAtEnd("Home");
              break;
          case Qt::Key_End:
              this->insertAtEnd("End");
              break;
          default:
              gotSomeKey_=false;
              break;
      }
    }
}

void LineEditShortCut::keyReleaseEvent(QKeyEvent *){
    this->clearFocus();

#ifdef ON_LINUX

    gotCtrlKey_=gotAltKey_=false;
    if(!gotSomeKey_){
        this->clear();
    }
    else
    {
        gotSomeKey_=false;
        if(!keybinder_bind(this->text().toLocal8Bit().data(), handler, NULL)){
            //key could not be binded!
            QMessageBox::warning(this, tr("Warning"), tr("The key sequence you chose is not valid or it is taken by another application."));
            this->clear();
        }
        else
        {
            /*
             * The key can be binded but the user has yet to click the Save button, so
             * unbind it for now till then.
             */
            keybinder_unbind(this->text().toLocal8Bit().data(), handler);
        }
    }
#endif //#ifdef ON_LINUX
}

void Preferences::on_shortcut_checkbox_clicked(bool checked)
{
    lineEditShortcut->setEnabled(checked);
    if(checked){
        lineEditShortcut->setFocus();
    }
}

void Preferences::shortcutEdited(){
    if(shortcutWasCheckedAtFirst_ && !textOfShortcutChanged_){
        unbindKeySignal(textOfShortcutInitially_);
    }
    textOfShortcutChanged_=true;
}

void Preferences::on_page_0_general_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->page_0_general->setChecked(true);
    ui->page_0_general->raise();
    ui->sep1->raise();
}

void Preferences::on_page_1_wallpapers_page_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->page_1_wallpapers_page->setChecked(true);
    ui->page_1_wallpapers_page->raise();
    ui->sep1->raise();
    ui->sep2->raise();
}

void Preferences::on_page_2_live_website_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
    ui->page_2_live_website->setChecked(true);
    ui->page_2_live_website->raise();
    ui->sep2->raise();
    ui->sep3->raise();
}

void Preferences::on_page_3_integration_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
    ui->page_3_integration->setChecked(true);
    ui->page_3_integration->raise();
    ui->sep3->raise();
}

void Preferences::setThemeToAmbiance(){
    ui->page_0_general->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_1_wallpapers_page->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_2_live_website->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->page_3_integration->setStyleSheet(AMBIANCE_THEME_STYLESHEET);
    ui->sep1->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep2->setPixmap(AMBIANCE_SEPARATOR);
    ui->sep3->setPixmap(AMBIANCE_SEPARATOR);
    ui->widget_2->setStyleSheet("QWidget { background-image: url(:/icons/Pictures/ambiance_not_checked.png); }");
#ifdef ON_LINUX
    app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_AMBIANCE_NORMAL, INDICATOR_DESCRIPTION);
#endif
    Q_EMIT changeThemeTo("ambiance");
}

void Preferences::setThemeToRadiance(){
    ui->page_0_general->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_1_wallpapers_page->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_2_live_website->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->page_3_integration->setStyleSheet(RADIANCE_THEME_STYLESHEET);
    ui->sep1->setPixmap(RADIANCE_SEPARATOR);
    ui->sep2->setPixmap(RADIANCE_SEPARATOR);
    ui->sep3->setPixmap(RADIANCE_SEPARATOR);
    ui->widget_2->setStyleSheet("QWidget { background-image: url(:/icons/Pictures/radiance_not_checked.png); }");
#ifdef ON_LINUX
    app_indicator_set_icon_full(gv.applicationIndicator, INDICATOR_RADIANCE_NORMAL, INDICATOR_DESCRIPTION);
#endif
    Q_EMIT changeThemeTo("radiance");
}

void Preferences::on_theme_combo_currentIndexChanged(int index)
{
    switch(index){
        case 0:
            setThemeToAmbiance();
            break;
        case 1:
            setThemeToRadiance();
            break;
        default:
        {
            //autodetect theme

            QString curTheme;
#ifdef ON_LINUX
            if(gv.currentDE==Gnome || gv.currentDE==UnityGnome){
                curTheme=Global::gsettingsGet("org.gnome.desktop.interface", "gtk-theme");
            }
            else
            {
                if(gv.currentTheme=="ambiance"){
                    curTheme="Ambiance";
                }
                else
                {
                    curTheme="Radiance";
                }
            }
#endif
            if(curTheme=="Ambiance"){
                setThemeToAmbiance();
            }
            else
            {
                setThemeToRadiance();
            }
            break;
        }
    }
}

void Preferences::on_random_time_checkbox_clicked(bool checked)
{
    ui->random_time_from_label->setEnabled(checked);
    ui->random_time_from_combobox->setEnabled(checked);
    ui->random_time_to_label->setEnabled(checked);
    ui->random_time_to_combobox->setEnabled(checked);
    ui->random_from->setEnabled(checked);
    ui->random_to->setEnabled(checked);

    ui->listWidget->setEnabled(!checked);
    ui->custom_intervals_label->setEnabled(!checked);
    ui->add_custom_interval->setEnabled(!checked);
    ui->remove_custom_interval->setEnabled(!checked);
}

void Preferences::on_add_custom_interval_clicked()
{
    QString time_type;
    int time_in_seconds=0;
    switch (ui->time_comboBox->currentIndex())
    {
        case 0:
            time_in_seconds=ui->time_spinBox->value();
            if(ui->time_spinBox->value()==1){
                time_type=tr("second");
            }
            else{
                time_type=tr("seconds");
            }
            break;
        case 1:
            time_in_seconds=60*ui->time_spinBox->value();
            if(ui->time_spinBox->value()==1){
                time_type=tr("minute");
            }
            else{
                time_type=tr("minutes");
            }
            break;
        case 2:
            time_in_seconds=3600*ui->time_spinBox->value();
            if(ui->time_spinBox->value()==1){
                time_type=tr("hour");
            }
            else{
                time_type=tr("hours");
            }
            break;
        case 3:
            time_in_seconds=86400*ui->time_spinBox->value();
            if(ui->time_spinBox->value()==1){
                time_type=tr("day");
            }
            else{
                time_type=tr("days");
            }
            break;
    }

    customIntervalsInSeconds_temp << time_in_seconds;
    for (short i=customIntervalsInSeconds_temp.count()-1; i>0; i--)
    {
        if(customIntervalsInSeconds_temp[i]<customIntervalsInSeconds_temp[i-1]){
            customIntervalsInSeconds_temp.swap(i,i-1);
        }
        else if(customIntervalsInSeconds_temp[i]==customIntervalsInSeconds_temp[i-1])
        {
            customIntervalsInSeconds_temp.removeAt(i);
            QMessageBox::warning(this, tr("Error"), tr("Interval")+" \""+ui->time_spinBox->text()+" "+time_type+"\" "+tr("already exists."));
            break;
        }
        else
        {
            customIntervalsChanged_=true;
            ui->listWidget->insertItem(i, ui->time_spinBox->text()+" "+time_type);
            break;
        }
        if(i==1)
        {
            customIntervalsChanged_=true;
            ui->listWidget->insertItem(0, ui->time_spinBox->text()+" "+time_type);
        }
    }
}

void Preferences::on_remove_custom_interval_clicked()
{
    if (ui->listWidget->count()==2){
        QMessageBox::warning(this, tr("Error"), tr("You need to have at least two intervals"));
    }
    else
    {
        customIntervalsChanged_=true;
        customIntervalsInSeconds_temp.removeAt(ui->listWidget->currentRow());
        delete ui->listWidget->currentItem();
    }
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

#ifdef ON_LINUX
void Preferences::on_de_combo_currentIndexChanged(int index)
{
    DesktopEnvironment curEnv = static_cast<DesktopEnvironment>(index);
    if(curEnv!=UnityGnome){
        ui->unity_prog_checkbox->hide();
    }
    else
    {
        ui->unity_prog_checkbox->show();
    }
}
#endif

void Preferences::on_startupCheckBox_clicked(bool checked)
{
    ui->onceCheckBox->setEnabled(checked);
    ui->label_6->setEnabled(checked);
    ui->label_7->setEnabled(checked);
    ui->startup_timeout_spinbox->setEnabled(checked);
    ui->label->setEnabled(checked);
}

void Preferences::on_help_clicked()
{
    Global::openUrl(HELP_URL);
}
