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
//TCP文件上传包头长度(2字节ID + 2字节长度)
#define FILE_UPLOAD_HEAD_LEN 4
//TCP ID长度
#define FILE_UPLOAD_ID_LEN 2
//TCP 长度字段的长度
#define FILE_UPLOAD_LEN_LEN 4
//最大文件长度
#define MAX_FILE_LEN (1024*32)
//定义最大拥塞窗口的大小
#define MAX_CWND_SIZE 5

enum ReqID {
    // ================= 1001 ~ 1004: HTTP 登录注册与验证码 (GateServer) =================
    ID_GER_VERIFY_CODE          = 1001, // 获取验证码
    ID_REG_USER                 = 1002, // 注册用户
    ID_RESET_PWD                = 1003, // 重置密码
    ID_LOGIN_USER               = 1004, // HTTP 登录请求

    // ================= 1005 ~ 1030: 聊天与用户信令 (ChatServer) =================
    MSG_CHAT_LOGIN              = 1005, // 用户 TCP 登录
    ID_CHAT_LOGIN_RSP           = 1006, // 用户登录回包 (服务端为 MSG_CHAT_LOGIN_RSP)
    ID_SEARCH_USER_REQ          = 1007, // 搜索用户请求
    ID_SEARCH_USER_RSP          = 1008, // 搜索用户回包
    ID_ADD_FRIEND_REQ           = 1009, // 申请添加好友请求
    ID_ADD_FRIEND_RSP           = 1010, // 申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ    = 1011, // 通知用户有新的好友申请
    ID_AUTH_FRIEND_REQ          = 1013, // 认证好友请求
    ID_AUTH_FRIEND_RSP          = 1014, // 认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ   = 1015, // 通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ        = 1017, // 文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP        = 1018, // 文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, // 通知用户文本聊天信息
    ID_NOTIFY_OFF_LINE_REQ      = 1021, // 通知用户下线
    ID_HEART_BEAT_REQ           = 1023, // 心跳请求
    ID_HEART_BEAT_RSP           = 1024, // 心跳回复
    ID_LOAD_CHAT_THREAD_REQ     = 1025, // 加载聊天线程/会话列表请求
    ID_LOAD_CHAT_THREAD_RSP     = 1026, // 加载聊天线程/会话列表回复
    ID_CREATE_PRIVATE_CHAT_REQ  = 1027, // 创建私聊线程请求
    ID_CREATE_PRIVATE_CHAT_RSP  = 1028, // 创建私聊线程回复
    ID_LOAD_CHAT_MSG_REQ        = 1029, // 加载聊天消息历史请求
    ID_LOAD_CHAT_MSG_RSP        = 1030, // 加载聊天消息历史回复

    // ================= 1031 ~ 1060: 文件与多媒体资源传输 (ResourceServer) =================
    ID_UPLOAD_HEAD_ICON_REQ         = 1031, // 上传头像请求
    ID_UPLOAD_HEAD_ICON_RSP         = 1032, // 上传头像回复
    ID_DOWN_LOAD_FILE_REQ           = 1033, // 下载文件请求
    ID_DOWN_LOAD_FILE_RSP           = 1034, // 下载文件回复
    ID_IMG_CHAT_MSG_REQ             = 1035, // 图片聊天消息请求
    ID_IMG_CHAT_MSG_RSP             = 1036, // 图片聊天消息回复
    ID_IMG_CHAT_UPLOAD_REQ          = 1037, // 上传聊天图片资源请求
    ID_IMG_CHAT_UPLOAD_RSP          = 1038, // 上传聊天图片资源回复
    ID_NOTIFY_IMG_CHAT_MSG_REQ      = 1039, // 通知用户图片消息
    ID_FILE_INFO_SYNC_REQ           = 1041, // 文件信息同步请求
    ID_FILE_INFO_SYNC_RSP           = 1042, // 文件信息同步回复
    ID_IMG_CHAT_CONTINUE_UPLOAD_REQ = 1043, // 续传聊天图片请求
    ID_IMG_CHAT_CONTINUE_UPLOAD_RSP = 1044, // 续传聊天图片回复
    ID_IMG_CHAT_DOWN_INFO_SYNC_REQ  = 1045, // 获取聊天图片下载同步信息
    ID_IMG_CHAT_DOWN_INFO_SYNC_RSP  = 1046, // 获取聊天图片下载同步信息回复
    ID_IMG_CHAT_DOWN_REQ            = 1047, // 聊天图片下载请求
    ID_IMG_CHAT_DOWN_RSP            = 1048, // 聊天图片下载回复

    // 独立号段（测试与基础文件分块）
    ID_TEST_MSG_REQ                 = 1051, // 测试消息请求
    ID_TEST_MSG_RSP                 = 1052, // 测试消息回复
    ID_UPLOAD_FILE_REQ              = 1053, // 普通文件上传请求
    ID_UPLOAD_FILE_RSP              = 1054, // 普通文件上传回复
    ID_SYNC_FILE_REQ                = 1055, // 普通文件同步请求
    ID_SYNC_FILE_RSP                = 1056, // 普通文件同步回复
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
    SettingsMode,
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

enum MessageStatus{
    UN_READ = 0,
    SEND_FAILED = 1,
    READED = 2,
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
    ServerInfo() = default;
    ServerInfo(const ServerInfo& other):_chat_host(other._chat_host),_chat_port(other._chat_port),
        _token(other._token),_uid(other._uid){}
    QString _chat_host;
    QString _chat_port;
    QString _res_host;
    QString _res_port;
    QString _token;
    int _uid;
};

struct MsgInfo{
    MsgInfo() = default;
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url，文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

namespace Utils {
bool CheckEmailValid(const QString& email, QString& err_msg);
bool CheckPassValid(const QString& pass, QString& err_msg);
bool CheckUserValid(const QString& user, QString& err_msg);
bool CheckVerifyValid(const QString& verify, QString& err_msg);
QPixmap GetAvatarPixmap(const QString& icon_str);
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

struct DownloadInfo {
    QString _name;
    int _total_size;
    int _current_size;
    int _seq;
    QString _client_path;
};



Q_DECLARE_METATYPE(ServerInfo)
Q_DECLARE_METATYPE(ReqID)
#endif // GLOBAL_H
