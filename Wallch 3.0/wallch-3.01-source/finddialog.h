#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>

namespace Ui {
    class FindDialog;
}

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = 0);
    ~FindDialog();

private:
    Ui::FindDialog *ui;

Q_SIGNALS:
    void send_term(QString term, bool case_sens);
    void continue_search();
private Q_SLOTS:
    void on_term_textChanged();
    void on_case_sensitive_clicked();
    void on_search_clicked();
    void doesnt_match();
};

#endif // FINDDIALOG_H
