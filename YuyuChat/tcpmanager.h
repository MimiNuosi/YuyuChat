#ifndef TCPMANAGER_H
#define TCPMANAGER_H
#include <QTcpSocket>
#include <QObject>
#include <functional>
#include "global.h"
#include "singleton.h"
#include "userdata.h"
#include <QTimer>
class TcpManager :public QObject,public Singleton<TcpManager>,public std::enable_shared_from_this<TcpManager>
{
    Q_OBJECT
public:
    ~TcpManager();
    void CloseConnection();
private:
    friend class Singleton<TcpManager>;
    TcpManager();
    void initHandlers();
    void handleMessage(ReqID id,int len,QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqID,std::function<void(ReqID id,int len,QByteArray data)>> _handlers;
    QTimer* _heart_timer;

public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqID id,QByteArray data);
signals:
    void sig_con_success(bool b_success);
    void sig_send_data(ReqID id,QByteArray data);
    void sig_switch_chatdialog();
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo> si);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_text_chat_msg(std::vector<std::shared_ptr<TextChatData>>);
    void sig_notify_offline();
    void sig_connection_close();
    void sig_load_chat_thread(bool load_more, int last_thread_id,std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads);
    void sig_create_private_chat(int uid, int other_id, int thread_id);
    void sig_load_chat_msg(int thread_id, int last_msg_id, bool load_more, std::shared_ptr<TextChatData> chat_data);
};

#endif // TCPMANAGER_H
