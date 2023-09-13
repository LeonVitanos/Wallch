#ifndef DIALOGHELPER_H
#define DIALOGHELPER_H

#include <QMessageBox>

#include "properties.h"
#include "wallpapermanager.h"

class DialogHelper : public QObject
{
    Q_OBJECT

public:
    DialogHelper(QObject *parent = 0);

private:
    Properties *properties_;
    bool propertiesShown_ = false;

public Q_SLOTS:
    void showPropertiesDialog(int currentIndex = -1, WallpaperManager *wallpaperManager = 0, QString filePath = "");

private Q_SLOTS:
    void propertiesDestroyed();


};

#endif // DIALOGHELPER_H
