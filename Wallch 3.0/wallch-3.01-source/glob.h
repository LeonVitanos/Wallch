/*Wallch - WallpaperChanger
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2011 by Alex Solanos and Leon Vitanos

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

#define QT_NO_KEYWORDS

#ifndef GLOB_H
#define GLOB_H

/*EXTERN INTS START*/
//the Start's int (not to have any problem with the Live Earth wallpaper (1 if running)
extern int _start_running_;
extern int _live_earth_running_;
extern int _photo_of_day_running_;
extern int _on_boot_once_running_;
extern int _on_boot_constant_running_;
//other
extern int minimize_to_tray;
extern int _show_notification_;
extern int _sound_notification_;
extern int _save_history_;
extern int __statistic__images_n;
extern int __statistic__etimer_elapsed;
extern int pref;
extern int prop;
extern int camera_open;
extern int camera_success;
extern int _new_album_while_on_command_line_mode_;
extern int history_type;
extern int already_checked_from_main;
extern int history_type;
extern int unity_prog;
extern int _live_earth_show_message_;
extern int _photo_of_day_show_message_;
extern int _connected_to_net_;
extern int gnome_version;
extern int hideapp;

/*EXTERN INTS END*/

#include <QNetworkReply>
#include <QTcpSocket>
#include <QTimer>

#include "unity/unity/unity.h"

class Global: public QObject
{
    Q_OBJECT
public:
    static void livearth();
    static void enable_unity(UnityLauncherEntry *entry, bool state);
    static void set_unity_value(UnityLauncherEntry *entry, int timeout, int delay);
    static void save_history(QString qimage);
    static void sound_notify();
    static void handle ();
    static void rotate_img(QString filename, int rotation_type);
    static void photo_of_day_now();
    static bool desktop_notify(QString qimage, int message_type);
    static bool connected_to_internet();
    static bool already_runs();
    static bool set_background(QString qimage);

private Q_SLOTS:
    void read_file(QNetworkReply *reply);
    void save_image(QNetworkReply *data);
    void start_photo_of_day_non_static();

public Q_SLOTS:
    void download_image(QString url);
};
#endif // GLOB_H
