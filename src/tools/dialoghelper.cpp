#include "dialoghelper.h"

DialogHelper::DialogHelper(QObject *parent):
    QObject(parent)
{}

void DialogHelper::showPropertiesDialog(int currentIndex, WallpaperManager *wallpaperManager, QString filePath)
{
    if(propertiesShown_)
        return;

    QString imagePath = !filePath.isEmpty() ? filePath :
                            currentIndex > -1 ? wallpaperManager->allWallpapers_.at(currentIndex) : wallpaperManager->currentBackgroundWallpaper();

    if (imagePath.isEmpty())
        QMessageBox::warning(0, tr("Properties"), tr("Error fetching the image file path."));
    else if(WallpaperManager::imageIsNull(imagePath))
        QMessageBox::warning(0, tr("Properties"), "\"" + imagePath + "\" " + tr("maybe doesn't exist or it's not an image. Please perform a check for the file and try again."));
    else {
        propertiesShown_=true;
        properties_ = new Properties(currentIndex, wallpaperManager, filePath);
        properties_->setModal(true);
        properties_->setAttribute(Qt::WA_DeleteOnClose);
        connect(properties_, SIGNAL(destroyed()), this, SLOT(propertiesDestroyed()));
        properties_->show();
    }
}

void DialogHelper::propertiesDestroyed(){
    propertiesShown_=false;
}
