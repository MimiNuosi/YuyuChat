#ifndef TCPMANAGER_H
#define TCPMANAGER_H
#include <QTcpSocket>
#include <QObject>
#include <functional>
#include "global.h"
#include "singleton.h"
#include "searchlist.h"
class TcpManager :public QObject,public Singleton<TcpManager>,public std::enable_shared_from_this<TcpManager>
{
    Q_OBJECT
public:
    ~TcpManager();
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
public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqID id,QString data);
signals:
    void sig_con_success(bool b_success);
    void sig_send_data(ReqID id,QString data);
    void sig_switch_chatdialog();
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo> si);
};

#endif // TCPMANAGER_H
