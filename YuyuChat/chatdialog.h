#ifndef CHATDIALOG_H
#define CHATDIALOG_H
#include <QDialog>
#include "global.h"
#include "statewidget.h"
#include "userdata.h"
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
    void addChatUserList();
    void AddLBGroup(StateWidget *lb);
    void ClearLabelState(StateWidget *lb);
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
private slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString& str);
public slots:
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
};

#endif // CHATDIALOG_H
