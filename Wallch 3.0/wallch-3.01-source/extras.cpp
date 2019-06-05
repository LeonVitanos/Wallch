#define QT_NO_KEYWORDS

#include "extras.h"
#include "ui_extras.h"
#include "glob.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <QTime>
#include <QDir>

#include <iostream>
using namespace std;

Extras::Extras(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Extras)
{
    ui->setupUi(this);
    QSettings settings( "Wallch", "Extras" );
    bool active_checked = settings.value("monitored_on", false).toBool();
    QString hour_minute=settings.value("hour_min_photoofday", "0:0").toString();
    QStringList hour_minute_entries = hour_minute.split(":");
    QTime time;
    time.setHMS(hour_minute_entries.at(0).toInt(),hour_minute_entries.at(1).toInt(), 0);
    ui->hour_min->setTime(time);

    ui->activate_monitor->setEnabled(!active_checked);
    ui->deactivate_monitor->setEnabled(active_checked);
    int size = settings.beginReadArray("monitored_folders");
    if(active_checked){
        for (int i = 0; i < size; ++i){
            settings.setArrayIndex(i);
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(settings.value("item").toString());
            if(QDir(settings.value("item").toString()).exists())
                item->setBackgroundColor(QColor::fromRgb(127,255,0));
            else
                item->setBackgroundColor(QColor::fromRgb(255, 0, 0));
            ui->listWidget->addItem(item);
        }
        settings.endArray();
    }
    else
    {
        for (int i = 0; i < size; ++i){
            settings.setArrayIndex(i);
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(settings.value("item").toString());
            ui->listWidget->addItem(item);
        }
        settings.endArray();
    }

    ui->toolBox->setItemIcon(0, QIcon::fromTheme("emblem-web"));
    ui->toolBox->setItemIcon(1, QIcon::fromTheme("folder"));
    ui->toolBox->setItemIcon(2, QIcon::fromTheme("image-x-generic"));
    ui->toolBox->setItemIcon(3, QIcon::fromTheme("help-about"));
    ui->add->setIcon(QIcon::fromTheme("list-add"));
    ui->remove->setIcon(QIcon::fromTheme("list-remove"));
    if(_live_earth_running_)
        ui->pushButton_6->setEnabled(false);
    else
    {
        //if live earth isn't running, then maybe photo of day is running...
        if(_photo_of_day_running_)
            ui->activate_photoofday->setEnabled(false);
        else
            ui->deactivate_photoofday->setEnabled(false);
        ui->pushButton_7->setEnabled(false);
    }

    ui->label_3->setText(QString(tr("Picture of the day is being chosen from")+" "+"<a href=\"en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">%1</span></a>").arg("Wikipedia"));
    ui->label_5->setText(QString(tr("Translate Wallch to your language. Learn more")+" "+"<a href=\"wall-changer.sourceforge.net/helpus.html\"><span style=\" font-family:'Sans'; font-size:10pt; text-decoration: underline; color:#ff5500;\">%1</span></a>").arg(tr("here")));
    if(!QFile(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg").exists()){
        ui->save_pic_ofday->setEnabled(false);
    }

}

Extras::~Extras()
{
    delete ui;
}

void Extras::closeEvent( QCloseEvent * )
{
    //writing current values
    QSettings settings( "Wallch", "Extras" );
    QTime time = ui->hour_min->time();
    settings.setValue("hour_min_photoofday",QString::number(time.hour())+":"+QString::number(time.minute()));
    settings.sync();
    //we don't like duplicate folders, do we?
    int list_count=ui->listWidget->count();
    QStringList folders;
    for(int i=0; i < list_count; i++)
        folders << ui->listWidget->item(i)->text();
    folders.removeDuplicates();
    int folders_count = folders.count();
    settings.setValue("monitored_on", ui->listWidget->count()&&ui->deactivate_monitor->isEnabled());
    settings.beginWriteArray("monitored_folders");
    for (int i = 0; i < folders_count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("item", folders.at(i));
    }
    settings.endArray();
}

void Extras::on_closeButton_clicked()
{
    close();
}

void Extras::on_pushButton_6_clicked()
{
    if(_start_running_){
    //Start is running, in order to avoid conflicts display an error message!
        QMessageBox::warning(this, tr("Live Earth Wallpaper"), tr("Please stop the current process and try again."));
        return;
    }
    else if(_photo_of_day_running_){
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" "+tr("(Picture Of The Day)"));
        return;
    }
    Q_EMIT start_live_earth();
}

void Extras::on_pushButton_7_clicked()
{
    _live_earth_running_=0;
    Q_EMIT stop_live_earth();
    ui->pushButton_7->setEnabled(false); ui->pushButton_6->setEnabled(true);
}

void Extras::fix_buttons_livearth(){
    ui->pushButton_6->setEnabled(false);
    ui->pushButton_7->setEnabled(true);
}

void Extras::fix_buttons_photoofday(){
    ui->deactivate_photoofday->setEnabled(true);
    ui->activate_photoofday->setEnabled(false);
    ui->save_pic_ofday->setEnabled(true);
}
void Extras::on_add_clicked()
{
    QString qpath = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), QDir::homePath());
    if(qpath.isEmpty()) return;
    int list_count=ui->listWidget->count();
    for(int i=0;i<list_count;i++){
        if(qpath==ui->listWidget->item(i)->text())
            return;
    }
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(qpath);
    if(ui->deactivate_monitor->isEnabled()){
        Q_EMIT monitored_on(qpath);
        item->setBackgroundColor(QColor::fromRgb(127,255,0));
    }
    ui->listWidget->addItem(item);
}

void Extras::on_remove_clicked()
{
    QString cur_item;
    if(ui->listWidget->currentIndex().isValid())
        cur_item=ui->listWidget->currentItem()->text();
    else
        return;
    if (ui->listWidget->count()==1){
        ui->listWidget->clear();
        on_deactivate_monitor_clicked();
    }
    else
        delete ui->listWidget->currentItem();
    if(ui->deactivate_monitor->isEnabled() && !cur_item.isEmpty())
        Q_EMIT monitored_off(cur_item);
    else if(ui->deactivate_monitor->isEnabled() && !ui->listWidget->count()){
        //monitoring is actually off...
        Q_EMIT monitored_off("all");
    }
}

void Extras::on_label_3_linkActivated()
{
    if(system("xdg-open http://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day"))
        cerr << "Error while opening http://en.wikipedia.org/wiki/Wikipedia:Picture_of_the_day\n";
}

void Extras::on_label_5_linkActivated()
{
    if(system("xdg-open http://wall-changer.sourceforge.net/helpus.html"))
        cerr << "Error while opening http://wall-changer.sourceforge.net/helpus.html\n";
}

QString Extras::get_ext(QString filename){
    QString extension=filename.right(3);
    if(extension=="PNG" || extension=="png")
        return QString("png");
    else if(extension=="JPG" || extension=="PEG" || extension=="jpg" || extension=="peg")
        return QString("jpg");
    else if(extension=="GIF" || extension=="gif")
        return QString("gif");
    else
        return QString("png");
}

void Extras::on_activate_photoofday_clicked()
{
    if(_start_running_){
    //Start is running, in order to avoid conflicts display an error message!
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Photo of day"));
        msgBox.setText(tr("Please stop the current process and try again."));
        msgBox.setIconPixmap(QIcon(":/icons/Pictures/earth.png").pixmap(QSize(100,100)));
        msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec();
        return;
    }
    else if(_live_earth_running_){
        QMessageBox::warning(this, tr("Error"), tr("Please stop the current process and try again.")+" "+tr("(Live Earth Wallpaper)"));
        return;
    }
    if (!Global::connected_to_internet()){ //Unable to reach network
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Live Earth Wallpaper"));
        msgBox.setText("<b>"+tr("Unable to connect")+".</b><br>"+tr("Check your internet connection!"));
        msgBox.setIconPixmap(QIcon::fromTheme("info").pixmap(QSize(100,100)));
        msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec();
        return;
    }
    _photo_of_day_running_=1;
    _photo_of_day_show_message_=1;
    QTime time=ui->hour_min->time();
    QString hour_min_photoofday=QString::number(time.hour())+":"+QString::number(time.minute());
    Q_EMIT start_photo_of_day(hour_min_photoofday);
    ui->deactivate_photoofday->setEnabled(true);
    ui->activate_photoofday->setEnabled(false);
}

void Extras::on_save_pic_ofday_clicked()
{
    if(QFile(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg").exists()){
        QString format=".jpg";
        QString initialPath = QDir::homePath() + "/pic_of_day" + format;
        QString filename = QFileDialog::getSaveFileName(this, tr("Save As"),
                                           initialPath,
                                           tr("%1 Files (*.%2);;All Files (*)")
                                           .arg(format.toUpper())
                                           .arg(format));
        if(!filename.isEmpty()){
            if(QFile(filename).exists())
                QFile::remove(filename);
            if(!QFile::copy(QDir::homePath()+"/.config/Wallch/photo_of_day.jpg", filename)){
                QMessageBox::warning(this, tr("Error"), tr("Something went wrong while saving the file!"));
                return;
            }
        }
    }
    else{
        QMessageBox::warning(this, tr("Error"), tr("You don't seem to have downloaded any picture yet!"));
    }
}

void Extras::on_deactivate_photoofday_clicked()
{
    _photo_of_day_running_=0;
    Q_EMIT stop_photo_of_day();
    ui->activate_photoofday->setEnabled(true);
    ui->deactivate_photoofday->setEnabled(false);
}

void Extras::on_pushButton_clicked()
{
    if(system("xdg-open \"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=JZFVMZQFHN4NE\"&"))
        cerr << "Error while opening \"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=JZFVMZQFHN4NE\"\n";
}

void Extras::on_activate_monitor_clicked()
{
    if(!ui->listWidget->count())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Monitor Folder"));
        msgBox.setText(tr("You must have added at least one folder."));
        msgBox.setIconPixmap(QIcon::fromTheme("dialog-information").pixmap(QSize(56,56)));
        msgBox.setWindowIcon(QIcon(":/icons/Pictures/wallch.png"));
        msgBox.exec();
        return;
    }
    ui->deactivate_monitor->setEnabled(true);
    ui->activate_monitor->setEnabled(false);
    Q_EMIT list_clear();
    for(int i=0; i<ui->listWidget->count(); i++){
        if(QDir(ui->listWidget->item(i)->text()).exists()){
            ui->listWidget->item(i)->setBackgroundColor(QColor::fromRgb(127,255,0));
            Q_EMIT monitored_on(ui->listWidget->item(i)->text());
        }
        else
        {
            ui->listWidget->item(i)->setBackgroundColor(QColor::fromRgb(255, 0, 0));
        }
    }
}

void Extras::on_deactivate_monitor_clicked()
{
    ui->deactivate_monitor->setEnabled(false);
    ui->activate_monitor->setEnabled(true);
    for(int i=0; i<ui->listWidget->count(); i++){
        ui->listWidget->item(i)->setBackgroundColor(QColor::fromRgb(255, 255, 255));
    }
    Q_EMIT monitored_off("all");
}
