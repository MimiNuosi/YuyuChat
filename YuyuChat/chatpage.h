#ifndef CHATPAGE_H
#define CHATPAGE_H
#include "userdata.h"
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
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendChatMsg(std::shared_ptr<TextChatData> msg);
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
};

#endif // CHATPAGE_H
