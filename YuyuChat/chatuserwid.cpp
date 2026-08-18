#include "chatuserwid.h"
#include "usermanager.h"
#include "ui_chatuserwid.h"

ChatUserWid::ChatUserWid(QWidget *parent)
    :  ListItemBase(parent)
    , ui(new Ui::ChatUserWid)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CHAT_USER_ITEM);
}

ChatUserWid::~ChatUserWid()
{
    delete ui;
}

QSize ChatUserWid::sizeHint() const  {
    return QSize(250, 70); // 返回自定义的尺寸
}

void ChatUserWid::SetChatInfo(std::shared_ptr<ChatThreadData> chat_data)
{
    chat_data = _chat_data;
    auto other_id = chat_data->GetOtherId();
    auto other_info = UserManager::GetInstance()->GetFriendById(other_id);

    // 加载图片
    QPixmap pixmap(other_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);

    ui->user_name_label->setText(other_info->_name);
    ui->user_chat_label->setText(other_info->_last_msg);
}

std::shared_ptr<ChatThreadData> ChatUserWid::GetChatInfo()
{
    return _chat_data;
}

void ChatUserWid::UpdateLastMsg(std::vector<std::shared_ptr<TextChatData>> msgs)
{
    int last_msg_id = 0;
    QString last_msg = "";
    for (auto& msg : msgs) {
        last_msg = msg->GetContent();
        last_msg_id = msg->GetMsgId();
        _chat_data->AddMsg(msg);
    }

    _chat_data->SetLastMsgId(last_msg_id);
    ui->user_chat_label->setText(last_msg);
}

void ChatUserWid::ShowRedPoint(bool b_show)
{

}