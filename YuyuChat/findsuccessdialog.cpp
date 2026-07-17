#include "findsuccessdialog.h"
#include "ui_findsuccessdialog.h"
#include "applyfriend.h"
FindSuccessDialog::FindSuccessDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindSuccessDialog)
{
    ui->setupUi(this);
    // 设置对话框标题
    setWindowTitle("添加");
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    // 获取当前应用程序的路径
    QPixmap head_pix(":/res/head_1.jpg");
    head_pix = head_pix.scaled(ui->head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_label->setPixmap(head_pix);
    ui->head_label->setPixmap(head_pix);
    this->setModal(true);

    connect(ui->add_friend_button, &QPushButton::clicked, this, [=](){
        // 1. 先创建新窗口
        ApplyFriend* applyFriendDlg = new ApplyFriend(parent->window());

        // 2. (可选) 如果 ApplyFriend 需要知道当前正在添加谁
        // applyFriendDlg->SetSearchInfo(this->_si);

        // 3. 🌟 关键：先让新窗口显示出来！抢占焦点！
        applyFriendDlg->show();

        // 4. 🌟 关键：最后再隐藏旧窗口
        this->hide();
    });
}

FindSuccessDialog::~FindSuccessDialog()
{
    qDebug()<<"FindSuccessDlg destruct";
    delete ui;
}

void FindSuccessDialog::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    ui->name_label->setText(si->_name);
    _si = si;
}

void FindSuccessDialog::on_add_friend_btn_clicked()
{
    //todo... 添加好友界面弹出
}