#ifndef FILETCPMANAGER_H
#define FILETCPMANAGER_H

#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"
#include <QJsonArray>
#include <memory>
#include <QThread>
#include <QQueue>
#include <memory>
#include "global.h"

class FileTcpThread {
public:
    FileTcpThread();
    ~FileTcpThread();
private:
    QThread _file_tcp_thread;
};

class FileTcpManager:public QObject, public Singleton<FileTcpManager>,
                       public std::enable_shared_from_this<FileTcpManager>
{
    Q_OBJECT
public:
    friend class Singleton<FileTcpManager>;
    ~FileTcpManager() = default;
    void SendData(ReqID , QByteArray);
    void CloseConnection();
    //void SendDownlowdData(std::shared_ptr<DownloadInfo> download_info,QString req_type);

private:
    FileTcpManager();
    void initHandlers();
    void handleMessage(ReqID id,int len,QByteArray data);
    void registerMetaType();

    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqID,std::function<void(ReqID id,int len,QByteArray data)>> _handlers;
    //发送队列
    QQueue<QByteArray> _send_queue;
    //正在发送的包
    QByteArray  _current_block;
    //当前已发送的字节数
    qint64  _bytes_sent;
    //是否正在发送
    bool _pending;

signals:
    void sig_send_data(ReqID ID,QByteArray data);
    void sig_tcp_close();
    void sig_con_success(bool b_success);
    void sig_connection_closed();

public slots:
    void slot_send_data(ReqID id,QByteArray data);
    void slot_tcp_connect(std::shared_ptr<ServerInfo> si);
    void slot_tcp_close();
};

#endif // FILETCPMANAGER_H
