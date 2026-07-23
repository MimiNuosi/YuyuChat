#include "global.h"

std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

QString gate_url_prefix = "";

std::function<QString(QString)> xorString = [](QString input){
    QString result = input;
    int length = input.length();
    length %= 255;
    for(int i=0;i<length;i++){
        result[i] = QChar(static_cast<ushort>(input[i].unicode()^static_cast<ushort>(length)));
    }
    return result;
};

namespace Utils {
bool CheckEmailValid(const QString& email, QString& err_msg) {
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    if (!regex.match(email).hasMatch()) {
        err_msg = "邮箱地址不正确";
        return false;
    }
    return true;
}

bool CheckPassValid(const QString& pass, QString& err_msg) {
    if (pass.length() < 6 || pass.length() > 15) {
        err_msg = "密码长度应为6~15";
        return false;
    }
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    if (!regExp.match(pass).hasMatch()) {
        err_msg = "不能包含非法字符";
        return false;
    }
    return true;
}

bool CheckUserValid(const QString& user, QString& err_msg) {
    if (user.isEmpty()) {
        err_msg = "用户名不能为空";
        return false;
    }
    return true;
}

bool CheckVerifyValid(const QString& verify, QString& err_msg) {
    if (verify.isEmpty()) {
        err_msg = "验证码不能为空";
        return false;
    }
    return true;
}
}