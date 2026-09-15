#include "tcpmanager.h"
#include <QAbstractSocket>
#include "usermanager.h"
#include "filetcpmanager.h"
TcpManager::~TcpManager(){

}

void TcpManager::CloseConnection(){
    if (_socket) {
        _socket->close();
    }
}

TcpManager::TcpManager() :_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0),_bytes_sent(0),_pending(false)
{
    registerMetaType();

    // 自连接：跨线程触发 slot_send_data（socket 实际发送在 _tcp_thread 中完成）
    QObject::connect(this, &TcpManager::sig_send_data, this, &TcpManager::slot_send_data);

    initHandlers();
}

// socket 不再作为值成员在构造时(main线程)创建，而是在 slot_tcp_connect
// 首次调用时、于 _tcp_thread 工作线程内以堆指针创建。这样 QTcpSocket 的线程
// 亲和性属于工作线程，connectToHost/write 全部在同一线程执行，
// 避免 “QObject: Cannot create children for a parent that is in a different thread” 警告。
void TcpManager::initSocketHandlers()
{
    _socket = new QTcpSocket(this);

    connect(_socket, &QTcpSocket::connected, this, [this]() {
        if (_heart_timer) {
            _heart_timer->start(10000);
        }
        qDebug() << "[TcpMgr] Connected to ChatServer";
        emit sig_con_success(true);
    });

    connect(_socket, &QTcpSocket::readyRead, this, [this]() {
        _buffer.append(_socket->readAll());
        qDebug() << "[网络日志] 收到数据，当前缓冲区总长度: " << _buffer.size();

        forever{
            QDataStream stream(&_buffer, QIODevice::ReadOnly);
            stream.setVersion(QDataStream::Qt_5_0);
            stream.setByteOrder(QDataStream::BigEndian);

            if (!_b_recv_pending) {
                if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
                    qDebug() << "[网络日志] 数据不足包头长度(4字节)，继续等待...";
                    return;
                }

                stream >> _message_id >> _message_len;
                _buffer = _buffer.mid(sizeof(quint16) * 2);

                qDebug() << "[网络日志] 解析出包头 -> 消息ID: " << _message_id << ", 负载长度: " << _message_len;
            }

            if (_buffer.size() < _message_len) {
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

    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this,
        [this](QAbstractSocket::SocketError socketError) {
            Q_UNUSED(socketError)
            qDebug() << "[TcpMgr] Error:" << _socket->errorString();
        });

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        if (_heart_timer) {
            _heart_timer->stop();
        }
        qDebug() << "[TcpMgr] Disconnected from ChatServer.";
        emit sig_connection_close();
    });

    connect(_socket, &QTcpSocket::bytesWritten, this, [this](qint64 bytes) {
        //更新发送数据
        _bytes_sent += bytes;
        //未发送完整
        if (_bytes_sent < _current_block.size()) {
            //继续发送
            auto data_to_send = _current_block.mid(_bytes_sent);
            _socket->write(data_to_send);
            return;
        }

        //发送完全，则查看队列是否为空
        if (_send_queue.isEmpty()) {
            //队列为空，说明已经将所有数据发送完成，将pending设置为false，这样后续要发送数据时可以继续发送
            _current_block.clear();
            _pending = false;
            _bytes_sent = 0;
            return;
        }

        //队列不为空，则取出队首元素
        _current_block = _send_queue.dequeue();
        _bytes_sent = 0;
        _pending = true;
        qint64 w2 = _socket->write(_current_block);
        qDebug() << "[TcpMgr] Dequeued and write() returned" << w2;
    });
}

void TcpManager::initHandlers()
{
    _handlers.insert(ReqID::ID_CHAT_LOGIN_RSP,[this](ReqID id,int len,QByteArray data){
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

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto send_uid = data["sender"].toInt();
            auto msg_id = data["msg_id"].toInt();
            auto thread_id = data["thread_id"].toInt();
            auto unique_id = data["unique_id"].toInt();
            auto msg_content = data["msg_content"].toString();
            auto chat_data = std::make_shared<TextChatData>(msg_id, thread_id, ChatFormType::PRIVATE,
                                                            ChatMsgType::TEXT, msg_content, send_uid,0,"");
            chat_datas.push_back(chat_data);
        }

        auto rsp = std::make_shared<AuthRsp>(
            from_uid, name, nick,
            icon, sex);
        rsp->SetChatDatas(chat_datas);
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
        int thread_id = jsonObj.contains("thread_id") ? jsonObj["thread_id"].toInt() : 0;

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto send_uid = data["sender"].toInt();
            auto msg_id = data["msg_id"].toInt();
            auto thread_id = data["thread_id"].toInt();
            auto unique_id = data["unique_id"].toInt();
            auto msg_content = data["msg_content"].toString();
            auto chat_data = std::make_shared<TextChatData>(msg_id, thread_id, ChatFormType::PRIVATE,
                                                            ChatMsgType::TEXT, msg_content, send_uid, 0 , "");
            chat_datas.push_back(chat_data);
        }

        auto apply_info = std::make_shared<AuthInfo>(
            from_uid, name, nick,
            icon, sex);
        apply_info->SetChatDatas(chat_datas);
        apply_info->_thread_id = thread_id;

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

        auto thread_id = jsonObj["thread_id"].toInt();
        auto sender = jsonObj["fromuid"].toInt();


        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto msg_id = data["message_id"].toInt();
            auto unique_id = data["unique_id"].toString();
            auto msg_content = data["content"].toString();
            QString chat_time = data["chat_time"].toString();
            int status = data["status"].toInt();
            auto chat_data = std::make_shared<TextChatData>(msg_id,unique_id, thread_id, ChatFormType::PRIVATE,
                                                            ChatMsgType::TEXT, msg_content, sender, status, chat_time);
            chat_datas.push_back(chat_data);
        }

        //发送信号通知界面
        emit sig_chat_msg_rsp(thread_id, chat_datas);
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

        auto sender = jsonObj["fromuid"].toInt();
        auto thread_id = UserManager::GetInstance()->GetThreadIdByUid(sender);

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        QJsonArray msgArray = jsonObj.contains("text_array") ? jsonObj["text_array"].toArray() : jsonObj["chat_datas"].toArray();

        for (const QJsonValue& val : msgArray) {
            QJsonObject obj = val.toObject();
            auto msg_id = obj.contains("message_id") ? obj["message_id"].toInt() : 0;
            // 服务端字段为 msgid
            auto unique_id = obj.contains("msgid") ? obj["msgid"].toString() : obj["unique_id"].toString();
            auto msg_content = obj["content"].toString();
            QString chat_time = obj["chat_time"].toString();
            int status = obj["status"].toInt();

            auto chat_data = std::make_shared<TextChatData>(
                msg_id, unique_id, thread_id, ChatFormType::PRIVATE,
                ChatMsgType::TEXT, msg_content, sender, status, chat_time
                );
            chat_datas.push_back(chat_data);
        }
        emit sig_text_chat_msg(chat_datas);
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

    _handlers.insert(ID_LOAD_CHAT_THREAD_RSP, [this](ReqID id, int len, QByteArray data) {
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
            qDebug() << "chat thread json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get chat thread rsp failed, error is " << err;
            return;
        }

        qDebug() << "Receive chat thread rsp Success";

        auto thread_array = jsonObj["threads"].toArray();
        std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads;
        for (const QJsonValue& value : thread_array) {
            auto cti = std::make_shared<ChatThreadInfo>();
            cti->_thread_id = value["thread_id"].toVariant().toLongLong();
            cti->_user1_id  = value["user1_id"].toVariant().toLongLong();
            cti->_type = value["type"].toString();
            cti->_user1_id = value["user1_id"].toVariant().toLongLong();
            cti->_user2_id = value["user2_id"].toVariant().toLongLong();
            chat_threads.push_back(cti);
        }

        bool load_more = jsonObj["load_more"].toBool();
        qint64 next_last_id = jsonObj["next_last_id"].toVariant().toLongLong();
        //发送信号通知界面
        emit sig_load_chat_thread(load_more, next_last_id, chat_threads);
    });

    _handlers.insert(ID_CREATE_PRIVATE_CHAT_RSP, [this](ReqID id, int len, QByteArray data) {
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
            qDebug() << "parse create private chat json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get create private chat failed, error is " << err;
            return;
        }

        qDebug() << "Receive create private chat rsp Success";

        int uid = jsonObj["uid"].toInt();
        int other_id = jsonObj["other_id"].toInt();
        int thread_id = jsonObj["thread_id"].toInt();

        //发送信号通知界面
        emit sig_create_private_chat(uid, other_id, thread_id);
    });

    _handlers.insert(ID_LOAD_CHAT_MSG_RSP, [this](ReqID id, int len, QByteArray data) {
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
            qDebug() << "load chat msg json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "oad chat msgt failed, error is " << err;
            return;
        }

        int thread_id = jsonObj["thread_id"].toInt();
        int last_msg_id = jsonObj["last_message_id"].toInt();
        bool load_more = jsonObj["load_more"].toBool();

        QJsonArray msgArray = jsonObj.contains("chat_datas") ? jsonObj["chat_datas"].toArray() : jsonObj["messages"].toArray();

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : msgArray) {
            auto send_uid = data["sender"].toInt();
            auto msg_id = data["msg_id"].toInt();
            auto thread_id = data["thread_id"].toInt();
            auto unique_id = data["unique_id"].toString();
            QString msg_content = data["msg_content"].toString();
            QString chat_time = data["chat_time"].toString();
            auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id, ChatFormType::PRIVATE,
                                                            ChatMsgType::TEXT, msg_content, send_uid, 2,chat_time);
            chat_datas.push_back(chat_data);
        }

        emit sig_load_chat_msg(thread_id, last_msg_id, load_more, chat_datas);
    });

_handlers.insert(ID_IMG_CHAT_MSG_RSP, [this](ReqID id, int len, QByteArray data) {
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
            qDebug() << "parse create private chat json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get create private chat failed, error is " << err;
            return;
        }

        qDebug() << "Receive create private chat rsp Success";

        //收到消息后转发给页面
        auto thread_id = jsonObj["thread_id"].toInt();
        auto unique_id = jsonObj["unique_id"].toString();
        auto unique_name = jsonObj["unique_name"].toString();

        auto sender = jsonObj["fromuid"].toInt();
        auto msg_id = jsonObj["message_id"].toInt();
        QString chat_time = jsonObj["chat_time"].toString();
        int status = jsonObj["status"].toInt();

        auto file_info = UserManager::GetInstance()->GetTransFileByName(unique_name);

        auto chat_data = std::make_shared<ImgChatData>(file_info, unique_id, thread_id, ChatFormType::PRIVATE,
            ChatMsgType::PIC, sender, status, chat_time);

        //发送信号通知界面
        emit sig_chat_img_rsp(thread_id, chat_data);

        QFile file(file_info->_text_or_url);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file:" << file.errorString();
            return;
        }

        file.seek(file_info->_current_size);
        auto buffer = file.read(MAX_FILE_LEN);
        qDebug() << "buffer is " << buffer;
        //将文件内容转换为base64编码
        QString base64Data = buffer.toBase64();
        QJsonObject file_obj;
        file_obj["name"] = file_info->_unique_name;
        file_obj["unique_id"] = unique_id;
        file_obj["seq"] = file_info->_seq;
        file_info->_current_size = buffer.size() + (file_info->_seq - 1) * MAX_FILE_LEN;
        file_obj["trans_size"] = file_info->_current_size;
        file_obj["total_size"] = file_info->_total_size;
        file_obj["token"] = UserManager::GetInstance()->GetToken();
        file_obj["md5"] = file_info->_md5;
        file_obj["uid"] = UserManager::GetInstance()->GetUid();
        file_obj["data"] = base64Data;

        if (buffer.size() + (file_info->_seq - 1) * MAX_FILE_LEN >= file_info->_total_size) {
            file_obj["last"] = 1;
        }
        else {
            file_obj["last"] = 0;
        }

        //发送文件  todo 留作以后收到服务器返回消息后再发送
        QJsonDocument doc_file(file_obj);
        QByteArray fileData = doc_file.toJson(QJsonDocument::Compact);

        //发送消息给ResourceServer
        FileTcpManager::GetInstance()->SendData(ReqID::ID_IMG_CHAT_UPLOAD_REQ, fileData);
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

void TcpManager::registerMetaType()
{
    // 注册基础枚举与连接配置
    qRegisterMetaType<ReqID>("ReqID");
    qRegisterMetaType<ServerInfo>("ServerInfo");

    // 注册自定义数据类型及其智能指针
    qRegisterMetaType<SearchInfo>("SearchInfo");
    qRegisterMetaType<std::shared_ptr<SearchInfo>>("std::shared_ptr<SearchInfo>");

    qRegisterMetaType<AddFriendApply>("AddFriendApply");
    qRegisterMetaType<std::shared_ptr<AddFriendApply>>("std::shared_ptr<AddFriendApply>");

    qRegisterMetaType<ApplyInfo>("ApplyInfo");
    qRegisterMetaType<std::shared_ptr<ApplyInfo>>("std::shared_ptr<ApplyInfo>");

    qRegisterMetaType<AuthInfo>("AuthInfo");
    qRegisterMetaType<std::shared_ptr<AuthInfo>>("std::shared_ptr<AuthInfo>");

    qRegisterMetaType<AuthRsp>("AuthRsp");
    qRegisterMetaType<std::shared_ptr<AuthRsp>>("std::shared_ptr<AuthRsp>");

    qRegisterMetaType<UserInfo>("UserInfo");
    qRegisterMetaType<std::shared_ptr<UserInfo>>("std::shared_ptr<UserInfo>");

    qRegisterMetaType<ChatDataBase>("ChatDataBase");
    qRegisterMetaType<std::shared_ptr<ChatDataBase>>("std::shared_ptr<ChatDataBase>");

    qRegisterMetaType<TextChatData>("TextChatData");
    qRegisterMetaType<std::shared_ptr<TextChatData>>("std::shared_ptr<TextChatData>");

    qRegisterMetaType<ChatThreadInfo>("ChatThreadInfo");
    qRegisterMetaType<std::shared_ptr<ChatThreadInfo>>("std::shared_ptr<ChatThreadInfo>");

    qRegisterMetaType<ChatThreadData>("ChatThreadData");
    qRegisterMetaType<std::shared_ptr<ChatThreadData>>("std::shared_ptr<ChatThreadData>");

    qRegisterMetaType<ImgChatData>("ImgChatData");
    qRegisterMetaType<std::shared_ptr<ImgChatData>>("std::shared_ptr<ImgChatData>");

    // 注册跨线程信号使用的 vector 集合
    qRegisterMetaType<std::vector<std::shared_ptr<TextChatData>>>("std::vector<std::shared_ptr<TextChatData>>");
    qRegisterMetaType<std::vector<std::shared_ptr<ChatThreadInfo>>>("std::vector<std::shared_ptr<ChatThreadInfo>>");
}

void TcpManager::slot_tcp_connect(std::shared_ptr<ServerInfo> si)
{
    if (!_socket) {
        // socket 在本线程(_tcp_thread)中创建，使线程亲和性与本对象一致
        initSocketHandlers();
    }

    if (!_heart_timer) {
        _heart_timer = new QTimer(this);
        connect(_heart_timer, &QTimer::timeout, this, [this]() {
            auto user_info = UserManager::GetInstance()->GetUserInfo();
            if (!user_info) return;

            QJsonObject jsonObj;
            jsonObj["fromuid"] = user_info->_uid;
            QJsonDocument doc(jsonObj);
            emit sig_send_data(ReqID::ID_HEART_BEAT_REQ, doc.toJson(QJsonDocument::Compact));
        });
    }
    _host = si->_chat_host;
    _port = static_cast<uint16_t>(si->_chat_port.toUInt());

    if (_socket->state() == QAbstractSocket::UnconnectedState) {
        _socket->connectToHost(_host, _port);
    }
    qDebug() << "[网络路由] 准备连接 ChatServer，目标 IP:" << _host << " 目标端口:" << _port;
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

    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState) {
        // 连接未就绪时丢弃，避免对空 socket 写入触发崩溃/警告
        qWarning() << "[TcpMgr] socket not connected, drop packet id =" << id;
        _current_block.clear();
        _bytes_sent = 0;
        _pending = false;
        return;
    }

    //判断是否正在发送
    if (_pending) {
        //放入队列直接返回，因为目前有数据正在发送
        _send_queue.enqueue(block);
        return;
    }

    // 没有正在发送，把这包设为“当前块”，重置计数，并写出去
    _current_block = block;        // ← 保存当前正在发送的 block
    _bytes_sent = 0;            // ← 归零
    _pending = true;         // ← 标记正在发送

    qint64 written = _socket->write(_current_block);
    if (written < 0) {
        qWarning() << "[TcpMgr] write() failed:" << _socket->errorString();
        _pending = false;
    }
}

TcpThread::TcpThread()
{
    TcpManager::GetInstance()->moveToThread(&_tcp_thread);
    _tcp_thread.start();
}

TcpThread::~TcpThread()
{
    _tcp_thread.quit();
    _tcp_thread.wait();
}
