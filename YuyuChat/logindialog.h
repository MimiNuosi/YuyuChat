#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H
#include "global.h"
#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
private:
    void initHead();
    Ui::LoginDialog *ui;
    bool checkEmailValid();
    bool checkPwdValid();
    void initHttpHandlers();
    void showTip(QString str, bool b_ok);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    QMap<ReqID, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    int _uid;
    QString _token;
public slots:
    void slot_forget_password();
signals:
    void switchRegister();
    void switchReset();
    void sig_connect_tcp(ServerInfo);
private slots:
    void on_login_button_clicked();
    void slot_login_mod_finish(ReqID id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool b_success);
    void slot_login_failed(int err);

};

#endif // LOGINDIALOG_H
