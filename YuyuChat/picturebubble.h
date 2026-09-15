#ifndef PICTUREBUBBLE_H
#define PICTUREBUBBLE_H

#include "BubbleFrame.h"
#include <QHBoxLayout>
#include <QPixmap>
#include <QProgressBar>
#include "global.h"

class PictureBubble : public BubbleFrame {
    Q_OBJECT
public:
    PictureBubble(const QPixmap& picture, ChatRole role, QWidget *parent = nullptr);
    ~PictureBubble() = default;

    // 纯 View 槽位接口，不与任何业务数据结构绑定
    void setPixmap(const QPixmap& pix);
    void setProgress(qint64 current, qint64 total);
    void setTransferState(TransferState state);

signals:
    void sig_clicked(); // 仅通知上层被点击

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel* m_picLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_overlayIcon = nullptr; // 暂停/播放/失败遮罩
    TransferState m_state = TransferState::None;
};
#endif // PICTUREBUBBLE_H
