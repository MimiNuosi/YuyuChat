#include "applyfriend.h"
#include "ui_applyfriend.h"
#include "usermanager.h"
#include "tcpmanager.h"
#include <QScrollBar>
#define MIN_APPLY_LABEL_ED_LEN 50

ApplyFriend::ApplyFriend(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ApplyFriend),_label_point(2,6)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("ApplyFriend");
    this->setModal(true);

    ui->name_edit->setPlaceholderText(tr("恋恋风辰"));
    ui->label_edit->setPlaceholderText("搜索、添加标签"); // 注意确认你的 UI 里叫 label_edit 还是 lb_ed
    ui->back_edit->setPlaceholderText("燃烧的胸毛");

    ui->label_edit->SetMaxLength(21); // 如果你用了自定义 Edit 就保留，原生 QLineEdit 用 setMaxLength

    ui->input_tip_wid->hide();
    _tip_cur_point = QPoint(5, 5);
    _tip_data = { "同学","家人","菜鸟教程","C++ Primer","Rust 程序设计",
                 "父与子学Python","nodejs开发指南","go 语言开发指南",
                 "游戏伙伴","金融投资","微信读书","拼多多拼友" };

    // 1. 初始化下方推荐标签墙
    InitTipLbs();

    // 2. 动态输入提示逻辑 (完美替代原教程的 3 个槽函数)
    connect(ui->label_edit, &QLineEdit::textChanged, this, [=](const QString &text) {
        if (text.isEmpty()) {
            ui->input_tip_wid->hide();
        } else {
            ui->pushButton->setText("点击添加标签：" + text); // 假设你在 UI 里没改名叫 btn_input_tip
            ui->input_tip_wid->show();
        }
    });

    // 3. 点击下拉提示按钮，添加标签
    connect(ui->pushButton, &QPushButton::clicked, this, [=]() {
        QString text = ui->label_edit->text();
        addLabel(text); // 调用添加标签的函数
        ui->label_edit->clear(); // 清空输入框，提示框会自动隐藏
    });

    // 4. 回车直接添加标签
    connect(ui->label_edit, &QLineEdit::returnPressed, this, [=]() {
        QString text = ui->label_edit->text();
        if(!text.isEmpty()){
            addLabel(text);
            ui->label_edit->clear();
        }
    });

    // 隐藏滚动条并安装事件过滤器
    ui->scrollArea->horizontalScrollBar()->setHidden(true);
    ui->scrollArea->verticalScrollBar()->setHidden(true);
    ui->scrollArea->installEventFilter(this);

    // 按钮连接
    connect(ui->cancel_button, &QPushButton::clicked, this, &ApplyFriend::SlotApplyCancel);
    connect(ui->sure_button, &QPushButton::clicked, this, &ApplyFriend::SlotApplySure);
}

ApplyFriend::~ApplyFriend()
{
    qDebug()<< "ApplyFriend destruct";
    delete ui;
}

void ApplyFriend::InitTipLbs()
{
    int lines = 1;
    for(int i = 0; i < _tip_data.size(); i++){
        // 🌟 换成原生 QPushButton
        auto* lb = new QPushButton(ui->label_list);
        lb->setObjectName("tipslb");
        lb->setText(_tip_data[i]);
        lb->setCursor(Qt::PointingHandCursor);

        // 🌟 使用 Lambda 替代原本的 SlotChangeFriendLabelByTip
        connect(lb, &QPushButton::clicked, this, [=]() {
            // 获取当前按钮是否处于“选中”状态
            bool isSelected = lb->property("selected").toBool();
            if (isSelected) {
                // 如果已经是选中状态，再次点击就是取消
                lb->setProperty("selected", false);
                SlotRemoveFriendLabel(lb->text()); // 移除上面的已选标签
            } else {
                // 如果未选中，点击即为选中
                lb->setProperty("selected", true);
                addLabel(lb->text()); // 添加到上面的已选列表
            }
            // 刷新 QSS 样式
            lb->style()->unpolish(lb);
            lb->style()->polish(lb);
        });

        // 计算排版位置 (保留教程的排版算法)
        QFontMetrics fontMetrics(lb->font());
        int textWidth = fontMetrics.horizontalAdvance(lb->text()); // Qt6 推荐用 horizontalAdvance，Qt5 用 width()
        int textHeight = fontMetrics.height();

        if (_tip_cur_point.x() + textWidth + 15 > ui->label_list->width()) { // tip_offset 用 15 代替
            lines++;
            if (lines > 2) {
                delete lb;
                return;
            }
            _tip_cur_point.setX(15);
            _tip_cur_point.setY(_tip_cur_point.y() + textHeight + 15);
        }

        auto next_point = _tip_cur_point;
        AddTipLbs(lb, _tip_cur_point, next_point, textWidth, textHeight);
        _tip_cur_point = next_point;
    }
}

void ApplyFriend::AddTipLbs(QPushButton* lb, QPoint cur_point, QPoint& next_point, int text_width, int text_height)
{
    lb->move(cur_point);
    lb->show();
    _add_labels.insert(lb->text(), lb);
    _add_label_keys.push_back(lb->text());
    next_point.setX(lb->pos().x() + text_width + 15);
    next_point.setY(lb->pos().y());
}

bool ApplyFriend::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->scrollArea && event->type() == QEvent::Enter)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(false);
    }
    else if (obj == ui->scrollArea && event->type() == QEvent::Leave)
    {
        ui->scrollArea->verticalScrollBar()->setHidden(true);
    }
    return QObject::eventFilter(obj, event);
}

void ApplyFriend::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    _si = si;
    auto applyname = UserManager::GetInstance()->GetName();
    auto bakname = si->_name;
    ui->name_edit->setText(applyname);
    ui->back_edit->setText(bakname);
}

void ApplyFriend::resetLabels()
{
    auto max_width = ui->grid_wid->width();
    auto label_height = 0;
    for(auto iter = _friend_labels.begin(); iter != _friend_labels.end(); iter++){
        //todo... 添加宽度统计
        if( _label_point.x() + iter.value()->width() > max_width) {
            _label_point.setY(_label_point.y()+iter.value()->height()+6);
            _label_point.setX(2);
        }

        iter.value()->move(_label_point);
        iter.value()->show();

        _label_point.setX(_label_point.x()+iter.value()->width()+2);
        _label_point.setY(_label_point.y());
        label_height = iter.value()->height();
    }

    if(_friend_labels.isEmpty()){
        ui->label_edit->move(_label_point);
        return;
    }

    if(_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->grid_wid->width()){
        ui->label_edit->move(2,_label_point.y()+label_height+6);
    }else{
        ui->label_edit->move(_label_point);
    }
}

void ApplyFriend::addLabel(QString name)
{
    // 1. 使用纯净的 name 进行查重，完美防止重复添加
    if (_friend_labels.find(name) != _friend_labels.end()) {
        return;
    }

    auto tmplabel = new QPushButton(ui->grid_wid);

    // 2. 视觉上加上叉号，但逻辑里不用它
    tmplabel->setText(name + "  ✕");
    tmplabel->setObjectName("FriendLabel");
    tmplabel->setCursor(Qt::PointingHandCursor);

    // 🌟 修复重叠 Bug：强制控件根据文字内容重新计算自己的真实宽高
    tmplabel->adjustSize();

    auto max_width = ui->grid_wid->width();

    // 宽度统计与排版 (现在获取到的 tmplabel->width() 是准确的了)
    if (_label_point.x() + tmplabel->width() > max_width) {
        _label_point.setY(_label_point.y() + tmplabel->height() + 6);
        _label_point.setX(2);
    }

    tmplabel->move(_label_point);
    tmplabel->show();

    // 3. 🌟 核心修复：Map 的 Key 严格使用纯净的 name！
    _friend_labels[name] = tmplabel;
    _friend_label_keys.push_back(name);

    // 4. 🌟 核心修复：Lambda 表达式捕获纯净的 name 传给删除槽函数
    connect(tmplabel, &QPushButton::clicked, this, [=]() {
        SlotRemoveFriendLabel(name);
    });

    _label_point.setX(_label_point.x() + tmplabel->width() + 2);

    if (_label_point.x() + MIN_APPLY_LABEL_ED_LEN > ui->grid_wid->width()) {
        ui->label_edit->move(2, _label_point.y() + tmplabel->height() + 2);
    }
    else {
        ui->label_edit->move(_label_point);
    }

    ui->label_edit->clear();

    if (ui->grid_wid->height() < _label_point.y() + tmplabel->height() + 2) {
        ui->grid_wid->setFixedHeight(_label_point.y() + tmplabel->height() * 2 + 2);
    }
}

void ApplyFriend::SlotRemoveFriendLabel(QString name)
{
    qDebug() << "执行删除标签逻辑:" << name;

    // 每次重新排版时，将坐标重置到起始位置
    _label_point.setX(2);
    _label_point.setY(6);

    // 1. 从已选标签的 Map 中寻找
    auto find_iter = _friend_labels.find(name);
    if(find_iter == _friend_labels.end()){
        return;
    }

    // 2. 从记录顺序的 Vector 中移除
    auto find_key = std::find(_friend_label_keys.begin(), _friend_label_keys.end(), name);
    if(find_key != _friend_label_keys.end()){
        _friend_label_keys.erase(find_key);
    }

    // 3. 释放原生 QPushButton 的内存，并从 Map 中擦除
    delete find_iter.value();
    _friend_labels.erase(find_iter);

    // 4. 调用重排函数，把剩下的标签重新排列紧凑
    resetLabels();

    // 5. 联动底部推荐标签墙：如果被删除的标签在下方推荐墙里，将其恢复为未选中状态(灰色)
    auto find_add = _add_labels.find(name);
    if(find_add != _add_labels.end()){
        find_add.value()->setProperty("selected", false);
        // 刷新 QSS 样式使其变回灰色
        find_add.value()->style()->unpolish(find_add.value());
        find_add.value()->style()->polish(find_add.value());
    }
}

void ApplyFriend::SlotApplyCancel(){
    qDebug() << "取消添加好友";
    this->hide();
    deleteLater();
}

void ApplyFriend::SlotApplySure(){
    qDebug()<<"确认添加好友";
    QJsonObject jsonObj;
    auto uid = UserManager::GetInstance()->GetUid();
    jsonObj["uid"] = uid;

    auto name = ui->name_edit->text();
    if(name.isEmpty()){
        name = ui->name_edit->placeholderText();
    }
    jsonObj["applyname"] = name;

    auto back_name = ui->back_edit->text();
    if(back_name.isEmpty()){
        back_name = ui->back_edit->placeholderText();
    }
    jsonObj["backname"] = back_name;
    jsonObj["touid"] = _si->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_ADD_FRIEND_REQ,jsonData);

    this->hide();
    deleteLater();
}