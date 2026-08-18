#ifndef CHATUSERWID_H
#define CHATUSERWID_H
#include <QWidget>
#include "listitembase.h"
#include "userdata.h"
namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const;

    void SetChatInfo(std::shared_ptr<ChatThreadData> chat_data);
    std::shared_ptr<ChatThreadData> GetChatInfo();
    void UpdateLastMsg(std::vector<std::shared_ptr<TextChatData>>);
    void ShowRedPoint(bool b_show);
private:
    Ui::ChatUserWid *ui;
    std::shared_ptr<ChatThreadData> _chat_data;
};

#endif // CHATUSERWID_H