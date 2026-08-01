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

    void SetInfo(std::shared_ptr<UserInfo> user_info);

    std::shared_ptr<UserInfo> GetUserInfo();
    void UpdateLastMsg(std::vector<std::shared_ptr<TextChatData>>);
private:
    Ui::ChatUserWid *ui;
    std::shared_ptr<UserInfo> _user_info;
};

#endif // CHATUSERWID_H