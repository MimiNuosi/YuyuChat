#include "chatuserwid.h"
#include "ui_chatuserwid.h"

ChatUserWid::ChatUserWid(QWidget *parent)
    :  ListItemBase(parent)
    , ui(new Ui::ChatUserWid)
{
    ui->setupUi(this);
}

ChatUserWid::~ChatUserWid()
{
    delete ui;
}

QSize ChatUserWid::sizeHint() const  {
    return QSize(250, 70); // 返回自定义的尺寸
}

void ChatUserWid::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    // 加载图片
    QPixmap pixmap(user_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);

    ui->user_name_label->setText(user_info->_name);
    ui->user_chat_label->setText(user_info->_last_msg);
}