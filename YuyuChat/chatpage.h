#ifndef CHATPAGE_H
#define CHATPAGE_H
#include "userdata.h"
#include "chatitembase.h"
#include <QWidget>

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void SetChatData(std::shared_ptr<ChatThreadData> chat_data);
    void AppendChatMsg(std::shared_ptr<ChatDataBase> msg);
    void UpdateChatStatus(std::shared_ptr<ChatDataBase> msg);
    void UpdateFileProgress(std::shared_ptr<MsgInfo> msg_info);
    void DownloadFileFinished(std::shared_ptr<MsgInfo> msg_info, QString file_path);
protected:
    void paintEvent(QPaintEvent *event);
private slots:
    void on_send_button_clicked();

private:
    Ui::ChatPage *ui;
    std::shared_ptr<UserInfo> _user_info;
    std::shared_ptr<ChatThreadData> _chat_data;
    QMap<QString, QWidget*>  _bubble_map;
    QHash<QString, ChatItemBase*> _unrsp_item_map;//未回复的消息集
    QHash<qint64, ChatItemBase*> _base_item_map;//已回复的消息集
    QMap<int, std::shared_ptr<MsgInfo>> _msg_id_to_img_info;
signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData> msg);

};

#endif // CHATPAGE_H
