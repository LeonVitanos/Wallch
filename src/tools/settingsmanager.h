#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include "glob.h"
#include "filemanager.h"

class SettingsManager
{
public:
    SettingsManager();
    static void initializeSettings();
    static void updateStartup();
    static void setDefaultFolder(const QString &folder);

private:
    static void loadSettings();
    static void checkFirstRun();
    static void loadPaths();
};

#endif // SETTINGSMANAGER_H
