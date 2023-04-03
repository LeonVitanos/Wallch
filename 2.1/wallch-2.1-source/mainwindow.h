/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vytanos

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "about.h"
#include "ui_mainwindow.h"
#include "properties.h"
#include "screenshot.h"
#include "preferences.h"
#include "MyCameraWindow.h"
#include "history.h"
#include "extras.h"

#include <QTimer>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QDragEnterEvent>
#include <QDropEvent>




class QSystemTrayIcon;

namespace Ui {
    class MainWindow;
}
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QStringList previous_pictures_from_main, QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QTimer *update_time;
    QTimer *statistic_timer;
    QTimer *earth_timer;
    QAction *Start_action;
    QAction *Stop_action;
    QAction *Next_action;
    QAction *Previous_action;
    QAction *Show_action;
    QAction *Settings_action;
    QAction *About_action;
    QAction *Quit_action;
    void closeEvent( QCloseEvent * );
    about *About;
    preferences *Preferences;
    screenshot *Screenshot;
    properties *Properties;
    history *History;
    Extras *extras;
    MyCameraWindow *mainWin;


private Q_SLOTS:
    void on_actionHistory_triggered();
    void on_actionStatistics_triggered();
    void on_previousButton_clicked();
    void on_actionContents_triggered();
    void on_addfolder_clicked();
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
    void on_nextButton_clicked();
    void on_startButton_clicked();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void dbus_action(const QString &msg);
    void start_normal();
    void time_updater();
    void desktop_notify(QString qimage, int notify);
    void sound_notify();
    void save_history(QString qimage);
    void add_to_stats();
    void disable_start_buttons();
    void enable_start_buttons();
    void disable_stop_buttons();
    void enable_stop_buttons();
    void disable_previous_and_next_buttons();
    void enable_previous_and_next_buttons();
    void on_timerSlider_valueChanged(int value);
    void livearth();
    void stop_livearth();
    void add_new_album();
    void start_livearth_extras_concurrent_thread();
    void livearth_extras();
    static void handle();
    void on_checkBox_2_clicked();

Q_SIGNALS:
     void minimized();
     void fix_livearth_buttons();

public Q_SLOTS:
    void clickSysTrayIcon(QSystemTrayIcon::ActivationReason reason);
    void ShowPreferences();
    void ShowExtras();
    void menushowabout();
    void checkForUpdate();
    void showClicked();
    void remove_list();
    void remove_disk();
    void bug();
    void load();
    void showProperties();
    void Openfolder();
    void rotate_right();
    void rotate_left();
    void copy_image();
    void copy_path();
    void Get_Help_Online();
    void errorstopprocess();
    void close_minimize();
    void hide_mainwindow();
    void show_mainwindow();
    void save_album();

};


#endif // MAINWINDOW_H
