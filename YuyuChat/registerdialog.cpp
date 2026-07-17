#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "httpmanager.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog),_countdown(5)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentWidget(ui->page);

    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state","normal");
    repolish(ui->err_tip);
    connect(HttpManager::GetInstance().get(),&HttpManager::sig_reg_mod_finish,
            this,&RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers();
    ui->err_tip->clear();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });

    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){
        checkVerifyValid();
    });

    // 1. 为密码输入框添加 Action
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

    // 2. 为确认密码输入框添加 Action
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

    _countdown_timer = new QTimer(this);
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown==0){
            _countdown_timer->stop();
            emit signSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_label->setText(str);
    });
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    // 启动定时器，设置间隔为1000毫秒（1秒）
    _countdown_timer->start(1000);
}

void RegisterDialog::slot_reg_mod_finish(ReqID id,QString res,ErrorCodes err){
    if(err!=ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }
    _handlers[id](jsonDoc.object());
    return;
}

void RegisterDialog::on_confirm_button_clicked()
{
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkEmailValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkConfirmValid();
    if(!valid){
        return;
    }

    valid = checkVerifyValid();
    if(!valid){
        return;
    }

    //day11 发送http请求注册用户
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["password"] = xorString(ui->pass_edit->text());
    json_obj["confirm"] = xorString(ui->confirm_edit->text());
    json_obj["verifycode"] = ui->verify_edit->text();
    HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqID::ID_REG_USER,Modules::REGISTERMOD);
}

void RegisterDialog::initHttpHandlers(){
    _handlers.insert(ReqID::ID_GER_VERIFY_CODE,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发到邮箱，请注意查收哦"),true);
        qDebug() << "email is " << email <<"\n";
    });

    _handlers.insert(ReqID::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug()<< "user uuid is " << jsonObj["uuid"].toString();
        qDebug()<< "email is " << email ;
        ChangeTipPage();
    });
}

void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if(match){
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_verifycode"),json_obj,ReqID::ID_GER_VERIFY_CODE,Modules::REGISTERMOD);

    }else{
        showTip(tr("邮箱地址不正确"),false);
    }
}

void RegisterDialog::showTip(QString str,bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}

bool RegisterDialog::checkUserValid()
{
    QString err_msg;
    if(!Utils::CheckUserValid(ui->user_edit->text(), err_msg)){
        AddTipErr(TipErr::TIP_USER_ERR, err_msg);
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkEmailValid()
{
    QString err_msg;
    if(!Utils::CheckEmailValid(ui->email_edit->text(), err_msg)){
        AddTipErr(TipErr::TIP_EMAIL_ERR, err_msg);
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegisterDialog::checkPassValid()
{
    QString err_msg;
    if(!Utils::CheckPassValid(ui->pass_edit->text(), err_msg)){
        AddTipErr(TipErr::TIP_PWD_ERR, err_msg);
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();

    // 1. 先检查确认密码框是否为空
    if(confirm.isEmpty()){
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("确认密码不能为空"));
        return false;
    }

    // 2. 检查两次输入的密码是否一致
    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("两次输入的密码不一致"));
        return false;
    }
    else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }

    // 3. 校验通过，清除错误提示
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
    return true;
}

bool RegisterDialog::checkVerifyValid()
{
    QString err_msg;
    if(!Utils::CheckVerifyValid(ui->verify_edit->text(), err_msg)){
        AddTipErr(TipErr::TIP_VERIFY_ERR, err_msg);
        return false;
    }

    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

void RegisterDialog::on_pushButton_clicked()
{
    _countdown_timer->stop();
    emit signSwitchLogin();
}

void RegisterDialog::on_cancel_button_clicked()
{
    _countdown_timer->stop();
    emit signSwitchLogin();
}

