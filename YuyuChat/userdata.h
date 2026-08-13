#ifndef USERDATA_H
#define USERDATA_H
#include <QString>
#include <memory>
#include <QJsonArray>
#include <vector>
#include <QJsonObject>
#include <QMap>
#include "global.h"

class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex , QString icon)
        : _uid(uid), _name(name), _nick(nick), _desc(desc), _sex(sex), _icon(icon){}
    int _uid;
    QString _name;
    QString _nick;
    QString _desc;
    int _sex;
    QString _icon;
};

class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc,
                   QString icon, QString nick, int sex)
        : _from_uid(from_uid), _name(name), _nick(nick), _desc(desc), _sex(sex), _icon(icon){}
    int _from_uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int     _sex;
};

struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc,
              QString icon, QString nick, int sex, int status)
        :_uid(uid),_name(name),_desc(desc),
        _icon(icon),_nick(nick),_sex(sex),_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :_uid(addinfo->_from_uid),_name(addinfo->_name),
        _desc(addinfo->_desc),_icon(addinfo->_icon),
        _nick(addinfo->_nick),_sex(addinfo->_sex),
        _status(0)
    {}
    void SetIcon(QString head){
        _icon = head;
    }
    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};

class TextChatData;
struct AuthInfo {
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex):
        _uid(uid), _name(name), _nick(nick), _icon(icon),
        _sex(sex), _thread_id(0){}

    void SetChatDatas(std::vector<std::shared_ptr<TextChatData>> _chat_datas);
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    int _thread_id;
    std::vector<std::shared_ptr<TextChatData>> _chat_datas;
};

struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name,
            QString peer_nick, QString peer_icon, int peer_sex)
        :_uid(peer_uid),_name(peer_name),_nick(peer_nick),
        _icon(peer_icon),_sex(peer_sex),_thread_id(0)
    {}
    void SetChatDatas(std::vector<std::shared_ptr<TextChatData>> _chat_datas);
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    int _thread_id;
    std::vector<std::shared_ptr<TextChatData>> _chat_datas;
};

struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = "", QString desc=""):
        _uid(uid),_name(name),_nick(nick),_icon(icon),_sex(sex),_desc(desc),_last_msg(last_msg){}

    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_desc(""),_last_msg(""){}

    UserInfo(int uid, QString name, QString icon):
        _uid(uid), _name(name), _icon(icon),_nick(_name),
        _sex(0),_desc(""),_last_msg(""){

    }

    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_desc(""),_last_msg(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        _uid(search_info->_uid),_name(search_info->_name),_nick(search_info->_nick),
        _icon(search_info->_icon),_sex(search_info->_sex), _desc(search_info->_desc),_last_msg(""){}

    void AppendChatMsgs(std::vector<std::shared_ptr<TextChatData>>);

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
    QString _last_msg;
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;
};

class ChatDataBase {
public:
    ChatDataBase(int msg_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
                 QString content,int _send_uid, int status, QString chat_time );
    ChatDataBase(QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
                 QString content, int send_uid, int status, QString chat_time);
    ChatDataBase(int msg_id, QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
                 QString content, int send_uid, int status, QString chat_time);
    int GetMsgId() { return _msg_id; }
    int GetThreadId() { return _thread_id; }
    ChatFormType GetFormType() { return _form_type; }
    ChatMsgType GetMsgType() { return _msg_type; }
    QString GetContent() { return _content; }
    int GetSendUid() { return _send_uid; }
    QString GetMsgContent(){return _content;}
    void SetUniqueId(int unique_id);
    QString GetUniqueId();
    int GetStatus() { return _status; }
    void SetMsgId(int msg_id) { _msg_id = msg_id; }
    void SetStatus(int status) { _status = status; }
private:
    //客户端本地唯一标识
    QString _unique_id;
    //消息id
    int _msg_id;
    //会话id
    int _thread_id;
    //群聊还是私聊
    ChatFormType _form_type;
    //文本信息为0，图片为1，文件为2
    ChatMsgType _msg_type;
    QString _content;
    //发送者id
    int _send_uid;
    //状态
    int _status;
    //聊天时间
    QString _chat_time;
};


struct TextChatData{
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid)
        :_msg_id(msg_id),_msg_content(msg_content),_from_uid(fromuid),_to_uid(touid)
    {
    }
    QString _msg_id;
    QString _msg_content;
    int _from_uid;
    int _to_uid;
};

struct TextChatMsg
{
    int _from_uid;
    int _to_uid;
    // 存放多条单条消息
    std::vector<std::shared_ptr<TextChatData>> _chat_msgs;

    // 构造函数：解析服务端下发的QJsonArray消息数组
    TextChatMsg(int fromuid, int touid, QJsonArray arrays)
        : _from_uid(fromuid), _to_uid(touid)
    {
        for (auto msg_data : arrays)
        {
            QJsonObject msg_obj = msg_data.toObject();
            QString content = msg_obj["content"].toString();
            QString msgid = msg_obj["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content, fromuid, touid);
            _chat_msgs.push_back(msg_ptr);
        }
    }
};

//聊天线程信息
struct ChatThreadInfo {
    int _thread_id;
    QString _type;     // "private" or "group"
    int _user1_id;    // 私聊时对应 private_chat.user1_id；群聊时设为 0
    int _user2_id;    // 私聊时对应 private_chat.user2_id；群聊时设为 0
};

#endif