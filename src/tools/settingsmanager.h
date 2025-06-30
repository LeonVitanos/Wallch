#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include "featurecontroller.h"

extern QSettings *settings;

class SettingsManager
{
public:
    SettingsManager();
    static void initializeSettings();
    static void updateStartup(FeatureController::Feature currentFeature);
    static void setDefaultFolder(const QString &folder);

private:
    static void loadSettings();
    static void checkFirstRun();
    static void loadPaths();
};

#endif // SETTINGSMANAGER_H
