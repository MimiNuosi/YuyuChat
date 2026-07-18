#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include <memory>
#include <mutex>
#include <iostream>
#include <Qstyle>
#include <QByteArray>
#include <QJsonObject>
#include <QDir>
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QSettings.h>
extern std::function<void(QWidget*)> repolish;

extern std::function<QString(QString)> xorString;

extern QString gate_url_prefix;

enum ReqID{
    ID_GER_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_RESET_PWD = 1003,
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_REP = 1006,
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VERIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum Modules{
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

enum ChatUIMode{
    SearchMode,
    ChatMode,
    ContactMode,
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
    TOKEN_INVALID = 3,
    UID_INVALID = 4,
};

enum ListItemType{
    CHAT_USER_ITEM,//聊天用户
    CONTACT_USER_ITEM,//联系人用户
    SEARCH_USER_ITEM,//搜索到的用户
    ADD_USER_TIP_ITEM,//提示添加用户
    INVALID_ITEM,//不可点击条目
    GROUP_TIP_ITEM,//分组提示条目
    LINE_ITEM,//分割线
    APPLY_FRIEND_ITEM,//好友申请条目
};

enum class ChatRole{
    Self,
    Other,
};

enum class ClickLbState {
    Normal,   // 普通未选中状态
    Selected  // 被点击选中状态
};

struct ServerInfo {
    int Uid;
    QString Host;
    QString Port;
    QString Token;
};

struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url，文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

namespace Utils {
bool CheckEmailValid(const QString& email, QString& err_msg);
bool CheckPassValid(const QString& pass, QString& err_msg);
bool CheckUserValid(const QString& user, QString& err_msg);
bool CheckVerifyValid(const QString& verify, QString& err_msg);
}



inline std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

inline std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

inline std::vector<QString> names = {
    "llfc",
    "zack",
    "golang",
    "cpp",
    "java",
    "nodejs",
    "python",
    "rust"
};


#endif // GLOBAL_H
