
#define QT_NO_KEYWORDS

#include "preferences.h"
#include "ui_preferences.h"
#include "glob.h"

#include <QSettings>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QHoverEvent>

#include <iostream>
#include <fstream>

using namespace std;

int minimize_already_checked=0;
int album=0;

preferences::preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::preferences)
{
    ui->setupUi(this);
    QSettings settings( "Wallch", "Preferences" );
    ui->browsesoundButton->setText( settings.value( "browsesoundButton" ).toString() );
    ui->checkBox_3->setChecked(settings.value("tray", false).toBool());
    ui->unity_prog_checkbox->setChecked(settings.value("unity_prog", false).toBool());
    ui->hideapp_checkbox->setChecked(settings.value("hideapp", false).toBool());
    ui->playsoundcheckBox->setChecked(settings.value("sound", false).toBool());
    ui->rotate_checkBox->setChecked(settings.value("rotation", false).toBool());
    ui->customsoundcheckBox->setChecked(settings.value("custom", false).toBool());
    ui->combostyle_3->setCurrentIndex(settings.value( "history" ).toInt());
    ui->checkBox_4->setChecked(settings.value("notification", false).toBool());
    ui->combostyle_2->setCurrentIndex(settings.value("language", 4).toInt());


    if(!ui->playsoundcheckBox->isChecked()){
        ui->customsoundcheckBox->setEnabled(false);
        ui->customsoundcheckBox->setChecked(false);
        ui->browsesoundButton->setEnabled(false);
    }
    if(!ui->customsoundcheckBox->isChecked())
        ui->browsesoundButton->setEnabled(false);

    //checking to see if there is any option available for on pc-start wallpaper change
    if(QFile(QDir::homePath()+"/.config/autostart/WallchLiveEarthOnBoot.desktop").exists())
    {
        ui->checkBox_5->setChecked(true);
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->radioButton_3->setEnabled(true); ui->radioButton_3->setChecked(true);
        ui->radioButton_4->setEnabled(true);
    }
    else if(QFile(QDir::homePath()+"/.config/autostart/WallchPictureOfDayOnBoot.desktop").exists()){
        ui->checkBox_5->setChecked(true);
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->radioButton_4->setEnabled(true); ui->radioButton_4->setChecked(true);
        ui->radioButton_3->setEnabled(true);
    }
    else if(QFile(QDir::homePath()+"/.config/autostart/WallchOnBoot.desktop").exists())
    {
        ui->checkBox_5->setChecked(true);
        ui->radioButton_3->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->radioButton->setEnabled(true); ui->radioButton->setChecked(true);
        ui->radioButton_4->setEnabled(true);
    }
    else if(QFile(QDir::homePath()+"/.config/autostart/WallchOnBootConstantApp.desktop").exists())
    {
        ui->checkBox_5->setChecked(true);
        ui->radioButton_3->setEnabled(true);
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true); ui->radioButton_2->setChecked(true);
        ui->radioButton_4->setEnabled(true);
    }
    else
    {
        ui->radioButton->setEnabled(false);
        ui->radioButton_2->setEnabled(false);
        ui->radioButton_3->setEnabled(false);
        ui->radioButton_4->setEnabled(false);
        ui->checkBox_5->setChecked(false);
    }

    if(ui->checkBox_3->isChecked()) minimize_already_checked=1; else minimize_already_checked=0;

    if(ui->browsesoundButton->text().isEmpty())
        ui->browsesoundButton->setText(tr("(None)"));

      FILE* pipe;
      if(gnome_version==2)
          pipe=popen("gconftool-2 --get /desktop/gnome/background/picture_options", "r");
      else
          pipe=popen("gsettings get org.gnome.desktop.background picture-options", "r");

      char buffer[128];
      string result = "";

      while(!feof(pipe)) {
          if(fgets(buffer, 128, pipe) != NULL)
              result += buffer;
      }
      pclose(pipe);
      QString res = QString::fromStdString(result);
      res.replace(QString("\n"),QString(""));
      res.replace(QString("'"),QString(""));
      if (res=="wallpaper") ui->combostyle->setCurrentIndex(0);
      if (res=="zoom") ui->combostyle->setCurrentIndex(1);
      if (res=="centered") ui->combostyle->setCurrentIndex(2);
      if (res=="scaled") ui->combostyle->setCurrentIndex(3);
      if (res=="stretched") ui->combostyle->setCurrentIndex(4);
      if (res=="spanned") ui->combostyle->setCurrentIndex(5);
      ui->pushButton->setIcon(QIcon::fromTheme("audio-volume-muted"));
      ui->browsesoundButton->setIcon(QIcon::fromTheme("audio-x-generic"));
      ui->pushButton_3->setIcon(QIcon::fromTheme("start-here"));
      ui->pushButton_4->setIcon(QIcon::fromTheme("applications-graphics"));
      ui->pushButton_5->setIcon(QIcon::fromTheme("applications-multimedia"));
}


void preferences::closeEvent( QCloseEvent * )
{
    //writing current values
    QSettings
       settings( "Wallch", "Preferences" );
       settings.setValue( "browsesoundButton", ui->browsesoundButton->text() );
       settings.sync();
}

preferences::~preferences()
{
    delete ui;
}

void preferences::changeEvent(QEvent *e)
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

void preferences::on_checkBox_5_clicked()
{
    bool status=ui->checkBox_5->isChecked();
    ui->radioButton->setEnabled(status);
    ui->radioButton_2->setEnabled(status);
    ui->radioButton_3->setEnabled(status);
    ui->radioButton_4->setEnabled(status);
}

void preferences::on_closeButton_clicked()
{
    close();
}

void preferences::on_applyButton_clicked()
{
    //checking if something is wrong
    //removing all Wallch .desktop files, this would be done either checkbox_5 is checked or not
    if(system("rm -f ~/.config/autostart/Wallch*"))
        cerr << "Error removing other Wallch desktop files inside ~/.config/autostart/.\n";
    if(ui->checkBox_5->isChecked()){
        if(!QFile(QDir::homePath()+"/.config/autostart").exists()){
            if(system("mkdir -p ~/.config/autostart/"))
                cerr << "Error creating ~/.config/autostart folder. Please check file existence and permissions.\n";
        }
        if (ui->radioButton->isChecked()){
            if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --once\nTerminal=false\nIcon=wallch\nComment=Change Desktop Background Once\nCategories=Utility;Application;\" > ~/.config/autostart/WallchOnBoot.desktop"))
                cerr << "Error creating ~/.config/autostart/WallchOnBoot.desktop. Please check file existence and permissions.\n";
        }
        else if(ui->radioButton_2->isChecked()){
            if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --constant\nTerminal=false\nIcon=wallch\nComment=Start Changing Desktop Background After a Certain Time\nCategories=Utility;Application;\" > ~/.config/autostart/WallchOnBootConstantApp.desktop"))
                cerr << "Error creating ~/.config/autostart/WallchOnBootConstantApp.desktop. Please check file existence and permissions.\n";
        }
        else if(ui->radioButton_3->isChecked()){
            if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --earth\nTerminal=false\nIcon=wallch\nComment=Enable Live Earth Wallpaper\nCategories=Utility;Application;\" > ~/.config/autostart/WallchLiveEarthOnBoot.desktop"))
                cerr << "Error creating ~/.config/autostart/WallchLiveEarthOnBoot.desktop. Please check file existence and permissions.\n";
        }
        else if(ui->radioButton_4->isChecked()){
            if(system("echo \"\n[Desktop Entry]\nType=Application\nName=Wallch\nExec=/usr/bin/wallch --picofday\nTerminal=false\nIcon=wallch\nComment=Enable Picture Of The Day\nCategories=Utility;Application;\" > ~/.config/autostart/WallchPictureOfDayOnBoot.desktop"))
                cerr << "Error creating ~/.config/autostart/WallchPictureOfDayOnBoot.desktop. Please check file existence and permissions.\n";
        }
    }

    //We need this because hideapp will be 1 if user chose so from the preferences and then tests it
    hideapp=ui->hideapp_checkbox->isChecked();

    QSettings settings( "Wallch", "Preferences" );
    settings.setValue("tray", ui->checkBox_3->isChecked());
    settings.setValue("rotation", ui->rotate_checkBox->isChecked());
    settings.setValue("unity_prog", ui->unity_prog_checkbox->isChecked());
    settings.setValue("hideapp", ui->hideapp_checkbox->isChecked());
    settings.setValue("notification", ui->checkBox_4->isChecked());
    settings.setValue("history", ui->combostyle_3->currentIndex());
    settings.setValue("language", ui->combostyle_2->currentIndex());
    settings.sync();

    if(ui->checkBox_3->isChecked()){ if (!minimize_already_checked) minimize_to_tray=1; }
    else minimize_to_tray=0;

    if(ui->checkBox_4->isChecked()) _show_notification_=1;
    else _show_notification_=0;

    if(ui->combostyle_3->currentIndex()==0) _save_history_=0;
    else _save_history_=1;

    history_type=ui->combostyle_3->currentIndex();

    if(ui->playsoundcheckBox->isChecked())
    {
        if(ui->customsoundcheckBox->isChecked() && ui->browsesoundButton->text() != tr("(None)"))
        {
            _sound_notification_=2;
            settings.setValue("sound", true);
            settings.setValue("custom", true);
        }

        if(ui->browsesoundButton->text() == "(None)" || !ui->customsoundcheckBox->isChecked())
        {
            _sound_notification_=1;
            settings.setValue("sound", true);
            settings.setValue("custom", false);
        }
    }
        else
    {
        _sound_notification_=0;
        settings.setValue("sound", false);
        settings.setValue("custom", false);
    }
    close();
}

void preferences::on_playsoundcheckBox_clicked()
{
    if (ui->playsoundcheckBox->isChecked()){
        ui->customsoundcheckBox->setEnabled(true);
        if(ui->customsoundcheckBox->isChecked())
            ui->browsesoundButton->setEnabled(true);
    }
    else
    {
        ui->customsoundcheckBox->setEnabled(false);
        ui->customsoundcheckBox->setChecked(false);
        ui->browsesoundButton->setEnabled(false);
    }
}


void preferences::on_customsoundcheckBox_clicked()
{
    ui->browsesoundButton->setEnabled(ui->customsoundcheckBox->isChecked());
}

void preferences::on_browsesoundButton_clicked()
{
    QString path;
    if(QFile("/usr/bin/mpg321").exists())
        path = QFileDialog::getOpenFileName(this, tr("Choose Custom Sound"), QDir::homePath(), "*.mp3 *.wav *.ogg");
    else
        path = QFileDialog::getOpenFileName(this, tr("Choose Custom Sound"), QDir::homePath(), "*.wav *.ogg");

    if(!path.isEmpty()){
        if(system("echo \"\" > ~/.config/Wallch/Checks/text_to_button"))
            cerr << "Error writing to ~/.config/Wallch/Checks/text_to_button. Check file's permissions.\n";
        char *realpath = path.toUtf8().data();
        QString home16 = QDir::homePath()+"/.config/Wallch/Checks/text_to_button";
        QFile file(home16);
        if (!file.open(QIODevice::WriteOnly)) {
            cerr << "Cannot open file ~/.config/Wallch/Checks/text_to_button for writing. The reason: "
                 << qPrintable(file.errorString()) << endl;
            cerr << "Try again.";
            if(system("echo "" > ~/.config/Wallch/Checks/text_to_buton"))
                cerr << "Error, could not write to ~/.config/Wallch/Checks/text_to_buton. Check file permissions.\n";
        }
        QTextStream out(&file);
        out << QString::fromUtf8(realpath);
        file.close();
        if(system("basename \"$(cat ~/.config/Wallch/Checks/text_to_button)\" > ~/.config/Wallch/Checks/text_to_button_name_only;"))
            cerr << "Error reading/writing ~/.config/Wallch/Checks/text_to_button*. Please check files' permissions.\n";
        fstream infile;
        infile.open(QString(home16+"_name_only").toLocal8Bit().data()); // open file
        string s="";
        if(infile) {
            while(getline(infile, s)) {
                QString qw = QString::fromUtf8(s.c_str());
                ui->browsesoundButton->setText(qw);
            }
        }
    }
}

void preferences::on_pushButton_clicked()
{
    if(system("killall -v mpg321 canberra-gtk-play 2> /dev/null")!=256){ //stopping all the sounds
        cerr << "Error while trying to kill mpg321 and canberra-gtk-play\n";
    }
}

void preferences::on_pushButton_2_clicked()
{
    QMessageBox msgBox2;
    QPushButton *ok;
    msgBox2.setWindowTitle(tr("Reset Preferences"));
    msgBox2.setText(tr("Are you sure you want to reset your Wallch\npreferences?"));
    ok = msgBox2.addButton(tr("Ok"), QMessageBox::ActionRole);
    msgBox2.addButton(tr("Cancel"), QMessageBox::ActionRole);
    msgBox2.setIconPixmap(QIcon::fromTheme("dialog-question").pixmap(QSize(48,48)));
    msgBox2.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
    msgBox2.exec();
    if (msgBox2.clickedButton() == ok) {
        ui->unity_prog_checkbox->setChecked(true);
        ui->checkBox_5->setChecked(false);
        ui->radioButton->setChecked(false);
        ui->radioButton_2->setChecked(false);
        ui->radioButton_3->setChecked(false);
        ui->radioButton_4->setChecked(false);
        ui->radioButton->setEnabled(false);
        ui->radioButton_2->setEnabled(false);
        ui->radioButton_3->setEnabled(false);
        ui->radioButton_4->setEnabled(false);
        ui->playsoundcheckBox->setChecked(false);
        ui->customsoundcheckBox->setChecked(false);
        ui->checkBox_3->setChecked(false);
        ui->checkBox_4->setChecked(false);
        ui->checkBox_4->setChecked(true);
        ui->rotate_checkBox->setChecked(false);
        ui->combostyle_3->setCurrentIndex(2);
        on_applyButton_clicked();
    }
}

void preferences::on_combostyle_currentIndexChanged(int index)
{
    QString type;
    switch(index){
    case 0:
        type="wallpaper";
        break;
    case 1:
        type="zoom";
        break;
    case 2:
        type="centered";
        break;
    case 4:
        type="scaled";
        break;
    case 5:
        type="stretched";
        break;
    case 6:
        type="spanned";
        break;
     default:
        type="zoom";
        break;
    }

    if(gnome_version==2){
        if(system(QString("gconftool-2 --type string --set /desktop/gnome/background/picture_options "+type+"&").toLocal8Bit().data()))
            cerr << "Error while trying to change style. Change your home's permissions.\n";
    }
    else if(gnome_version==3){
        if(system(QString("gsettings set org.gnome.desktop.background picture-options "+type+"&").toLocal8Bit().data()))
                cerr << "Error while trying to change style. Change your home's permissions.\n";
    }
}

void preferences::on_pushButton_3_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void preferences::on_pushButton_4_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void preferences::on_pushButton_5_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}
