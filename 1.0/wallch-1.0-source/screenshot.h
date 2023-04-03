#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QDialog>

namespace Ui {
    class screenshot;
}

class screenshot : public QDialog
{
    Q_OBJECT

public:
    explicit screenshot(QWidget *parent = 0);
    ~screenshot();

private:
    Ui::screenshot *ui;
    void updateScreenshotLabel();

        QPixmap originalPixmap;

    private slots:
        void on_QuitButton_clicked();
        void on_saveScreenshot_clicked();
        void on_newScreenshotButton_clicked();
        void shootScreen();
};

#endif // SCREENSHOT_H
