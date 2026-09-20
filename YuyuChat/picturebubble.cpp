#include "picturebubble.h"
#include <QVBoxLayout>
#include <QMouseEvent>

PictureBubble::PictureBubble(const QPixmap& picture, ChatRole role, QWidget *parent)
    : BubbleFrame(role, parent)
{
    m_picLabel = new QLabel(this);
    m_picLabel->setScaledContents(true);
    m_picLabel->installEventFilter(this);

    // 进度条与遮罩
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->hide();

    m_overlayIcon = new QLabel(this);
    m_overlayIcon->setFixedSize(32, 32);
    m_overlayIcon->hide();

    if (this->layout() != nullptr) {
        delete this->layout();
    }

    QVBoxLayout* layout = new QVBoxLayout(this);
    if (role == ChatRole::Self) {
        layout->setContentsMargins(2, 2, 10, 2); // 右边预留尖角空间
    } else {
        layout->setContentsMargins(10, 2, 2, 2); // 左边预留尖角空间
    }
    layout->setSpacing(2);
    layout->addWidget(m_picLabel);
    layout->addWidget(m_progressBar);
    this->setLayout(layout);

    setPixmap(picture);
}

void PictureBubble::setPixmap(const QPixmap& pix) {
    if (pix.isNull()) return;
    // 按比例限制最大尺寸
    QPixmap scaled = pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_picLabel->setPixmap(scaled);
    m_picLabel->setFixedSize(scaled.size());

    int margin_h = 4; // layout 的上下 margin 累加
    int margin_w = 4; // layout 的左右 margin 累加
    int triangle_width = 8; // BubbleFrame 三角尖角预留宽度 (WIDTH_SANJIAO)

    int total_w = scaled.width() + margin_w + triangle_width;
    int total_h = scaled.height() + margin_h;

    // Frame 自身宽度与尺寸策略，防止被外层 Grid 网格列拉伸撑大
    this->setFixedWidth(total_w);
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void PictureBubble::setProgress(qint64 current, qint64 total) {
    if (total <= 0) return;
    m_progressBar->show();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(static_cast<int>(current * 100 / total));
}

void PictureBubble::setTransferState(TransferState state) {
    m_state = state;
    if (state == TransferState::Completed || state == TransferState::None) {
        m_progressBar->hide();
        m_overlayIcon->hide();
    } else if (state == TransferState::Uploading || state == TransferState::Downloading) {
        m_progressBar->show();
        m_overlayIcon->hide();
    } else if (state == TransferState::Paused) {
        m_overlayIcon->setPixmap(QPixmap(":/res/play.png")); // 显示继续图标
        m_overlayIcon->show();
    } else if (state == TransferState::Failed) {
        m_overlayIcon->setPixmap(QPixmap(":/res/retry.png")); // 显示重试图标
        m_overlayIcon->show();
    }
}

bool PictureBubble::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_picLabel && event->type() == QEvent::MouseButtonPress) {
        emit sig_clicked();
        return true;
    }
    return BubbleFrame::eventFilter(watched, event);
}