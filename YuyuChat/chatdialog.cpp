#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "chatuserwid.h"
#include "loadingdialog.h"
#include <QRandomGenerator>
#include <QAction>
#include <QMouseEvent>
#include <QApplication>
ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog),_mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),_b_loading(false)
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

    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user,
            this, &ChatDialog::slot_loading_chat_user);


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

    // 🌟 核心修复：安装到 qApp，监听整个程序的鼠标点击，而不是只听自己的
    qApp->installEventFilter(this);

    // 设置聊天label选中状态
    ui->side_chat_label->SetSelected(true);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

std::vector<QString> names = {
    "llfc",
    "zack",
    "golang",
    "cpp",
    "java",
    "nodejs",
    "python",
    "rust"
};

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

    // 🌟🌟🌟 新增修复：检查被点击的控件是否在弹出的对话框（QDialog）内部
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
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
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


void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDialog *loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    addChatUserList();
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

