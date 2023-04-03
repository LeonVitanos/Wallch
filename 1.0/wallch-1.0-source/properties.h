#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>

namespace Ui {
    class properties;
}

class properties : public QDialog
{
    Q_OBJECT

public:
    explicit properties(QWidget *parent = 0);
    ~properties();

private:
    Ui::properties *ui;

private slots:
    void on_pushButton_clicked();
};

#endif // PROPERTIES_H
