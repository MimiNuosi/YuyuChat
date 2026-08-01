#include "chatview.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QEvent>
#include <QStyleOption>
#include <QPainter>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent), isAppended(false)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(0, 0, 0, 0); // 修复废弃 API

    m_pScrollArea = new QScrollArea(this);
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);

    QVBoxLayout *pVLayout_1 = new QVBoxLayout(w);
    // 🌟 修复毒点 1：使用真正的弹簧替代 new QWidget
    pVLayout_1->addStretch(1);

    m_pScrollArea->setWidget(w);
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    // 把垂直 ScrollBar 放到上边，实现“悬浮”效果
    QHBoxLayout *pHLayout_2 = new QHBoxLayout(m_pScrollArea);
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setContentsMargins(0, 0, 0, 0); // 修复废弃 API

    pVScrollBar->setHidden(true);
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);

    // 如果你有 initStyleSheet 函数，可以保留调用
    // initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    // 插入到倒数第一个位置（即弹簧的前面）
    vl->insertWidget(vl->count() - 1, item);
    isAppended = true; // 标记刚刚追加了新消息
}

void ChatView::removeAllItem()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    int count = layout->count();
    for (int i = 0; i < count - 1; ++i) {
        QLayoutItem *item = layout->takeAt(0);
        if (QWidget *widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::Enter && o == m_pScrollArea) {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if(e->type() == QEvent::Leave && o == m_pScrollArea) {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this); // 注意这里在 Qt 5 中通常用 initFrom(this)，如果是 Qt5 建议改为 opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatView::onVScrollBarMoved(int min, int max)
{
    Q_UNUSED(min); // 消除未使用变量的警告

    // 🌟 修复毒点 2：极其干脆的逻辑，坚决不用 500ms 定时器去猜
    if(isAppended)
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(max); // 直接拉到最底
        isAppended = false; // 立刻重置状态，干净利落
    }
}