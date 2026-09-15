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

    qDebug() << "[UI渲染] 当前会话 thread_id=" << chat_data->GetThreadId()
             << " 已落库消息数=" << chat_data->GetMsgMapRef().size()
             << " 待确认消息数=" << chat_data->GetMsgUnRspRef().size();

    //  遍历已落库消息
    for (auto& msg : chat_data->GetMsgMapRef()) {
        qDebug() << "[UI渲染] 正在添加气泡: " << msg->GetMsgContent();
        AppendChatMsg(msg);
    }
    // 遍历待确认消息
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
        pChatItem->setUserIcon(self_info->_icon);
        if (msg->GetMsgType() == ChatMsgType::TEXT) {
            pBubble = new TextBubble(role, msg->GetMsgContent());
        }
        else if (msg->GetMsgType() == ChatMsgType::PIC) {
            auto img_msg = std::dynamic_pointer_cast<ImgChatData>(msg);
            // 自己发的消息或本地有缓存的，直接用缩略图/原图；无图时放默认占位
            QPixmap pixmap = (img_msg && img_msg->_msg_info && !img_msg->_msg_info->_preview_pix.isNull())
                                 ? img_msg->_msg_info->_preview_pix
                                 : QPixmap(":/res/pic_loading.png");
            pBubble = new PictureBubble(pixmap, role);
        }
    }
    else {
        role = ChatRole::Other;
        auto friend_info = UserManager::GetInstance()->GetFriendById(msg->GetSendUid());
        if (!friend_info) return;
        pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(friend_info->_icon);
        if (msg->GetMsgType() == ChatMsgType::TEXT) {
            pBubble = new TextBubble(role, msg->GetMsgContent());
        }
        else if (msg->GetMsgType() == ChatMsgType::PIC) {
            auto img_msg = std::dynamic_pointer_cast<ImgChatData>(msg);
            // 对方的图：检查本地缓存是否有该图；若无则展示加载占位图
            QPixmap pixmap;
            if (img_msg && img_msg->_msg_info && !img_msg->_msg_info->_preview_pix.isNull()) {
                pixmap = img_msg->_msg_info->_preview_pix;
            } else {
                pixmap = QPixmap(":/res/pic_loading.png");
            }

            auto* pic_bubble = new PictureBubble(pixmap, role);
            // 标记为等待/下载状态
            pic_bubble->setTransferState(TransferState::Downloading);
            pBubble = pic_bubble;
        }
    }

    if (pBubble) {
        pChatItem->setWidget(pBubble);
        auto status = msg->GetStatus();
        pChatItem->setStatus(status); // 设置发送状态（0=转圈中, 2=已送达）
        ui->chat_data_list->appendChatItem(pChatItem);

        // 收集未确认的气泡指针，用于后续状态回填
        if (status == MessageStatus::UN_READ) {
            _unrsp_item_map[msg->GetUniqueId()] = pChatItem;
        } else {
            // 已经落库的消息，直接以 msg_id 注册进 _base_item_map
            _base_item_map[msg->GetMsgId()] = pChatItem;
        }
        if (msg->GetMsgType() == ChatMsgType::PIC) {
            auto img_msg = std::dynamic_pointer_cast<ImgChatData>(msg);
            if (img_msg && img_msg->_msg_info) {
                int msg_id = msg->GetMsgId();
                _msg_id_to_img_info[msg_id] = img_msg->_msg_info;

                auto* pic_bubble = dynamic_cast<PictureBubble*>(pBubble);
                if (pic_bubble) {
                    connect(pic_bubble, &PictureBubble::sig_clicked, this, [this, msg_id]() {
                        qDebug() << "[图片查看] 点击了图片，msg_id=" << msg_id;
                    });
                }
            }
        }
    }
}

void ChatPage::UpdateChatStatus(std::shared_ptr<ChatDataBase> msg)
{
    if (!msg) return;

    auto iter = _unrsp_item_map.find(msg->GetUniqueId());
    if (iter == _unrsp_item_map.end()) {
        return;
    }

    auto* item = iter.value();

    // 1. 公共逻辑：更新气泡发送状态（送达/失败/等待）
    item->setStatus(msg->GetStatus());

    // 2. 公共逻辑：从未确认池转移到以 msg_id 寻址的基表，并清除待确认记录
    _base_item_map[msg->GetMsgId()] = item;
    _unrsp_item_map.erase(iter);

    // 3. 差异化逻辑：如果是图片类型，提取 ImgChatData 并为 PictureBubble 绑定传输元信息
    if (msg->GetMsgType() == ChatMsgType::PIC) {
        auto img_msg = std::dynamic_pointer_cast<ImgChatData>(msg);
        if (img_msg) {
            int msg_id = msg->GetMsgId();
            _msg_id_to_img_info[msg_id] = img_msg->_msg_info; // 状态归 Controller 统一管理

            auto* bubble = item->GetBubble();
            auto* pic_bubble = dynamic_cast<PictureBubble*>(bubble);
            if (pic_bubble) {
                pic_bubble->setTransferState(TransferState::Uploading);

                // 绑定气泡点击事件到控制器
                connect(pic_bubble, &PictureBubble::sig_clicked, this, [this, msg_id]() {
                    qDebug() << "[图片交互] 用户点击了图片，msg_id=" << msg_id;
                    // 点击后在此处统一处理：放大原图预览 或 暂停/恢复切片传输
                });
            }
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
        qDebug() << "[发送拦截] 当前会话为空";
        return;
    }

    auto user_info = UserManager::GetInstance()->GetUserInfo();
    if (!user_info) return;

    auto pTextEdit = ui->chat_edit;
    // 获取输入框解析出来的图文混合消息列表
    const auto& msgList = pTextEdit->getMsgList();
    if (msgList.isEmpty()) return;

    QJsonObject textObj;
    QJsonArray textArray;
    int txt_size = 0;
    auto thread_id = _chat_data->GetThreadId();
    ChatRole role = ChatRole::Self;

    // 辅助 Lambda：立即将缓冲区中累积的纯文本打包发出并清空
    auto flushTextBatch = [&]() {
        if (textArray.isEmpty()) return;

        textObj["text_array"] = textArray;
        textObj["fromuid"] = user_info->_uid;
        textObj["touid"] = _chat_data->GetOtherId();
        textObj["thread_id"] = thread_id;

        QJsonDocument doc(textObj);
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_TEXT_CHAT_MSG_REQ, doc.toJson(QJsonDocument::Compact));

        // 清空状态
        txt_size = 0;
        textArray = QJsonArray();
        textObj = QJsonObject();
    };

    for (int i = 0; i < msgList.size(); ++i)
    {
        // 1. 过滤超长文本
        if (msgList[i]->_text_or_url.length() > 1024) continue;

        MsgType type = msgList[i]->_msg_type;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(user_info->_name);
        pChatItem->setUserIcon(user_info->_icon);

        QWidget *pBubble = nullptr;
        QString uuidString = QUuid::createUuid().toString(QUuid::WithoutBraces);

        // ================= 2. 文本消息分支 =================
        if (type == MsgType::TEXT_MSG)
        {
            pBubble = new TextBubble(role, msgList[i]->_text_or_url);

            // 超长分批保护：累加超过限制时先发一轮
            if (txt_size + msgList[i]->_text_or_url.length() > 1024) {
                flushTextBatch();
            }

            txt_size += msgList[i]->_text_or_url.length();
            QJsonObject obj;
            obj["content"] = msgList[i]->_text_or_url;
            obj["unique_id"] = uuidString;
            textArray.append(obj);

            auto txt_msg = std::make_shared<TextChatData>(
                uuidString, thread_id, ChatFormType::PRIVATE,
                ChatMsgType::TEXT, msgList[i]->_text_or_url, user_info->_uid, 0
                );
            _chat_data->AppendUnRspMsg(uuidString, txt_msg);
        }
        // ================= 3. 图片消息分支 =================
        else if (type == MsgType::IMG_MSG)
        {
            //核心保序：发送图片前，必须先把前面积累的文字刷出去！
            flushTextBatch();

            // 生成本地图片气泡展示
            pBubble = new PictureBubble(QPixmap(msgList[i]->_text_or_url), role);

            // 构造图片消息元数据
            auto img_msg = std::make_shared<ImgChatData>(
                msgList[i], uuidString, thread_id, ChatFormType::PRIVATE,
                ChatMsgType::PIC, user_info->_uid, 0
                );
            _chat_data->AppendUnRspMsg(uuidString, img_msg);

            // 组装发往 ChatServer 的图片通知信令
            QJsonObject imgObj;
            imgObj["fromuid"] = user_info->_uid;
            imgObj["touid"] = _chat_data->GetOtherId();
            imgObj["thread_id"] = thread_id;
            imgObj["md5"] = msgList[i]->_md5;
            imgObj["name"] = msgList[i]->_unique_name;
            imgObj["token"] = UserManager::GetInstance()->GetToken();
            imgObj["unique_id"] = uuidString;

            // 登记待传输文件，准备通过 ResourceServer 切片推送
            UserManager::GetInstance()->AddTransFile(msgList[i]->_unique_name, msgList[i]);

            QJsonDocument doc(imgObj);
            emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_IMG_CHAT_MSG_REQ, doc.toJson(QJsonDocument::Compact));
        }

        // ================= 4. 挂载气泡到右侧聊天视图 =================
        if (pBubble != nullptr) {
            pChatItem->setWidget(pBubble);
            pChatItem->setStatus(MessageStatus::UN_READ); // 初始为 0 (转圈发送中)
            ui->chat_data_list->appendChatItem(pChatItem);
            _unrsp_item_map[uuidString] = pChatItem;
        }
    }

    // 5. 循环结束后，把最后剩余的文本一次性全部刷出
    flushTextBatch();

    // 6. 清空输入框
    pTextEdit->clear();
}

// 当收到文件传输进度通知时
void ChatPage::UpdateFileProgress(std::shared_ptr<MsgInfo> msg_info)
{
    if (!msg_info) return;

    // 1. 查找气泡
    auto iter = _base_item_map.find(msg_info->_msg_id);
    if (iter == _base_item_map.end()) {
        return;
    }

    // 2. 根据类型驱动 UI
    if (msg_info->_msg_type == MsgType::IMG_MSG) {
        auto* bubble = iter.value()->GetBubble();
        auto* pic_bubble = dynamic_cast<PictureBubble*>(bubble);
        if (pic_bubble) {
            // 🌟 仅传递当前大小与总大小给纯 View
            pic_bubble->setProgress(msg_info->_current_size, msg_info->_total_size); // 教程里有的字段叫 _rsp_size，对齐你的 MsgInfo 字段名即可
        }
    }
}

void ChatPage::DownloadFileFinished(std::shared_ptr<MsgInfo> msg_info, QString file_path)
{
    if (!msg_info) return;

    auto iter = _base_item_map.find(msg_info->_msg_id);
    if (iter == _base_item_map.end()) {
        return;
    }

    if (msg_info->_msg_type == MsgType::IMG_MSG) {
        auto* bubble = iter.value()->GetBubble();
        auto* pic_bubble = dynamic_cast<PictureBubble*>(bubble);
        if (pic_bubble) {
            // 🌟 方案 B 纯 View 调用：直接替换为下载好的本地图片，并切换状态
            pic_bubble->setPixmap(QPixmap(file_path));
            pic_bubble->setTransferState(TransferState::Completed);
        }

        // 🌟 数据模型状态由 Controller 维护，不堆在 Bubble 里
        auto chat_data_base = _chat_data->GetChatDataBase(msg_info->_msg_id);
        auto img_data = std::dynamic_pointer_cast<ImgChatData>(chat_data_base);
        if (img_data && img_data->_msg_info) {
            img_data->_msg_info->_preview_pix = QPixmap(file_path);
            img_data->_msg_info->_current_size = img_data->_msg_info->_total_size;
        }
    }
}