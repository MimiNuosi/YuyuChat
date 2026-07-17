#include "resetdialog.h"
#include "ui_resetdialog.h"
#include <QDebug>
#include <QRegularExpression>
#include "httpmanager.h"
#include <QAction>
#include <QIcon>

ResetDialog::ResetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResetDialog)
{
    ui->setupUi(this);

    // 1. 设置默认密码遮罩形式
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);
    ui->err_tip->clear();

    // 2. 连接失去焦点时的校验信号
    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this](){ checkUserValid(); });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){ checkEmailValid(); });
    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){ checkPassValid(); });
    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){ checkConfirmValid(); });
    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){ checkVerifyValid(); });

    // 3. 注册 HTTP 回包处理器，并连接网络请求完成信号
    initHandlers();
    connect(HttpManager::GetInstance().get(), &HttpManager::sig_reset_mod_finish,
            this, &ResetDialog::slot_reset_mod_finish);

    // 4. 为【新密码】输入框添加原生“小眼睛”Action
    QAction *passAction = new QAction(this);
    passAction->setIcon(QIcon(":/res/unvisible.png"));
    ui->pass_edit->addAction(passAction, QLineEdit::TrailingPosition);
    connect(passAction, &QAction::triggered, this, [=]() {
        if (ui->pass_edit->echoMode() == QLineEdit::Password) {
            ui->pass_edit->setEchoMode(QLineEdit::Normal);
            passAction->setIcon(QIcon(":/res/visible.png"));
        } else {
            ui->pass_edit->setEchoMode(QLineEdit::Password);
            passAction->setIcon(QIcon(":/res/unvisible.png"));
        }
    });

    // 5. 为【确认密码】输入框添加原生“小眼睛”Action
    QAction *confirmAction = new QAction(this);
    confirmAction->setIcon(QIcon(":/res/unvisible.png"));
    ui->confirm_edit->addAction(confirmAction, QLineEdit::TrailingPosition);
    connect(confirmAction, &QAction::triggered, this, [=]() {
        if (ui->confirm_edit->echoMode() == QLineEdit::Password) {
            ui->confirm_edit->setEchoMode(QLineEdit::Normal);
            confirmAction->setIcon(QIcon(":/res/visible.png"));
        } else {
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
            confirmAction->setIcon(QIcon(":/res/unvisible.png"));
        }
    });
}

ResetDialog::~ResetDialog()
{
    delete ui;
}

// ---------------- UI 提示功能 ----------------
void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

// ---------------- 校验逻辑 (复用 Utils) ----------------
bool ResetDialog::checkUserValid()
{
    QString err_msg;
    if (!Utils::CheckUserValid(ui->user_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_USER_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool ResetDialog::checkEmailValid()
{
    QString err_msg;
    if (!Utils::CheckEmailValid(ui->email_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_EMAIL_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool ResetDialog::checkPassValid()
{
    QString err_msg;
    if (!Utils::CheckPassValid(ui->pass_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_PWD_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool ResetDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();

    if(confirm.isEmpty()){
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("确认密码不能为空"));
        return false;
    }
    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("两次输入的密码不一致"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_CONFIRM);
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
    return true;
}

bool ResetDialog::checkVerifyValid()
{
    QString err_msg;
    if (!Utils::CheckVerifyValid(ui->verify_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_VERIFY_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

// ---------------- 按钮点击事件 ----------------
void ResetDialog::on_get_code_clicked()
{
    if (!checkEmailValid()) return;

    QJsonObject json_obj;
    json_obj["email"] = ui->email_edit->text();
    HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_verifycode"),
                                            json_obj, ReqID::ID_GER_VERIFY_CODE, Modules::RESETMOD);
}

void ResetDialog::on_confirm_button_clicked()
{
    if (!checkUserValid() || !checkEmailValid() || !checkPassValid() || !checkConfirmValid() || !checkVerifyValid()) {
        return;
    }

    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["password"] = xorString(ui->pass_edit->text());
    json_obj["verifycode"] = ui->verify_edit->text();

    HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/reset_pwd"),
                                            json_obj, ReqID::ID_RESET_PWD, Modules::RESETMOD);
}

void ResetDialog::on_cancel_button_clicked()
{
    // 点击取消直接发送信号退回登录界面
    emit switchLogin();
}

// ---------------- 网络回包处理 ----------------
void ResetDialog::initHandlers()
{
    // 获取验证码回包逻辑
    _handlers.insert(ReqID::ID_GER_VERIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
    });

    // 重置密码回包逻辑
    _handlers.insert(ReqID::ID_RESET_PWD, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("重置失败或参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("重置成功，点击返回登录"),true);
        qDebug()<<"email is "<<email<<"\n";
        qDebug()<<"User id is "<<jsonObj["uid"].toString();
    });
}

void ResetDialog::slot_reset_mod_finish(ReqID id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"), false);
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull() || !jsonDoc.isObject()){
        showTip(tr("JSON解析错误"), false);
        return;
    }

    // 执行对应 ReqID 的回调
    if (_handlers.contains(id)) {
        _handlers[id](jsonDoc.object());
    }
}