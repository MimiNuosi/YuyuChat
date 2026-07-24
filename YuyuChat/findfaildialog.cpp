#include "findfaildialog.h"
#include "ui_findfaildialog.h"

FindFailDialog::FindFailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindFailDialog)
{
    ui->setupUi(this);
    setWindowTitle("添加");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FailFindDialog");
    this->setModal(true);
}

FindFailDialog::~FindFailDialog()
{
    delete ui;
}

void FindFailDialog::on_fail_sure_button_clicked()
{
    this->hide();
}

