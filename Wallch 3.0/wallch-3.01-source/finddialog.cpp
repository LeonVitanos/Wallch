#include "finddialog.h"
#include "ui_finddialog.h"

bool text_edited=false;

FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    ui->term->setFocus();
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::on_term_textChanged()
{
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::black);
    ui->term->setPalette(pal);
    text_edited=true;
}

void FindDialog::doesnt_match(){
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::red);
    ui->term->setPalette(pal);
    text_edited=true;
}

void FindDialog::on_case_sensitive_clicked()
{
    ui->term->setFocus();
    QPalette pal;
    pal.setColor(QPalette::Text, Qt::black);
    ui->term->setPalette(pal);
    text_edited=true;
}

void FindDialog::on_search_clicked()
{
    /*
      Note, the code bellow will search for images even if
      the color is red, which means that the text doesnt really
      exist, but this is in purpose, because even if the term
      didn't exist, if folder monitoring is active, a new picture
      may has been added and the term then may exist.
    */
    ui->term->setFocus();
    if(ui->term->text().isEmpty())
        return;
    bool case_sens;
    if(ui->case_sensitive->isChecked())
        case_sens=true;
    else
        case_sens=false;
    if(text_edited){
        text_edited=false;
        Q_EMIT send_term(ui->term->text(), case_sens);
    }
    else
        Q_EMIT continue_search();
}
