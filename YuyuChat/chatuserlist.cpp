#include "chatuserlist.h"

ChatUserList::ChatUserList(QWidget *parent):QListWidget(parent)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 安装事件过滤器，用于处理鼠标悬浮显示滚动条
    this->viewport()->installEventFilter(this);

    // 监听滚动条数值变化，触底触发加载更多
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        // 如果当前滚动值等于最大值，说明到底了
        if (value == this->verticalScrollBar()->maximum()) {
            qDebug() << "load more chat user";
            emit sig_loading_chat_user();
        }
    });
}

bool ChatUserList::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    return QListWidget::eventFilter(watched, event);
}