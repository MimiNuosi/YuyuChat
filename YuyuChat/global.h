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

const int CHAT_COUNT_PER_PAGE = 13;

enum ReqID{
    ID_GER_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_RESET_PWD = 1003,
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_REP = 1006,
    ID_SEARCH_USER_REQ = 1007,//用户搜索请求
    ID_SEARCH_USER_RSP = 1008,//搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,//添加好友申请
    ID_ADD_FRIEND_RSP = 1010,//申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,//通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,//认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,//认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,//通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ = 1017,//文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP = 1018,//文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,//通知用户文本聊天信息
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

//聊天形式，私聊和群聊
enum class ChatFormType {
    PRIVATE = 0,
    GROUP = 1
};

//聊天消息类型，文本，图片，文件等
enum class ChatMsgType {
    TEXT = 0,
    PIC = 1,
    FILE = 2
};


#endif // GLOBAL_H
