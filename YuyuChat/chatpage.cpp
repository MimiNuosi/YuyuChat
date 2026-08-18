#include "chatpage.h"
#include "ui_chatpage.h"
#include <QStyleOption>
#include <QPainter>
#include "chatitembase.h"
#include "global.h"
#include "textbubble.h"
#include "picturebubble.h"
#include "chatview.h"
#include "usermanager.h"
#include "tcpmanager.h"
ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
{
    ui->setupUi(this);
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    ui->title_label->setText(user_info->_name);
    ui->chat_data_list->removeAllItem();
    for(auto& msg : user_info->_chat_msgs){
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserManager::GetInstance()->GetUserInfo();
    ChatRole role;
    if (msg->GetSendUid() == self_info->_uid) {
        role = ChatRole::Self;
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->GetMsgContent());

        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
    else {
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserManager::GetInstance()->GetFriendById(msg->GetSendUid());
        if (friend_info == nullptr) {
            return;
        }
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->GetMsgContent());
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }


}
void ChatPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_button_clicked()
{
    if (_user_info == nullptr) {
        qDebug() << "[警告] 当前未选中任何聊天对象，取消发送";
        return;
    }

    auto self_info = UserManager::GetInstance()->GetUserInfo();
    if (!self_info) return;

    auto pTextEdit = ui->chat_edit;
    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    if (msgList.isEmpty()) return;

    QJsonObject textObj;
    QJsonArray textArray;

    auto thread_id = UserManager::GetInstance()->GetThreadIdByUid(_user_info->_uid);

    for (int i = 0; i < msgList.size(); ++i)
    {
        QString type = msgList[i].msgFlag;
        if (type == "text")
        {
            QString clean_content = msgList[i].content.trimmed();
            if (clean_content.isEmpty()) {
                continue; // 避免发送纯换行或空消息
            }

            QString uuidString = QUuid::createUuid().toString();

            QJsonObject obj;
            obj["content"] = clean_content;
            obj["msgid"] = uuidString;
            textArray.append(obj);

            auto txt_msg = std::make_shared<TextChatData>(
                uuidString, thread_id, ChatFormType::PRIVATE,
                ChatMsgType::TEXT, clean_content, self_info->_uid, 0
                );

            emit sig_append_send_chat_msg(txt_msg);
        }
    }

    if (!textArray.isEmpty()) {
        textObj["fromuid"] = self_info->_uid;
        textObj["touid"] = _user_info->_uid;
        textObj["text_array"] = textArray;

        QJsonDocument doc(textObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 发送给服务器
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_TEXT_CHAT_MSG_REQ, jsonData);
    }

    // 清空输入框
    pTextEdit->clear();
}