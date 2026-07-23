#include "logindialog.h"
#include "ui_logindialog.h"
#include <QPainterPath>
#include <QPainter>
#include "httpmanager.h"
#include "tcpmanager.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_button,&QPushButton::clicked,this,&LoginDialog::switchRegister);
    connect(ui->forget_button, &QPushButton::clicked, this, &LoginDialog::switchReset);
    initHead();
    initHttpHandlers();
    connect(HttpManager::GetInstance().get(), &HttpManager::sig_login_mod_finish,this, &LoginDialog::slot_login_mod_finish);
    connect(this, &LoginDialog::sig_connect_tcp, TcpManager::GetInstance().get(), &TcpManager::slot_tcp_connect);
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    connect(TcpManager::GetInstance().get(), &TcpManager::sig_login_failed,this, &LoginDialog::slot_login_failed);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::initHead()
{
    // 加载图片
    QPixmap originalPixmap(":/res/head_1.jpg");
    // 设置图片自动缩放
    qDebug()<< originalPixmap.size() << ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建一个和原始图片相同大小的QPixmap，用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent); // 用透明色填充

    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing); // 设置抗锯齿，使圆角更平滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0, 0, originalPixmap.width(), originalPixmap.height(), 10, 10); // 最后两个参数分别是x和y方向的圆角半径
    painter.setClipPath(path);

    // 将原始图片绘制到roundedPixmap上
    painter.drawPixmap(0, 0, originalPixmap);

    // 设置绘制好的圆角图片到QLabel上
    ui->head_label->setPixmap(roundedPixmap);
}

void LoginDialog::slot_forget_password()
{
    qDebug()<<"slot forget password";
    emit switchReset();
}

// ---------------- UI 错误提示封装 ----------------
void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

// ---------------- 复用 Utils 的校验逻辑 ----------------
bool LoginDialog::checkEmailValid()
{
    QString err_msg;
    // 直接调全局封装，拒绝重复写 if-else!
    if (!Utils::CheckEmailValid(ui->email_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_EMAIL_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool LoginDialog::checkPwdValid()
{
    QString err_msg;
    if (!Utils::CheckPassValid(ui->pass_edit->text(), err_msg)) {
        AddTipErr(TipErr::TIP_PWD_ERR, err_msg);
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

void LoginDialog::initHttpHandlers()
{
    _handlers.insert(ReqID::ID_LOGIN_USER, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("账号或密码错误"), false); // 出于安全，登录失败不具体提示哪个错
            return;
        }
        auto email = jsonObj["email"].toString();

        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;

        showTip(tr("登录成功"), true);
        qDebug() << "User logged in: " << email;
        emit sig_connect_tcp(si);
    });
}

void LoginDialog::on_login_button_clicked()
{
    if (!checkEmailValid() || !checkPwdValid()) {
        return; // 只要有一个不合法，直接拦截，不浪费网络资源
    }

    ui->login_button->setEnabled(false);
    auto email = ui->email_edit->text();
    auto password = ui->pass_edit->text();

    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["password"] = xorString(password);

    HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
                                            json_obj, ReqID::ID_LOGIN_USER, Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(ReqID id, QString res, ErrorCodes err)
{
    ui->login_button->setEnabled(true);
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

void LoginDialog::slot_tcp_con_finish(bool b_success)
{
    if(b_success){
        showTip(tr("聊天服务连接成功，正在登录..."),true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

        //发送tcp请求给chat server
        emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_CHAT_LOGIN, jsonData);

    }else{
        showTip(tr("网络异常"),false);
    }

}

void LoginDialog::slot_login_failed(int err)
{
    QString tipMsg;

    // 根据后端的错误码，给用户翻译成“人话”
    switch (err) {
    case ErrorCodes::TOKEN_INVALID:
    case ErrorCodes::UID_INVALID:
        tipMsg = tr("登录状态已失效，请重新输入密码登录！");
        break;
    case ErrorCodes::ERR_NETWORK:
        tipMsg = tr("聊天服务器连接异常，请检查网络！");
        break;
    case ErrorCodes::ERR_JSON:
        tipMsg = tr("客户端数据解析异常！");
        break;
    default:
        tipMsg = tr("未知错误，错误码：%1").arg(err);
        break;
    }

    showTip(tipMsg, false);
}

