#include "tcpmanager.h"
#include <QAbstractSocket>
#include "usermanager.h"
TcpManager::~TcpManager(){

}

void TcpManager::CloseConnection(){
    _socket.close();
}

TcpManager::TcpManager() :_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    _heart_timer = new QTimer(this);
    QObject::connect(_heart_timer, &QTimer::timeout, [&](){
        // 构造心跳包的 JSON 数据并发送
        auto user_info = UserManager::GetInstance()->GetUserInfo();
        if (!user_info) {
            return; // 还没登录就不发心跳
        }

        QJsonObject jsonObj;
        jsonObj["uid"] = user_info->_uid;
        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 触发发送心跳请求
        emit sig_send_data(ReqID::ID_HEART_BEAT_REQ, jsonData);
    });

    QObject::connect(&_socket,&QTcpSocket::connected,[&](){
        _heart_timer->start(10000);
        qDebug()<< "Connected to server" ;
        emit sig_con_success(true);
    });

    QObject::connect(&_socket,&QTcpSocket::readyRead,[&](){
        _buffer.append(_socket.readAll());
        qDebug() << "[网络日志] 收到数据，当前缓冲区总长度: " << _buffer.size();

        forever{
            QDataStream stream(&_buffer,QIODevice::ReadOnly);
            stream.setVersion(QDataStream::Qt_5_0);
            stream.setByteOrder(QDataStream::BigEndian);

            if(!_b_recv_pending){
                if(_buffer.size() < static_cast<int>(sizeof(quint16)*2)){
                    qDebug() << "[网络日志] 数据不足包头长度(4字节)，继续等待...";
                    return;
                }

                stream >> _message_id >> _message_len;
                _buffer = _buffer.mid(sizeof(quint16)*2);

                qDebug() << "[网络日志] 解析出包头 -> 消息ID: " << _message_id << ", 负载长度: " << _message_len;
            }

            if(_buffer.size() < _message_len){
                qDebug() << "[网络日志] 负载数据未接收完整 (目前 " << _buffer.size() << " / 需要 " << _message_len << ")，发生拆包，继续等待...";
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            QByteArray message = _buffer.mid(0, _message_len);

            qDebug() << "[网络日志] 成功读取完整消息负载: " << message;

            _buffer = _buffer.mid(_message_len);
            handleMessage(ReqID(_message_id), _message_len, message);
        }
    });

    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError)
        qDebug() << "Error:" << _socket.errorString();
    });

    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        _heart_timer->stop();
        qDebug() << "Disconnected from server.";
        emit sig_connection_close();
    });

    QObject::connect(this, &TcpManager::sig_send_data, this, &TcpManager::slot_send_data);
    initHandlers();


}

void TcpManager::initHandlers()
{
    _handlers.insert(ReqID::ID_CHAT_LOGIN_REP,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login error , json parse error :" << err;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() <<"Login error: "<<err;
            emit sig_login_failed(err);
            return;
        }

        auto uid =jsonObj["uid"].toInt();
        auto name =jsonObj["name"].toString();
        auto nick =jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex= jsonObj["sex"].toInt();

        auto user_info = std::make_shared<UserInfo>(uid,name,nick,icon,sex);
        UserManager::GetInstance()->SetToken(jsonObj["token"].toString());
        UserManager::GetInstance()->SetUserInfo(user_info);

        if(jsonObj.contains("apply_list")){
            UserManager::GetInstance()->AddApplyList(jsonObj["apply_list"].toArray());
        }
        if(jsonObj.contains("friend_list")){
            UserManager::GetInstance()->AddFriendList(jsonObj["friend_list"].toArray());
        }
        emit sig_switch_chatdialog();
    });

    _handlers.insert(ReqID::ID_SEARCH_USER_RSP,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            emit sig_user_search(nullptr);
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Search User Failed, err is " << err ;
            emit sig_user_search(nullptr);
            return;
        }

        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),
                                                        jsonObj["name"].toString(), jsonObj["nick"].toString(),
                                                        jsonObj["desc"].toString(), jsonObj["sex"].toInt(), jsonObj["icon"].toString());
        emit sig_user_search(search_info);
    });

    _handlers.insert(ReqID::ID_ADD_FRIEND_RSP,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Add Friend RSP Failed, err is " << err ;
            return;
        }

        qDebug() << "Add Friend RSP Success" ;
    });

    _handlers.insert(ReqID::ID_NOTIFY_ADD_FRIEND_REQ,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Notify Add Friend Error, err is " << err ;
            return;
        }

        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString desc = jsonObj["desc"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AddFriendApply>(
            from_uid, name, desc,
            icon, nick, sex);
        emit sig_friend_apply(apply_info);
    });

    _handlers.insert(ReqID::ID_AUTH_FRIEND_RSP,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Add User Failed, err is " << err ;
            return;
        }

        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();

        auto rsp = std::make_shared<AuthRsp>(
            from_uid, name, nick,
            icon, sex);
        emit sig_auth_rsp(rsp);
    });

    _handlers.insert(ReqID::ID_NOTIFY_AUTH_FRIEND_REQ,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "auth User Failed, err is " << err ;
            return;
        }

        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AuthInfo>(
            from_uid, name, nick,
            icon, sex);
        emit sig_add_auth_friend(apply_info);
    });

    _handlers.insert(ReqID::ID_TEXT_CHAT_MSG_RSP,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Recvice Text Msg Failed, err is " << err ;
            return;
        }
    });

    _handlers.insert(ReqID::ID_NOTIFY_TEXT_CHAT_MSG_REQ,[this](ReqID id,int len,QByteArray data){
        Q_UNUSED(len);

        qDebug()<<"handle id is "<< id <<" , data is "<< data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<< "Failed to created JsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Notify Recvice Text Msg Failed, err is " << err ;
            return;
        }
        auto msg = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg);
    });

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,[this](ReqID id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        qDebug() << "Receive offline Notify Success, uid is " << uid ;
        //断开连接
        //并且发送通知到界面
        emit sig_notify_offline();
    });

    _handlers.insert(ID_HEART_BEAT_REQ,[this](ReqID id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "JSON Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Heart Beat Time Out, err is " << err;
            return;
        }
    });
}


void TcpManager::handleMessage(ReqID id, int len, QByteArray data)
{
    auto find_iter =  _handlers.find(id);
    if(find_iter == _handlers.end()){
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
    }

    find_iter.value()(id,len,data);
}

void TcpManager::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "Connect to server ...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toInt());
    _socket.connectToHost(_host,_port);
}

void TcpManager::slot_send_data(ReqID reqid, QByteArray data)
{
    uint16_t id = static_cast<uint16_t>(reqid);

    quint16 len = static_cast<quint16>(data.length());

    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);

    out.setByteOrder(QDataStream::BigEndian);
    out<<id<<len;

    block.append(data);

    _socket.write(block);
    qDebug() << "Send data: "<< block;
}

