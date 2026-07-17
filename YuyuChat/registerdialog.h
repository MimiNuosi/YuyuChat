#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H
#include "global.h"
#include <QDialog>
#include <QAction>
#include <QIcon>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();
private slots:
    void on_get_code_clicked();
    void slot_reg_mod_finish(ReqID id,QString res,ErrorCodes err);
    void on_confirm_button_clicked();
    void on_pushButton_clicked();
    void on_cancel_button_clicked();
private:
    void initHttpHandlers();
    void showTip(QString str,bool b_ok);
    void ChangeTipPage();
    Ui::RegisterDialog *ui;
    QMap<ReqID,std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkConfirmValid();
    bool checkVerifyValid();
    QTimer *_countdown_timer;
    int _countdown;
signals:
    void signSwitchLogin();
};

#endif // REGISTERDIALOG_H
