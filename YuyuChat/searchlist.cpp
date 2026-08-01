#include "searchlist.h"
#include "tcpmanager.h"
#include "customizeedit.h"
#include "loadingdialog.h"
#include "adduseritem.h"
#include "findsuccessdialog.h"
#include "findfaildialog.h"
#include "usermanager.h"
#include <QScrollBar>
#include <QJsonDocument>

SearchList::SearchList(QWidget *parent):QListWidget(parent),_find_dlg(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->viewport()->installEventFilter(this);
    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    //添加条目
    addTipItem();
    //连接搜索条目
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if(_find_dlg){
        _find_dlg->hide();
        _find_dlg=nullptr;
    }
}

void SearchList::SetSearchEdit(QWidget *edit)
{
    _search_edit = edit;
}

bool SearchList::eventFilter(QObject *watched, QEvent *event) {
    // 检查事件是否是鼠标悬浮进入或离开 (这部分保留)
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    // 🌟 注意：把下面这段检查 QEvent::Wheel 的恶魔代码统统删掉！
    // if (watched == this->viewport() && event->type() == QEvent::Wheel) { ... }

    return QListWidget::eventFilter(watched, event);
}

void SearchList::waitPending(bool pending)
{
    if(pending){
        _loadingDialog = new LoadingDialog(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
    }
    else{
        if(_loadingDialog){
            _loadingDialog->hide();
            _loadingDialog->deleteLater();
            _loadingDialog = nullptr;
        }
    }
    _send_pending = pending;
}

void SearchList::addTipItem()
{
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);
    if (_find_dlg != nullptr) {
        _find_dlg->deleteLater();
        _find_dlg = nullptr;
    }

    if(si == nullptr){
        _find_dlg = new FindFailDialog(this);
    }
    else{
        auto self_uid = UserManager::GetInstance()->GetUid();
        if(si->_uid == self_uid){
            return;
        }

        bool exist = UserManager::GetInstance()->CheckFriendById(si->_uid);
        if(exist){
            emit sig_switch_chat_item(si);
            return;
        }
        _find_dlg = new FindSuccessDialog(this);
        FindSuccessDialog* success_dlg = dynamic_cast<FindSuccessDialog*>(_find_dlg);
        if (success_dlg) {
            success_dlg->SetSearchInfo(si);
        }
    }
    _find_dlg->show();
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); //获取自定义widget对象
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
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    if(itemType == ListItemType::ADD_USER_TIP_ITEM){
        if (_send_pending || !_search_edit){
            return;
        }

        waitPending(true);
        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto user_str = search_edit->text();
        QJsonObject jsonObj;
        jsonObj["uid"] = user_str;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_SEARCH_USER_REQ,jsonData);

        return;
    }

    //清除弹出框
    CloseFindDlg();

}