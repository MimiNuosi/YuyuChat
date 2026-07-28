#ifndef AUTHENFRIEND_H
#define AUTHENFRIEND_H
#include <QDialog>
#include <QPushButton>
#include <QMap>
#include <QPoint>
#include "userdata.h"
namespace Ui {
class AuthenFriend;
}

class AuthenFriend : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenFriend(QWidget *parent = nullptr);
    ~AuthenFriend();
    void InitTipLbs();
    void AddTipLbs(QPushButton* lb, QPoint cur_point, QPoint &next_point, int text_width, int text_height);
    bool eventFilter(QObject *obj, QEvent *event);
    void SetApplyInfo(std::shared_ptr<ApplyInfo> apply_info);

private:
    Ui::AuthenFriend *ui;
    void resetLabels();
    //已经创建好的标签
    QMap<QString, QPushButton*> _add_labels;
    std::vector<QString> _add_label_keys;
    QPoint _label_point;
    //用来在输入框显示添加新好友的标签
    QMap<QString, QPushButton*> _friend_labels;
    std::vector<QString> _friend_label_keys;
    void addLabel(QString name);
    std::vector<QString> _tip_data;
    QPoint _tip_cur_point;
    std::shared_ptr<ApplyInfo> _apply_info;
public slots:
    //点击关闭，移除展示栏好友便签
    void SlotRemoveFriendLabel(QString);
    //处理确认回调
    void SlotApplySure();
    //处理取消回调
    void SlotApplyCancel();
};

#endif // AUTHENFRIEND_H
