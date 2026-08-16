#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <QDialog>
#include "global.h"
#include "statewidget.h"
#include "userdata.h"
#include "loadingdialog.h"
#include <QListWidgetItem>
#include <QList>
namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void AddLBGroup(StateWidget *lb);
    void ClearLabelState(StateWidget *lb);
    void SetSelectChatItem(int uid = 0);
    void SetSelectChatPage(int uid = 0);
    void loadChatList();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void handleGlobalMousePress(QMouseEvent *event);
private:
    void ShowSearch(bool bsearch = false);
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode _state;
    bool _b_loading;
    QList<StateWidget*> _lb_list;
    QMap <int,QListWidgetItem*>_chat_items_added;
    int _cur_chat_uid;
    QWidget* _last_widget;
    void loadMoreChatUser();
    void loadMoreConUser();
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>>);
    void showLoadingDlg(bool b_show);
    LoadingDialog* _loading_dlg = nullptr;
    qint64 _next_last_thread_id = 0;
    bool _b_chat_load_more = false;  // 记录是否还有下一页
private slots:
    void slot_loading_chat_user();
    void slot_loading_contact_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString& str);
public slots:
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_switch_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_switch_chat_item(std::shared_ptr<UserInfo> user_info);
    void slot_switch_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slow_switch_apply_friend_page();
    void slot_item_clicked(QListWidgetItem* item);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msg);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    void slot_load_chat_thread(bool load_more, qint64 last_thread_id, std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads);
};

#endif // CHATDIALOG_H
