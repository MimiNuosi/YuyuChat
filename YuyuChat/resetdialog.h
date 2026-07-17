#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();

signals:
    // 切换回登录界面的信号
    void switchLogin();

private slots:
    // HTTP 处理完成的槽函数
    void slot_reset_mod_finish(ReqID id, QString res, ErrorCodes err);

    void on_get_code_clicked();
    void on_confirm_button_clicked();
    void on_cancel_button_clicked();

private:
    void initHandlers();
    void showTip(QString str, bool b_ok);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    // 输入框失去焦点时的本地校验包装函数
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkConfirmValid();
    bool checkVerifyValid();
    Ui::ResetDialog *ui;

    // 用于保存不同网络请求 (ReqID) 对应的回调处理逻辑
    QMap<ReqID, std::function<void(const QJsonObject&)>> _handlers;

    // 用于保存当前的错误提示状态（保证多个错误同时存在时，修复一个还能显示另一个）
    QMap<TipErr, QString> _tip_errs;
};

#endif // RESETDIALOG_H