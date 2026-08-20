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

void ChatPage::SetChatData(std::shared_ptr<ChatThreadData> chat_data)
{
    _chat_data = chat_data; // 成员变量改为 _chat_data
    auto other_id = _chat_data->GetOtherId();

    // 1. 群聊分支判断
    if(other_id == 0) {
        ui->title_label->setText(_chat_data->GetGroupName());
        return;
    }

    // 2. 私聊获取好友信息
    auto friend_info = UserManager::GetInstance()->GetFriendById(other_id);
    if (friend_info == nullptr) return;
    ui->title_label->setText(friend_info->_name);

    // 3. 清屏并清空未确认气泡映射
    ui->chat_data_list->removeAllItem();
    _unrsp_item_map.clear();

    // 4. 双循环加载：已落库消息 + 未确认落库消息
    for(auto & msg : chat_data->GetMsgMapRef()){
        AppendChatMsg(msg);
    }
    for (auto& msg : chat_data->GetMsgUnRspRef()) {
        AppendChatMsg(msg);
    }
}


void ChatPage::AppendChatMsg(std::shared_ptr<ChatDataBase> msg)
{
    auto self_info = UserManager::GetInstance()->GetUserInfo();
    ChatRole role;
    ChatItemBase* pChatItem = nullptr;
    QWidget* pBubble = nullptr;

    if (msg->GetSendUid() == self_info->_uid) {
        role = ChatRole::Self;
        pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));
        if (msg->GetMsgType() == ChatMsgType::TEXT) {
            pBubble = new TextBubble(role, msg->GetMsgContent());
        }
    }
    else {
        role = ChatRole::Other;
        pChatItem = new ChatItemBase(role);
        auto friend_info = UserManager::GetInstance()->GetFriendById(msg->GetSendUid());
        if (!friend_info) return;
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));
        if (msg->GetMsgType() == ChatMsgType::TEXT) {
            pBubble = new TextBubble(role, msg->GetMsgContent());
        }
    }

    if (pBubble) {
        pChatItem->setWidget(pBubble);
        auto status = msg->GetStatus();
        pChatItem->setStatus(status); // 设置发送状态（0=转圈中, 2=已送达）
        ui->chat_data_list->appendChatItem(pChatItem);

        // 收集未确认的气泡指针，用于后续状态回填
        if (status == 0) {
            _unrsp_item_map[msg->GetUniqueId()] = pChatItem;
        }
    }
}

void ChatPage::UpdateChatStatus(const QString &unique_id, int status)
{
    auto iter = _unrsp_item_map.find(unique_id);
    if (iter != _unrsp_item_map.end()) {
        iter.value()->setStatus(status);
        if (status == 2) { // 2 代表成功送达
            _unrsp_item_map.erase(iter);
        }
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
    if (_chat_data == nullptr) {
        qDebug() << "friend_info is empty";
        return;
    }

    auto user_info = UserManager::GetInstance()->GetUserInfo();
    if (!user_info) return;

    auto pTextEdit = ui->chat_edit;
    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    if (msgList.isEmpty()) return;

    QJsonObject textObj;
    QJsonArray textArray;

    auto thread_id = _chat_data->GetThreadId();
    int txt_size = 0;

    for (int i = 0; i < msgList.size(); ++i)
    {
        if (msgList[i].content.length() > 1024) continue;

        QString type = msgList[i].msgFlag;
        ChatRole role = ChatRole::Self;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(user_info->_name);
        pChatItem->setUserIcon(QPixmap(user_info->_icon));
        QWidget *pBubble = nullptr;

        QString uuidString = QUuid::createUuid().toString();
        if (type == "text")
        {
            pBubble = new TextBubble(role, msgList[i].content);

            // 超长分包保护
            if (txt_size + msgList[i].content.length() > 1024) {
                textObj["fromuid"] = user_info->_uid;
                textObj["touid"] = _chat_data->GetOtherId();
                textObj["thread_id"] = thread_id;
                textObj["text_array"] = textArray;
                QJsonDocument doc(textObj);
                emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_TEXT_CHAT_MSG_REQ, doc.toJson(QJsonDocument::Compact));

                txt_size = 0;
                textArray = QJsonArray();
                textObj = QJsonObject();
            }

            txt_size += msgList[i].content.length();
            QJsonObject obj;
            obj["content"] = msgList[i].content;
            obj["unique_id"] = uuidString;
            textArray.append(obj);

            auto txt_msg = std::make_shared<TextChatData>(
                uuidString, thread_id, ChatFormType::PRIVATE,
                ChatMsgType::TEXT, msgList[i].content, user_info->_uid, 0
                );

            // 存入当前会话的未回复池
            _chat_data->AppendUnRspMsg(uuidString, txt_msg);
        }
        if (pBubble != nullptr) {
            pChatItem->setWidget(pBubble);
            pChatItem->setStatus(0); // 初始为发送中
            ui->chat_data_list->appendChatItem(pChatItem);
            _unrsp_item_map[uuidString] = pChatItem; // 建立 UI 映射
        }
    }

    if (!textArray.isEmpty()) {
        textObj["text_array"] = textArray;
        textObj["fromuid"] = user_info->_uid;
        textObj["touid"] = _chat_data->GetOtherId();
        textObj["thread_id"] = thread_id;
        QJsonDocument doc(textObj);
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_TEXT_CHAT_MSG_REQ, doc.toJson(QJsonDocument::Compact));
    }

    // 清空输入框
    pTextEdit->clear();
}