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
    _state(ChatUIMode::ChatMode),_b_loading(false),_cur_chat_uid(0),_last_widget(nullptr)
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

        // 🌟 终极杀招：每次文本改变时，遍历输入框内部的所有子部件（必然包含那个新冒出来的小叉号）
        // 因为 QLineEdit 本身的文本区不是子 Widget，所以能找到的 Widget 都是附加图标
        for (QWidget *w : ui->search_edit->findChildren<QWidget*>()) {
            w->setCursor(Qt::PointingHandCursor);
        }
    });

    // 🌟 为了让左侧的“搜索图标”一上来也是小手，在构造函数的 connect 后面补上这一刀：
    for (QWidget *w : ui->search_edit->findChildren<QWidget*>()) {
        w->setCursor(Qt::PointingHandCursor);
    }

    ShowSearch(false);
    addChatUserList();

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
    connect(ui->con_user_list,&QListWidget::itemClicked,this,&ChatDialog::slot_item_clicked);
    connect(ui->chat_page,&ChatPage::sig_append_send_chat_msg,this,&ChatDialog::slot_append_send_chat_msg);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_text_chat_msg,this,&ChatDialog::slot_text_chat_msg);
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_load_chat_thread, this, &ChatDialog::slot_load_chat_thread);
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

void ChatDialog::addChatUserList()
{
    auto friend_list = UserManager::GetInstance()->GetChatListPerPage();
    // 范围for循环：遍历当前页每一个好友信息
    if(!friend_list.empty()){
        for (auto& friend_ele : friend_list)
        {
            // _chat_items_added：哈希容器，存 <好友UID, 列表条目item>，作用：记录哪些好友已经被加到列表里了
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            // 如果能查到这个UID，说明该好友条目早已创建，跳过本次循环，防止重复添加
            if (find_iter != _chat_items_added.end())
            {
                continue;
            }

            // 2. 新建自定义好友条目Qt控件（头像、昵称、签名那一整行UI）
            auto* chat_user_wid = new ChatUserWid();
            // 把当前好友信息打包，传给自定义控件展示
            chat_user_wid->SetInfo(friend_ele);

            // 3. 创建Qt列表基础条目外壳，用来承载我们自定义的控件
            QListWidgetItem* item = new QListWidgetItem;
            // 设置条目固定高度，适配自定义控件大小
            item->setSizeHint(chat_user_wid->sizeHint());

            // 4. 把条目插入到侧边聊天列表中
            ui->chat_user_list->addItem(item);
            // 将自定义控件塞进列表条目内部显示
            ui->chat_user_list->setItemWidget(item, chat_user_wid);

            // 5. 将当前好友UID和对应条目存入哈希表，标记：该好友已添加至列表，下次分页不再重复创建
            _chat_items_added.insert(friend_ele->_uid, item);
        }
        // 6.分页加载计数 + 一页条数，更新已加载总数，下一次分页截取区间会后移
        UserManager::GetInstance()->UpdateChatLoadedCount();
    }

    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid();
        auto user_info = std::make_shared<UserInfo>(0,names[name_i],names[name_i],heads[head_i],0,strs[str_i]);
        chat_user_wid->SetInfo(user_info);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
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

void ChatDialog::SetSelectChatItem(int uid)
{
    if(ui->chat_user_list->count()<= 0){
        return;
    }
    if(uid == 0){
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem* firstItem = ui->chat_user_list->item(0);
        if(!firstItem){
            return;
        }

        QWidget* wid = ui->chat_user_list->itemWidget(firstItem);
        if(!wid){
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(wid);
        if (!con_item)
        {
            return;
        }

        _cur_chat_uid = con_item->GetUserInfo()->_uid;
        return;
    }
}

void ChatDialog::SetSelectChatPage(int uid)
{
    if(ui->chat_user_list->count()<= 0){
        return;
    }
    if(uid == 0){
        auto item = ui->chat_user_list->item(0);
        QWidget* wid = ui->chat_user_list->itemWidget(item);
        if(!wid){
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(wid);
        if(!con_item){
            return;
        }

        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);
        return;
    }

    auto find_iter = _chat_items_added.find(uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    //转为widget
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);

        return;
    }
}


void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }


    if(UserManager::GetInstance()->IsLoadChatFin()){
        return;
    }
    _b_loading = true;
    LoadingDialog *loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    loadMoreChatUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
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
    if(bfriend){
        return;
    }

    UserManager::GetInstance()->AddFriend(auth_info);

    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_info->_uid, item);
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
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_rsp->_uid, item);
}

void ChatDialog::slot_switch_chat_item(std::shared_ptr<SearchInfo> si)
{
    auto iter = _chat_items_added.find(si->_uid);
    if(iter != _chat_items_added.end()){
        ui->chat_user_list->scrollToItem(iter.value());
        ui->side_chat_label->SetSelected(true);
        SetSelectChatItem(si->_uid);
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(si->_uid,item);

    SetSelectChatItem(si->_uid);
    SetSelectChatPage(si->_uid);
    slot_side_chat();
    return;
}

void ChatDialog::slot_switch_chat_item(std::shared_ptr<UserInfo> si)
{
    auto iter = _chat_items_added.find(si->_uid);
    if(iter != _chat_items_added.end()){
        ui->chat_user_list->scrollToItem(iter.value());
        ui->side_chat_label->SetSelected(true);
        SetSelectChatItem(si->_uid);
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(si);
    chat_user_wid->SetInfo(si);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(si->_uid,item);

    SetSelectChatItem(si->_uid);
    SetSelectChatPage(si->_uid);
    slot_side_chat();
    return;
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
    QWidget *widget = ui->chat_user_list->itemWidget(item); // 获取自定义widget对象
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM
        || itemType == ListItemType::GROUP_TIP_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    if(itemType == ListItemType::CHAT_USER_ITEM){

        // 创建对话框，提示用户
        qDebug()<< "chat user item clicked ";
        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        auto user_info = chat_wid->GetUserInfo();

        ui->chat_page->SetUserInfo(user_info);

        return;
    }


}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msg)
{
    if(_cur_chat_uid == 0){
        return;
    }

    auto iter = _chat_items_added.find(_cur_chat_uid);
    if(iter == _chat_items_added.end()){
        return;
    }

    QWidget *widget = ui->chat_user_list->itemWidget(iter.value()); // 获取自定义widget对象
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM
        || itemType == ListItemType::GROUP_TIP_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    if(itemType == ListItemType::CHAT_USER_ITEM){

        // 创建对话框，提示用户
        qDebug()<< "chat user item clicked ";
        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        if(!chat_wid){
            return;
        }
        auto user_info = chat_wid->GetUserInfo();
        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msg);
        UserManager::GetInstance()->AppendFriendChatMsg(_cur_chat_uid,msg_vec);
        return;
    }
}

void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    auto iter = _chat_items_added.find(msg->_from_uid);
    if(iter != _chat_items_added.end()){
        QWidget *widget = ui->chat_user_list->itemWidget(iter.value());
        auto chat_user_wid = qobject_cast<ChatUserWid*>(widget);
        if(!chat_user_wid){
            return;
        }
        chat_user_wid->UpdateLastMsg(msg->_chat_msgs);
        UpdateChatMsg(msg->_chat_msgs);
        UserManager::GetInstance()->AppendFriendChatMsg(msg->_from_uid,msg->_chat_msgs);
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    auto friend_info = UserManager::GetInstance()->GetFriendById(msg->_from_uid);
    chat_user_wid->SetInfo(friend_info); // 填充好友数据

    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint()); // 设置每行高度
    chat_user_wid->UpdateLastMsg(msg->_chat_msgs);
    UserManager::GetInstance()->AppendFriendChatMsg(msg->_from_uid,msg->_chat_msgs);
    ui->chat_user_list->addItem(item); // 把行加入侧边列表
    ui->chat_user_list->setItemWidget(item, chat_user_wid); // 塞入自定义UI
    _chat_items_added.insert(msg->_from_uid,item);
}

void ChatDialog::showLoadingDlg(bool b_show) {
    if (b_show) {
        if (!_loading_dlg) {
            _loading_dlg = new LoadingDialog(this);
            _loading_dlg->setModal(true);
        }
        _loading_dlg->show();

        // 【核心优化 1：悬空 Loading 兜底】10秒后如果还没关，强制关闭防止死锁
        QTimer::singleShot(10000, this, [this]() {
            if (_loading_dlg && _loading_dlg->isVisible()) {
                showLoadingDlg(false);
                qDebug() << "[UI安全] 拉取会话列表超时，已强制关闭 Loading 框";
            }
        });
    } else {
        if (_loading_dlg) {
            _loading_dlg->hide();
            _loading_dlg->deleteLater();
            _loading_dlg = nullptr;
        }
    }
}

// 3. 发送拉取请求
void ChatDialog::loadChatList() {
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

// 4. 处理回包
void ChatDialog::slot_load_chat_thread(bool load_more, qint64 last_thread_id, std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads) {
    for (auto& cti : chat_threads) {
        if (cti->_type == "group") {
            continue; // 群聊暂不处理
        }

        qint64 my_uid = UserManager::GetInstance()->GetUid();
        qint64 other_uid = (my_uid == cti->_user1_id) ? cti->_user2_id : cti->_user1_id;

        auto friend_info = UserManager::GetInstance()->GetFriendById(other_uid);
        if (!friend_info) continue;

        // 避免重复添加
        if (_chat_items_added.contains(other_uid)) continue;

        auto* chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(friend_info);
        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(chat_user_wid->sizeHint());

        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
        _chat_items_added.insert(other_uid, item);
    }

    // 【核心优化 2：杜绝 UI 风暴】只更新状态，不再递归触发 emit 发送网络请求
    _b_chat_load_more = load_more;
    _next_last_thread_id = last_thread_id;

    SetSelectChatItem();
    SetSelectChatPage();
    showLoadingDlg(false);
}

void ChatDialog::loadMoreChatUser()
{
    // 1. 调用用户管理器，获取【下一页】分页聊天好友数据
    auto friend_list = UserManager::GetInstance()->GetChatListPerPage();

    // 如果拿到的这一页数据不为空，才去创建UI条目
    if (friend_list.empty() == false)
    {
        // 遍历当前页每一个好友会话
        for (auto& friend_ele : friend_list)
        {
            // _chat_items_added：哈希表，记录已经添加到列表的好友UID，用来去重
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            // 该好友已经渲染过了，跳过，防止重复添加同一行
            if (find_iter != _chat_items_added.end())
            {
                continue;
            }

            // 2. 创建自定义行控件（展示头像、昵称、最新消息）
            auto* chat_user_wid = new ChatUserWid();
            chat_user_wid->SetInfo(friend_ele); // 填充好友数据

            // 3. Qt列表标准结构：QListWidgetItem 承载自定义Widget
            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint()); // 设置每行高度
            ui->chat_user_list->addItem(item); // 把行加入侧边列表
            ui->chat_user_list->setItemWidget(item, chat_user_wid); // 塞入自定义UI

            // 4. 标记：这个好友已经添加到列表，下次不再重复创建
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        // 5. 关键：更新已加载条数，_chat_loaded += 每页数量
        // 下次再滚动加载，就会截取往后一段的数据
        UserManager::GetInstance()->UpdateChatLoadedCount();
    }
}

void ChatDialog::loadMoreConUser()
{
    // 1. 调用用户管理器，获取【下一页】分页聊天好友数据
    auto friend_list = UserManager::GetInstance()->GetConListPerPage();

    // 如果拿到的这一页数据不为空，才去创建UI条目
    if (friend_list.empty() == false)
    {
        // 遍历当前页每一个好友会话
        for (auto& friend_ele : friend_list)
        {
            // _chat_items_added：哈希表，记录已经添加到列表的好友UID，用来去重
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            // 该好友已经渲染过了，跳过，防止重复添加同一行
            if (find_iter != _chat_items_added.end())
            {
                continue;
            }

            // 2. 创建自定义行控件（展示头像、昵称、最新消息）
            auto* chat_user_wid = new ConUserItem();
            chat_user_wid->SetInfo(friend_ele->_uid,friend_ele->_name,friend_ele->_icon); // 填充好友数据

            // 3. Qt列表标准结构：QListWidgetItem 承载自定义Widget
            QListWidgetItem* item = new QListWidgetItem;
            item->setSizeHint(chat_user_wid->sizeHint()); // 设置每行高度
            ui->chat_user_list->addItem(item); // 把行加入侧边列表
            ui->chat_user_list->setItemWidget(item, chat_user_wid); // 塞入自定义UI

            // 4. 标记：这个好友已经添加到列表，下次不再重复创建
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        // 5. 关键：更新已加载条数，_chat_loaded += 每页数量
        // 下次再滚动加载，就会截取往后一段的数据
        UserManager::GetInstance()->UpdateContactLoadedCount();
    }
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgs)
{
    for(auto & msg : msgs){
        if(msg->_from_uid != _cur_chat_uid){
            continue;
        }
        ui->chat_page->AppendChatMsg(msg);
    }
}
