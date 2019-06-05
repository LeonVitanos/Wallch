#ifndef PICTURES_LOCATIONS_H
#define PICTURES_LOCATIONS_H

#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QTreeWidgetItem>

class TreeWidgetDrop : public QTreeWidget
{
    Q_OBJECT

public:
    explicit TreeWidgetDrop(QWidget *parent = 0);
    void fixPositions();

private:
    QTreeWidgetItem *pictures_item;
    QTreeWidgetItem *deBackgrounds_item;

protected:
    virtual void dropEvent(QDropEvent *event);
};

namespace Ui {
class pictures_locations;
}

class PicturesLocations : public QDialog
{
    Q_OBJECT

public:
    TreeWidgetDrop *treeWidgetDrop;
    explicit PicturesLocations(QWidget *parent = 0);
    ~PicturesLocations();

private Q_SLOTS:
    void on_add_location_clicked();
    void on_remove_location_clicked();
    void on_foldersTreeWidget_currentItemChanged(QTreeWidgetItem *current);
    void on_cancel_pushButton_clicked();
    void on_save_pushButton_clicked();
    void on_reset_pushButton_clicked();

private:
    Ui::pictures_locations *ui;

Q_SIGNALS:
    void picturesLocationsChanged();
};

#endif // PICTURES_LOCATIONS_H
