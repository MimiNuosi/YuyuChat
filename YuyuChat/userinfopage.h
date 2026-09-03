#ifndef USERINFOPAGE_H
#define USERINFOPAGE_H

#include <QWidget>
#include <QLabel>
#include <memory>

namespace Ui {
class UserInfoPage;
}

class UserInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoPage(QWidget *parent = nullptr);
    ~UserInfoPage();

signals:
    void sig_reset_head(); // 通知主窗口刷新侧边栏/聊天列表头像

private slots:
    void slot_upload_avatar();
    void on_submit_button_clicked();

private:
    void LoadHeadIcon(const QString& avatarPath, QLabel* icon_label, const QString& file_name, const QString& req_type);
    QString generateUniqueIconName();

    Ui::UserInfoPage *ui;
};

#endif // USERINFOPAGE_H