#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "chatuserwid.h"
#include "loadingdialog.h"
#include "tcpmanager.h"
#include "usermanager.h"
#include "searchlist.h"
#include <QRandomGenerator>
#include <QAction>
#include <QMouseEvent>
#include <QApplication>
ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog),_mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),_b_loading(false),_cur_chat_uid(0),_last_widget(nullptr),_cur_chat_thread_id(0)
{
    ui->setupUi(this);
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    // 开启原生的清除按钮
    ui->search_edit->setClearButtonEnabled(true);

    for (auto *btn : ui->search_edit->findChildren<QAbstractButton*>()) {
        btn->setCursor(Qt::PointingHandCursor);
    }

    // 字符长度限制
    ui->search_edit->setMaxLength(15);

    connect(ui->search_edit, &QLineEdit::textChanged, [this](const QString &text) {
        if (text.isEmpty()) {
            // 如果文本空了 (包括用户点击了原生的清除小叉号)，就隐藏搜索框，恢复聊天列表
            ShowSearch(false);
            ui->search_edit->clearFocus(); // 清除焦点，让光标消失
        } else {
            // 如果文本不为空，说明用户正在输入，立刻切换到搜索列表 UI
            ShowSearch(true);
        }

        // 每次文本改变时，遍历输入框内部的所有子部件（必然包含那个新冒出来的小叉号）
        // 因为 QLineEdit 本身的文本区不是子 Widget，所以能找到的 Widget 都是附加图标
        for (QWidget *w : ui->search_edit->findChildren<QWidget*>()) {
            w->setCursor(Qt::PointingHandCursor);
        }
    });

    // 为了让左侧的“搜索图标”一上来也是小手，在构造函数的 connect 后面补上这一刀：
    for (QWidget *w : ui->search_edit->findChildren<QWidget*>()) {
        w->setCursor(Qt::PointingHandCursor);
    }

    ShowSearch(false);

    QPixmap pixmap(":/res/head_1.jpg");
    ui->side_head_label->setPixmap(pixmap); // 将图片设置到QLabel上
    QPixmap scaledPixmap = pixmap.scaled( ui->side_head_label->size(), Qt::KeepAspectRatio); // 将图片缩放到label的大小
    ui->side_head_label->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
    ui->side_head_label->setScaledContents(true); // 设置QLabel自动缩放图片内容以适应大小

    ui->side_chat_label->setProperty("state","normal");

    ui->side_chat_label->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    ui->side_contact_label->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    AddLBGroup(ui->side_chat_label);
    AddLBGroup(ui->side_contact_label);

    connect(ui->side_chat_label, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_label, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

    // 安装到 qApp，监听整个程序的鼠标点击，而不是只听自己的
    qApp->installEventFilter(this);

    // 设置聊天label选中状态
    ui->side_chat_label->SetSelected(true);
    ui->search_list->SetSearchEdit(ui->search_edit);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);

    SetSelectChatItem();
    SetSelectChatPage();

    connect(TcpManager::GetInstance().get(),&TcpManager::sig_friend_apply,this,&ChatDialog::slot_apply_friend);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_add_auth_friend,this,&ChatDialog::slot_add_auth_friend);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_auth_rsp,this,&ChatDialog::slot_auth_rsp);
    connect(ui->search_list,&SearchList::sig_switch_chat_item,this,qOverload<std::shared_ptr<SearchInfo>>(&ChatDialog::slot_switch_chat_item));
    connect(ui->friend_info_page,&FriendInfoPage::sig_switch_chat_item,this,qOverload<std::shared_ptr<UserInfo>>(&ChatDialog::slot_switch_chat_item));
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user,this, &ChatDialog::slot_loading_chat_user);
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user,this, &ChatDialog::slot_loading_contact_user);
    connect(ui->con_user_list, &ContactUserList::sig_switch_friend_info_page,this, &ChatDialog::slot_switch_friend_info_page);
    connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page,this, &ChatDialog::slow_switch_apply_friend_page);
    connect(ui->chat_user_list,&QListWidget::itemClicked,this,&ChatDialog::slot_item_clicked);
    connect(ui->chat_page,&ChatPage::sig_append_send_chat_msg,this,&ChatDialog::slot_append_send_chat_msg);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_text_chat_msg,this,&ChatDialog::slot_text_chat_msg);
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_load_chat_thread, this, &ChatDialog::slot_load_chat_thread);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_create_private_chat,this,&ChatDialog::slot_create_private_chat);
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_load_chat_msg, this, &ChatDialog::slot_load_chat_msg);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    // 如果不处于搜索模式则直接返回
    if( _mode != ChatUIMode::SearchMode){
        return;
    }

    // 获取兼容 Qt 5 和 Qt 6 的全局坐标
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint globalPos = event->globalPosition().toPoint();
#else
    QPoint globalPos = event->globalPos();
#endif

    // 1. 判断点击位置是否在【搜索列表】的范围内
    QPoint posInSearchList = ui->search_list->mapFromGlobal(globalPos);
    bool isClickInSearchList = ui->search_list->rect().contains(posInSearchList);

    // 2. 判断点击位置是否在【搜索输入框】的范围内
    QPoint posInSearchEdit = ui->search_edit->mapFromGlobal(globalPos);
    bool isClickInSearchEdit = ui->search_edit->rect().contains(posInSearchEdit);
    // 获取当前鼠标实际点击到的那个具体的 widget
    QWidget* clickedWidget = qApp->widgetAt(globalPos);
    bool isClickInDialog = false;

    if (clickedWidget) {
        // 一层层往上找，看它是不是被包在一个 QDialog 里面
        QWidget* parent = clickedWidget;
        while (parent != nullptr) {
            if (qobject_cast<QDialog*>(parent)) {
                isClickInDialog = true;
                break;
            }
            parent = parent->parentWidget();
        }
    }

    // 3. 修改判断逻辑：只有当点击位置不在搜索列表，不在搜索框，且【不在任何弹出的对话框内】时，才关闭搜索
    if (!isClickInSearchList && !isClickInSearchEdit && !isClickInDialog) {
        ui->search_edit->clear();
        ShowSearch(false);
    }
}

void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto & ele: _lb_list){
        if(ele == lb){
            continue;
        }

        ele->ClearState();
    }
}

void ChatDialog::SetSelectChatItem(int thread_id)
{
    if (ui->chat_user_list->count() <= 0) return;

    // 默认选中第 0 项
    if (thread_id == 0) {
        ui->chat_user_list->setCurrentRow(0);
        auto* firstItem = ui->chat_user_list->item(0);
        if (!firstItem) return;
        auto* con_item = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(firstItem));
        if (con_item) {
            // 通过 thread_data 获取会话 thread_id 和对方 uid
            auto thread_data = con_item->GetChatInfo(); // 或者 con_item->GetUserInfo() 如果返回值是 ChatThreadData
            if (thread_data) {
                _cur_chat_thread_id = thread_data->GetThreadId();
                _cur_chat_uid = thread_data->GetOtherId();
            }
        }
        return;
    }

    // 指定 thread_id 选中
    auto iter = _chat_thread_items.find(thread_id);
    if (iter != _chat_thread_items.end()) {
        ui->chat_user_list->setCurrentItem(iter.value());
        _cur_chat_thread_id = thread_id;
    }
}

void ChatDialog::SetSelectChatPage(int thread_id)
{
    if (ui->chat_user_list->count() <= 0) return;

    // 默认选中第 0 项
    if (thread_id == 0) {
        auto item = ui->chat_user_list->item(0);
        if (!item) return;
        auto* con_item = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(item));
        if (con_item) {
            auto thread_data = con_item->GetChatInfo();
            if (thread_data) {
                // 🌟 通过 other_id 查出对应的 UserInfo 传给聊天界面
                auto user_info = UserManager::GetInstance()->GetFriendById(thread_data->GetOtherId());
                if (user_info) {
                    ui->chat_page->SetUserInfo(user_info);
                }
            }
        }
        return;
    }

    // 指定 thread_id 选中
    auto find_iter = _chat_thread_items.find(thread_id);
    if (find_iter == _chat_thread_items.end()) return;

    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) return;

    auto* con_item = qobject_cast<ChatUserWid*>(widget);
    if (con_item) {
        auto thread_data = con_item->GetChatInfo();
        if (thread_data) {
            auto user_info = UserManager::GetInstance()->GetFriendById(thread_data->GetOtherId());
            if (user_info) {
                ui->chat_page->SetUserInfo(user_info);
            }
        }
    }
}

void ChatDialog::slot_loading_chat_user()
{
    // 如果正在拉取，或者服务端已经标记没有更多会话了，直接返回
    if (_b_loading || !_b_chat_load_more) {
        return;
    }

    // 触发网络请求，拉取下一页真实的 chat_thread
    loadChatList();
}

void ChatDialog::slot_loading_contact_user()
{
    if(_b_loading){
        return;
    }


    if(UserManager::GetInstance()->IsLoadConFin()){
        return;
    }
    _b_loading = true;
    LoadingDialog *loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    loadMoreConUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
    qDebug()<< "receive side chat clicked";
    ClearLabelState(ui->side_chat_label);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_side_contact()
{
    qDebug()<< "receive side contact clicked";
    ClearLabelState(ui->side_contact_label);
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    if (!str.isEmpty()) {
        ShowSearch(true);
    }
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    bool b_already = UserManager::GetInstance()->AlreadyApply(apply->_from_uid);
    if(b_already){
        return;
    }

    UserManager::GetInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->side_contact_label->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info) {
    qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
             << " name is " << auth_info->_name << " nick is " << auth_info->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserManager::GetInstance()->CheckFriendById(auth_info->_uid);
    if (bfriend) return;

    UserManager::GetInstance()->AddFriend(auth_info);
    auto chat_thread_data = std::make_shared<ChatThreadData>(auth_info->_uid, auth_info->_thread_id, 0);
    for (auto& chat_msg : auth_info->_chat_datas) {
        chat_thread_data->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }
    UserManager::GetInstance()->AddChatThreadData(chat_thread_data, auth_info->_uid);

    //  组装 UserInfo 并通过辅助函数一键渲染到左侧列表顶部
    auto user_info = std::make_shared<UserInfo>(auth_info);
    AddChatUserItem(user_info, auth_info->_thread_id, true);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
             << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserManager::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if(bfriend){
        return;
    }

    UserManager::GetInstance()->AddFriend(auth_rsp);
    auto* chat_user_wid = new ChatUserWid();
    auto chat_thread_data = std::make_shared<ChatThreadData>(auth_rsp->_uid, auth_rsp->_thread_id, 0);
    UserManager::GetInstance()->AddChatThreadData(chat_thread_data, auth_rsp->_uid);
    for (auto& chat_msg : auth_rsp->_chat_datas) {
        chat_thread_data->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }
    chat_user_wid->SetChatInfo(chat_thread_data);

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_thread_items.insert(auth_rsp->_thread_id, item);
}

void ChatDialog::slot_switch_chat_item(std::shared_ptr<SearchInfo> si)
{
    auto thread_id = UserManager::GetInstance()->GetThreadIdByUid(si->_uid);
    if (thread_id != -1) {
        auto iter = _chat_thread_items.find(thread_id);
        if (iter != _chat_thread_items.end()) {
            ui->chat_user_list->scrollToItem(iter.value());
        } else {
            auto user_info = std::make_shared<UserInfo>(si);
            AddChatUserItem(user_info, thread_id, true);
        }
        JumpToChatSession(thread_id);
        return;
    }

    auto user_info = std::make_shared<UserInfo>(si);
    slot_switch_chat_item(user_info);
}

void ChatDialog::slot_switch_chat_item(std::shared_ptr<UserInfo> si)
{
    auto thread_id = UserManager::GetInstance()->GetThreadIdByUid(si->_uid);
    if (thread_id != -1) {
        auto iter = _chat_thread_items.find(thread_id);
        if (iter != _chat_thread_items.end()) {
            ui->chat_user_list->scrollToItem(iter.value());
        } else {
            AddChatUserItem(si, thread_id, true);
        }
        JumpToChatSession(thread_id);
        return;
    }

    auto uid = UserManager::GetInstance()->GetUid();
    QJsonObject jsonObj;
    jsonObj["uid"] = uid;
    jsonObj["other_id"] = si->_uid;
    QJsonDocument doc(jsonObj);

    emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_CREATE_PRIVATE_CHAT_REQ, doc.toJson(QJsonDocument::Compact));
}

void ChatDialog::slot_switch_friend_info_page(std::shared_ptr<UserInfo> user_info)
{
    _last_widget = ui->friend_info_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_info_page);
    ui->friend_info_page->SetInfo(user_info);
}

void ChatDialog::slow_switch_apply_friend_page()
{
    _last_widget = ui->friend_apply_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
}

void ChatDialog::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = ui->chat_user_list->itemWidget(item);
    if (!widget) return;

    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) return;

    if (customItem->GetItemType() == ListItemType::CHAT_USER_ITEM) {
        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        if (!chat_wid) return;

        auto thread_data = chat_wid->GetChatInfo(); // 获取该行绑定的 ChatThreadData
        if (!thread_data) return;

        auto friend_info = UserManager::GetInstance()->GetFriendById(thread_data->GetOtherId());
        if (friend_info) {
            ui->chat_page->SetUserInfo(friend_info);
            _cur_chat_uid = friend_info->_uid;
            _cur_chat_thread_id = thread_data->GetThreadId();
        }
    }
}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msg)
{
    if (!msg) return;

    int thread_id = (_cur_chat_thread_id != 0) ? _cur_chat_thread_id : msg->GetThreadId();

    // 存入本地内存模型
    std::vector<std::shared_ptr<TextChatData>> msg_vec;
    msg_vec.push_back(msg);
    UserManager::GetInstance()->AppendFriendChatMsg(_cur_chat_uid, msg_vec);

    ui->chat_page->AppendChatMsg(msg);

    auto iter = _chat_thread_items.find(thread_id);
    if (iter != _chat_thread_items.end()) {
        QWidget *widget = ui->chat_user_list->itemWidget(iter.value());
        auto chat_wid = qobject_cast<ChatUserWid*>(widget);
        if (chat_wid) {
            chat_wid->UpdateLastMsg(msg_vec);
        }
    }
}

void ChatDialog::slot_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> chat_msgs)
{
    for (auto& msg : chat_msgs) {

        //更新数据
        auto thread_id = msg->GetThreadId();
        auto thread_data = UserManager::GetInstance()->GetChatThreadByThreadId(thread_id);
        if (!thread_data) {
            continue; // 判空保护，防止未缓存的会话导致崩溃
        }
        thread_data->AddMsg(msg);

        auto iter = _chat_thread_items.find(thread_id);
        if (iter != _chat_thread_items.end()) {
            QWidget* widget = ui->chat_user_list->itemWidget(iter.value());
            auto chat_wid = qobject_cast<ChatUserWid*>(widget);
            if (chat_wid) {
                std::vector<std::shared_ptr<TextChatData>> single_vec = { msg };
                chat_wid->UpdateLastMsg(single_vec);
            }
        }

        if ((_cur_chat_thread_id != 0 && _cur_chat_thread_id == thread_id) ||
            (_cur_chat_uid != 0 && _cur_chat_uid == msg->GetSendUid())) {
            ui->chat_page->AppendChatMsg(msg);
        }

    }
}

void ChatDialog::showLoadingDlg(bool b_show) {
    if (b_show) {
        if (!_loading_dlg) {
            _loading_dlg = new LoadingDialog(this);
            _loading_dlg->setModal(true);
        }
        _loading_dlg->show();

        // 10秒后如果还没关，强制关闭防止死锁
        QTimer::singleShot(10000, this, [this]() {
            if (_loading_dlg && _loading_dlg->isVisible()) {
                showLoadingDlg(false);
                qDebug() << "[UI安全] 拉取会话列表超时，已强制关闭 Loading 框";
            }
        });
    } else {
        _b_loading = false;
        if (_loading_dlg) {
            _loading_dlg->hide();
            _loading_dlg->deleteLater();
            _loading_dlg = nullptr;
        }
    }
}

void ChatDialog::loadChatList() {
    _b_loading = true;
    showLoadingDlg(true);

    QJsonObject jsonObj;
    auto uid = UserManager::GetInstance()->GetUid();
    jsonObj["uid"] = uid;
    // 从本地记录的最后一个 id 开始拉，初始为 0
    jsonObj["last_id"] = _next_last_thread_id;
    jsonObj["page_size"] = 20;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 注意替换为你实际定义的 ReqId
    emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_LOAD_CHAT_THREAD_REQ, jsonData);
}

void ChatDialog::loadChatMsg()
{
    _cur_load_chat = UserManager::GetInstance()->GetCurLoadThreadData();

    if (_cur_load_chat == nullptr) {
        return;
    }

    showLoadingDlg(true);

    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_LOAD_CHAT_MSG_REQ, jsonData);
}

void ChatDialog::slot_load_chat_thread(bool load_more, qint64 last_thread_id, std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads)
{
    int first_thread_id = 0;

    for (auto& cti : chat_threads) {
        if (cti->_type == "group") continue;

        qint64 my_uid = UserManager::GetInstance()->GetUid();
        qint64 other_uid = (my_uid == cti->_user1_id) ? cti->_user2_id : cti->_user1_id;

        auto friend_info = UserManager::GetInstance()->GetFriendById(other_uid);
        if (!friend_info) continue;

        // 1. 同步到本地数据模型
        auto chat_thread_data = std::make_shared<ChatThreadData>(other_uid, cti->_thread_id, 0);
        UserManager::GetInstance()->AddChatThreadData(chat_thread_data, other_uid);

        // 2. 去重防重复添加
        if (_chat_thread_items.contains(cti->_thread_id)) continue;

        // 3. 复用封装函数添加 UI
        AddChatUserItem(friend_info, cti->_thread_id, false);

        if (first_thread_id == 0) {
            first_thread_id = cti->_thread_id;
        }
    }

    UserManager::GetInstance()->SetLastChatThreadId(last_thread_id);
    _next_last_thread_id = last_thread_id;
    _b_chat_load_more = load_more;

    // 递归拉取下一页
    if (load_more) {
        QJsonObject jsonObj;
        auto uid = UserManager::GetInstance()->GetUid();
        jsonObj["uid"] = uid;
        jsonObj["last_id"] = _next_last_thread_id; // 对齐服务端字段名
        jsonObj["page_size"] = 20;

        QJsonDocument doc(jsonObj);
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_LOAD_CHAT_THREAD_REQ, doc.toJson(QJsonDocument::Compact));
        return;
    }

    // 全部会话拉取完毕
    _b_loading = false;
    showLoadingDlg(false);

    if (first_thread_id != 0) {
        SetSelectChatItem(first_thread_id);
        SetSelectChatPage(first_thread_id);
    }

}

void ChatDialog::slot_create_private_chat(int uid, int other_id, int thread_id)
{
    auto user_info = UserManager::GetInstance()->GetFriendById(other_id);
    if (!user_info) return;

    // 1. 插入顶部
    AddChatUserItem(user_info, thread_id, true);

    // 2. 更新本地数据模型
    auto chat_thread_data = std::make_shared<ChatThreadData>(other_id, thread_id, 0);
    UserManager::GetInstance()->AddChatThreadData(chat_thread_data, other_id);

    // 3. 选中并激活会话
    JumpToChatSession(thread_id);
}

void ChatDialog::slot_load_chat_msg(int thread_id, int msg_id, bool load_more, std::vector<std::shared_ptr<TextChatData> > msglists)
{
    _cur_load_chat->SetLastMsgId(msg_id);

    for(auto& msg: msglists){
        _cur_load_chat->AppendMsg(msg->GetMsgId(),msg);
    }

    if (load_more) {
        //发送请求给服务器
        //发送请求逻辑
        QJsonObject jsonObj;
        jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
        jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_LOAD_CHAT_MSG_REQ, jsonData);
        return;
    }

    //获取下一个chat_thread
    _cur_load_chat = UserManager::GetInstance()->GetNextLoadThreadData();
    //都加载完了
    if(!_cur_load_chat){
        //更新聊天界面信息
        SetSelectChatItem();
        SetSelectChatPage();
        showLoadingDlg(false);
        return;
    }

    //继续加载下一个聊天
    //发送请求给服务器
    //发送请求逻辑
    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_LOAD_CHAT_MSG_REQ, jsonData);
    return;

}

void ChatDialog::loadMoreConUser()
{
    auto friend_list = UserManager::GetInstance()->GetConListPerPage();
    if (!friend_list.empty())
    {
        for (auto& friend_ele : friend_list)
        {
            auto* con_user_item = new ConUserItem();
            con_user_item->SetInfo(friend_ele->_uid, friend_ele->_name, friend_ele->_icon);

            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(con_user_item->sizeHint());

            ui->con_user_list->addItem(item);
            ui->con_user_list->setItemWidget(item, con_user_item);
        }
        UserManager::GetInstance()->UpdateContactLoadedCount();
    }
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgs)
{
    for(auto & msg : msgs){
        if(msg->GetSendUid() != _cur_chat_uid){
            continue;
        }
        ui->chat_page->AppendChatMsg(msg);
    }
}

void ChatDialog::JumpToChatSession(int thread_id)
{
    ui->side_chat_label->SetSelected(true);
    SetSelectChatItem(thread_id);
    SetSelectChatPage(thread_id);
    slot_side_chat();
}

QListWidgetItem* ChatDialog::AddChatUserItem(std::shared_ptr<UserInfo> user_info, int thread_id, bool insert_top)
{
    if (!user_info) return nullptr;

    auto* chat_user_wid = new ChatUserWid();
    auto chat_thread_data = UserManager::GetInstance()->GetChatThreadByThreadId(thread_id);
    if (!chat_thread_data) {
        chat_thread_data = std::make_shared<ChatThreadData>(user_info->_uid, thread_id, 0);
        UserManager::GetInstance()->AddChatThreadData(chat_thread_data, user_info->_uid);
    }
    chat_user_wid->SetChatInfo(chat_thread_data);

    auto* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());

    if (insert_top) {
        ui->chat_user_list->insertItem(0, item);
    } else {
        ui->chat_user_list->addItem(item);
    }
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    // 统一使用全局唯一的 thread_id 作为 Key
    _chat_thread_items.insert(thread_id, item);
    return item;
}