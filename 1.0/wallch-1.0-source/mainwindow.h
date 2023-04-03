/*WallpaperChanger -A tool for having fun changing desktop wallpapers-
Copyright (C) 2010 by Leon Vytanos and Alex Solanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QTimer>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include "about.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include "ui_mainwindow.h"
#include "properties.h"
#include "screenshot.h"
#include "preferences.h"



class QSystemTrayIcon;

namespace Ui {
    class MainWindow;
}
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QTimer *timer;
    QTimer *timer2;
    QTimer *ti2;
    QAction *Start_action;
    QAction *Pause_action;
    QAction *Stop_action;
    QAction *Next_action;
    QAction *Show_action;
    QAction *Settings_action;
    QAction *About_action;
    QAction *Quit_action;
    void closeEvent( QCloseEvent * );
    about *About;
    preferences *Preferences;
    screenshot *Screenshot;
    properties *Properties;


private slots:
    void on_actionContents_triggered();
    void on_spinBox_valueChanged(int );
    void on_addfolder_clicked();
    void on_spinBox_2_valueChanged(int );
    void on_webcamButton_clicked();
    void on_preview_clicked();
    void on_listWidget_customContextMenuRequested();
    void on_screenshotButton_clicked();
    void on_listWidget_itemDoubleClicked();
    void on_listWidget_itemSelectionChanged();
    void on_movedownButton_clicked();
    void on_moveupButton_clicked();
    void on_removeallButton_clicked();
    void on_removeButton_clicked();
    void on_addButton_clicked();
    void on_stopButton_clicked();
    void on_checkBox_2_clicked();
    void on_checkBox_clicked();
    void on_nextButton_clicked();
    void on_pauseButton_clicked();
    void on_startButton_clicked();
    void write_random_image();
    void write_random_time();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void album_slot(const QString &album);

signals:
     void minimized();

public slots:
    void clickSysTrayIcon(QSystemTrayIcon::ActivationReason reason);
    void ShowPreferences();
    void menushowabout();
    void checkForUpdate();
    void showClicked();
    void remove_history();
    void remove_list();
    void remove_disk();
    void bug();
    void load();
    void doubleonrightclick();
    void showProperties();
    void Get_Help_Online();
    void translatetogreek();
    void translatetoenglish();
    void translatetraytogreek();
    void translatetraytoenglish();
    void errorstopprocess();
    void close_minimize();
};


#endif // MAINWINDOW_H
