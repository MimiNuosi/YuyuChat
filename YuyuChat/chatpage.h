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
    void UpdateChatStatus(const QString& unique_id, int status);
protected:
    void paintEvent(QPaintEvent *event);
private slots:
    void on_send_button_clicked();

private:
    Ui::ChatPage *ui;
    std::shared_ptr<UserInfo> _user_info;
    std::shared_ptr<ChatThreadData> _chat_data;

signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData> msg);
    QMap<QString, QWidget*>  _bubble_map;
    QHash<QString, ChatItemBase*> _unrsp_item_map;
};

#endif // CHATPAGE_H
