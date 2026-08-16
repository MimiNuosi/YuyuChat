#include "chatitembase.h"

ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent)
    : QWidget(parent)
    , m_role(role)
{
    m_pNameLabel    = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(20);
    m_pIconLabel    = new QLabel();
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(42, 42);
    m_pBubble       = new QWidget();
    QGridLayout *pGLayout = new QGridLayout();
    pGLayout->setVerticalSpacing(3);
    pGLayout->setHorizontalSpacing(3);
    pGLayout->setContentsMargins(3, 3, 3, 3);
    QSpacerItem*pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_pStatusLabel = new QLabel();
    m_pStatusLabel->setFixedSize(16,16);
    m_pStatusLabel->setScaledContents(true);
    if(m_role == ChatRole::Self)
    {
        m_pNameLabel->setContentsMargins(0,0,8,0);
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0,2, 1,1);
        pGLayout->addWidget(m_pIconLabel, 0, 3, 2,1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pStatusLabel,1,1,1,1,Qt::AlignCenter);
        pGLayout->addWidget(m_pBubble, 1,2, 1,1);
        pGLayout->setColumnStretch(0, 2);
        pGLayout->setColumnStretch(1, 0);
        pGLayout->setColumnStretch(2, 3);
        pGLayout->setColumnStretch(3, 0);
    }else{
        m_pNameLabel->setContentsMargins(8,0,0,0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2,1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0,1, 1,1);
        pGLayout->addWidget(m_pBubble, 1,1, 1,1);
        pGLayout->addItem(pSpacer, 1, 2, 1, 1);
        pGLayout->setColumnStretch(0, 0); // 头像固定
        pGLayout->setColumnStretch(1, 0); // 气泡根据内容自适应
        pGLayout->setColumnStretch(2, 1); // 右侧弹簧吸收剩余所有空间
    }
    this->setLayout(pGLayout);
}

void ChatItemBase::setUserName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w)
{
    QGridLayout *pGLayout = qobject_cast<QGridLayout *>(this->layout());

    // 1. 替换控件，并接收返回的旧布局项
    QLayoutItem *oldItem = pGLayout->replaceWidget(m_pBubble, w);
    if (oldItem) {
        delete oldItem; // 必须删除旧的布局项以防止内存泄漏
    }

    m_pBubble->deleteLater();
    m_pBubble = w;

    //  2. 必须手动调用 show() 唤醒新气泡
    m_pBubble->show();
}

void ChatItemBase::setStatus(int status)
{
    if (status == MessageStatus::UN_READ) {
        m_pStatusLabel->setPixmap(QPixmap(":/res/unread.png"));
        return;
    }

    if (status == MessageStatus::SEND_FAILED) {
        m_pStatusLabel->setPixmap(QPixmap(":/res/send_fail.png"));
        return;
    }

    if (status == MessageStatus::READED) {
        m_pStatusLabel->setPixmap(QPixmap(":/res/readed.png"));
        return;
    }

}