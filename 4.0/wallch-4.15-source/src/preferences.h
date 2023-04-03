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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QHBoxLayout>

class LineEditShortCut : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEditShortCut(QWidget *parent = 0) : QLineEdit(parent){
        this->setReadOnly(true);
        this->setFocusPolicy(Qt::ClickFocus);
        gotCtrlKey_=gotAltKey_=gotSomeKey_=false;
    };

private:
    bool gotCtrlKey_;
    bool gotAltKey_;
    bool gotSomeKey_;
    void insertAtEnd(const QString &text){
        this->setCursorPosition(this->text().count());
        this->insert(text);
    }

protected:
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void keyReleaseEvent(QKeyEvent *);
};

namespace Ui {
    class preferences;
}

class Preferences : public QDialog {
    Q_OBJECT
public:
    LineEditShortCut *lineEditShortcut;
    Preferences(QWidget *parent = 0);
    ~Preferences();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::preferences *ui;
    QHBoxLayout *buttonsLayout_;
    bool shortcutWasCheckedAtFirst_;
    bool historyWasEnabledAtFirst_=false;
    bool textOfShortcutChanged_;
    bool listIsAlreadyIcons_;
    bool customIntervalsChanged_;
    bool addNewIntervalIsShown_;
    QString textOfShortcutInitially_;
    QList<int> customIntervalsInSeconds_temp;
    void setThemeToAmbiance();
    void setThemeToRadiance();
    void setupShortcuts();
    void setupThemeFallbacks();
    bool createDesktopFile(const QString &path, const QString &command, const QString &comment);
    QString getCommandOfDesktopFile(const QString &file);

private Q_SLOTS:
    void shortcutEdited();
    void previousPage();
    void nextPage();
    void on_reset_clicked();
    void on_saveButton_clicked();
    void on_closeButton_clicked();
    void on_shortcut_checkbox_clicked(bool checked);
    void on_random_time_from_combobox_currentIndexChanged(int index);
    void on_random_time_to_combobox_currentIndexChanged(int index);
    void on_page_0_general_clicked();
    void on_page_1_wallpapers_page_clicked();
    void on_page_3_integration_clicked();
    void on_theme_combo_currentIndexChanged(int index);
    void on_random_time_checkbox_clicked(bool checked);
    void on_add_custom_interval_clicked();
    void on_remove_custom_interval_clicked();
    void on_rotate_checkBox_clicked(bool checked);
    void on_random_from_valueChanged(int value);
    void on_random_to_valueChanged(int value2);
    void on_page_2_live_website_clicked();
#ifdef ON_LINUX
    void on_de_combo_currentIndexChanged(int index);
#endif
    void on_startupCheckBox_clicked(bool checked);
    void on_help_clicked();

Q_SIGNALS:
    void changePathsToIcons();
    void changeIconsToPaths();
    void bindKeySignal(QString key);
    void unbindKeySignal(QString key);
    void changeThemeTo(QString theme);
    void changeRandomTime(short index);
    void refreshCustomIntervals();
    void tvPreviewChanged(bool show);
    void unityProgressbarChanged(bool show);
};

#endif // PREFERENCES_H
