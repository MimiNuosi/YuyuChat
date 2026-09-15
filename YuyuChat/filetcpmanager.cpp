#include "filetcpmanager.h"
#include "usermanager.h"



void FileTcpManager::SendData(ReqID id, QByteArray data)
{
    emit sig_send_data(id,data);
}

void FileTcpManager::CloseConnection()
{
    emit sig_tcp_close();
}

void FileTcpManager::SendDownloadInfo(std::shared_ptr<DownloadInfo> download) {
    QJsonObject jsonObj;
    jsonObj["name"] = download->_name;
    jsonObj["seq"] = download->_seq;
    jsonObj["trans_size"] = 0;
    jsonObj["total_size"] = 0;
    jsonObj["token"] = UserManager::GetInstance()->GetToken();
    jsonObj["uid"] = UserManager::GetInstance()->GetUid();
    jsonObj["client_path"] = download->_client_path;

    QJsonDocument doc(jsonObj);
    SendData(ID_DOWN_LOAD_FILE_REQ, doc.toJson(QJsonDocument::Compact));
}

FileTcpManager::FileTcpManager():_host(""),_port(0),_b_recv_pending(false),
    _message_id(0),_message_len(0),_bytes_sent(0),_pending(false),_socket(nullptr)
{
    registerMetaType();

    // 保留线程安全的跨线程信号绑定
    QObject::connect(this, &FileTcpManager::sig_send_data, this, &FileTcpManager::slot_send_data);
    QObject::connect(this, &FileTcpManager::sig_tcp_close, this, &FileTcpManager::slot_tcp_close);

    initHandlers();
}

void FileTcpManager::initHandlers()
{
    _handlers.insert(ID_UPLOAD_HEAD_ICON_RSP, [this](ReqID id, int len, QByteArray data){
        Q_UNUSED(len);

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "[文件上传] JSON解析失败";
            return;
        }

        QJsonObject recvObj = jsonDoc.object();
        if (!recvObj.contains("error") || recvObj["error"].toInt() != ErrorCodes::SUCCESS) {
            int err = recvObj.value("error").toInt(ErrorCodes::ERR_JSON);
            qDebug() << "[文件上传] 服务端返回错误码:" << err;
            //1. 增加错误回抛通知
            //emit sig_upload_failed(recvObj["md5"].toString(), err);
            return;
        }

        auto md5 = recvObj["md5"].toString();
        auto seq = recvObj["seq"].toInt();
        qint64 trans_size = recvObj["trans_size"].toVariant().toLongLong();
        qint64 total_size = recvObj["total_size"].toVariant().toLongLong();
        auto uid = recvObj["uid"].toInt();
        auto name = recvObj["name"].toString();

        // 2. 进度回调通知 UI
        //emit sig_update_upload_progress(md5, trans_size, total_size);

        // 3. 上传完成判定与状态闭环
        if (trans_size >= total_size) {
            qDebug() << "[文件上传] 上传成功完毕, MD5:" << md5;
            //emit sig_reset_label_icon(md5); // 通知界面刷新头像
            return;
        }

        auto file_info = UserManager::GetInstance()->GetUploadInfoByName(name);
        if (!file_info) {
            qWarning() << "[文件上传] 未找到对应的本地文件记录:" << md5;
            return;
        }

        // 4. 打开文件并定位到服务端已确认接收的偏移位置
        QFile file(file_info->filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "[文件上传] 无法读取文件:" << file.errorString();
            return;
        }

        file.seek(trans_size);
        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close(); // 读取当前切片后释放

        if (buffer.isEmpty()) return;

        seq++;
        qint64 next_trans_size = trans_size + buffer.size();

        // 5. 组织发送下一个分片
        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["name"] = file_info->fileName();
        sendObj["seq"] = seq;
        sendObj["trans_size"] = next_trans_size;
        sendObj["total_size"] = total_size;
        sendObj["last"] = (next_trans_size >= total_size) ? 1 : 0;
        sendObj["data"] = QString::fromUtf8(buffer.toBase64());
        sendObj["last_seq"] = recvObj["last_seq"].toInt();
        sendObj["uid"] = uid;

        QJsonDocument doc(sendObj);
        SendData(ID_UPLOAD_HEAD_ICON_REQ, doc.toJson(QJsonDocument::Compact));
    });

    _handlers.insert(ID_DOWN_LOAD_FILE_RSP, [this](ReqID id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) return;

        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            const QString failedName = jsonObj["name"].toString();
            qWarning() << "[文件下载] 服务端返回错误:" << jsonObj["error"].toInt()
                       << " name=" << failedName;
            if (!failedName.isEmpty()) {
                UserManager::GetInstance()->RmvDownloadFile(failedName);
            }
            return;
        }

        QString name = jsonObj["name"].toString();
        int seq = jsonObj["seq"].toInt();
        bool is_last = jsonObj["is_last"].toBool();
        qint64 total_size = jsonObj["total_size"].toString().toLongLong();
        qint64 current_size = jsonObj["current_size"].toString().toLongLong();
        QString base64Data = jsonObj["data"].toString();

        auto file_info = UserManager::GetInstance()->GetDownloadInfo(name);
        if (!file_info) {
            qWarning() << "[文件下载] 未找到下载任务:" << name;
            return;
        }

        const QString clientPath = file_info->_client_path;
        file_info->_current_size = current_size;
        file_info->_total_size = total_size;

        // 解码并写入本地文件
        QByteArray decodedData = QByteArray::fromBase64(base64Data.toUtf8());
        QFile file(clientPath);
        QIODevice::OpenMode mode = (seq == 1) ? QIODevice::WriteOnly : (QIODevice::WriteOnly | QIODevice::Append);

        if (file.open(mode)) {
            file.write(decodedData);
            file.close();
        } else {
            qWarning() << "[文件下载] 无法写入文件:" << clientPath;
            return;
        }

        if (is_last) {
            qDebug() << "[文件下载] 文件接收完毕:" << clientPath;
            UserManager::GetInstance()->RmvDownloadFile(name);

            if (name.startsWith("head_")) {
                emit sig_reset_label_icon(clientPath); // 头像下载完，刷新头像
            } else {
                // 聊天图片下载完，通过消息缓存拿到 msg_id 并精准驱动气泡
                auto trans_file = UserManager::GetInstance()->GetTransFileByName(name);
                int msg_id = trans_file ? trans_file->_msg_id : 0;
                emit sig_download_img_finished(msg_id, clientPath);
            }
        } else {
            // 中间包：刷新下载进度条
            auto trans_file = UserManager::GetInstance()->GetTransFileByName(name);
            if (trans_file) {
                emit sig_update_img_progress(trans_file->_msg_id, current_size, total_size);
            }
            // 请求下一分片
            file_info->_seq = seq + 1;
            SendDownloadInfo(file_info);
        }
    });

    _handlers.insert(ID_IMG_CHAT_UPLOAD_RSP, [this](ReqID id, int len, QByteArray data) {
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) return;

        QJsonObject recvObj = jsonDoc.object();
        if (recvObj.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
            qWarning() << "[图片上传] 服务端报错:" << recvObj["error"].toInt();
            return;
        }

        auto name = recvObj["name"].toString();
        auto md5 = recvObj["md5"].toString();
        auto seq = recvObj["seq"].toInt();
        qint64 trans_size = recvObj["trans_size"].toVariant().toLongLong();
        qint64 total_size = recvObj["total_size"].toVariant().toLongLong();

        // 1. 从 UserManager 获取对应发送任务
        auto file_info = UserManager::GetInstance()->GetTransFileByName(name);
        if (!file_info) return;

        // 2. 更新内存对象已传输大小，并向外发射进度信号刷新 PictureBubble
        file_info->_current_size = trans_size;
        emit sig_update_img_progress(file_info->_msg_id, trans_size, total_size);

        // 3. 传输完成判定
        if (trans_size >= total_size) {
            qDebug() << "[图片上传] 资源传输完毕，落盘成功:" << name;
            UserManager::GetInstance()->RmvTransFileByName(name);
            return;
        }

        // 4. 读取下一个 32KB 分片继续上推
        QFile file(file_info->_text_or_url);
        if (!file.open(QIODevice::ReadOnly)) return;

        file.seek(trans_size);
        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close();

        if (buffer.isEmpty()) return;

        qint64 next_trans_size = trans_size + buffer.size();

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["name"] = name;
        sendObj["seq"] = seq + 1;
        sendObj["trans_size"] = QString::number(next_trans_size);
        sendObj["total_size"] = QString::number(total_size);
        sendObj["last"] = (next_trans_size >= total_size) ? 1 : 0;
        sendObj["data"] = QString::fromUtf8(buffer.toBase64());
        sendObj["uid"] = UserManager::GetInstance()->GetUid();
        sendObj["token"] = UserManager::GetInstance()->GetToken();

        QJsonDocument doc(sendObj);
        SendData(ReqID::ID_IMG_CHAT_UPLOAD_REQ, doc.toJson(QJsonDocument::Compact));
    });
}

void FileTcpManager::handleMessage(ReqID id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end()) {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}

void FileTcpManager::registerMetaType()
{
    qRegisterMetaType<ServerInfo>("ServerInfo");
    qRegisterMetaType<std::shared_ptr<ServerInfo>>("std::shared_ptr<ServerInfo>");
    qRegisterMetaType<SearchInfo>("SearchInfo");
    qRegisterMetaType<std::shared_ptr<SearchInfo>>("std::shared_ptr<SearchInfo>");

    qRegisterMetaType<AddFriendApply>("AddFriendApply");
    qRegisterMetaType<std::shared_ptr<AddFriendApply>>("std::shared_ptr<AddFriendApply>");

    qRegisterMetaType<ApplyInfo>("ApplyInfo");

    qRegisterMetaType<std::shared_ptr<AuthInfo>>("std::shared_ptr<AuthInfo>");

    qRegisterMetaType<AuthRsp>("AuthRsp");
    qRegisterMetaType<std::shared_ptr<AuthRsp>>("std::shared_ptr<AuthRsp>");

    qRegisterMetaType<UserInfo>("UserInfo");

    qRegisterMetaType<std::vector<std::shared_ptr<TextChatData>>>("std::vector<std::shared_ptr<TextChatData>>");

    qRegisterMetaType<std::vector<std::shared_ptr<ChatThreadInfo>>>("std::vector<std::shared_ptr<ChatThreadInfo>>");

    qRegisterMetaType<std::shared_ptr<ChatThreadData>>("std::shared_ptr<ChatThreadData>");
    qRegisterMetaType<ReqID>("ReqID");
    qRegisterMetaType<MsgInfo>("MsgInfo");
    qRegisterMetaType<std::shared_ptr<MsgInfo>>("std::shared_ptr<MsgInfo>");
}

void FileTcpManager::slot_send_data(ReqID reqid, QByteArray data)
{
    uint16_t id = static_cast<uint16_t>(reqid);

    quint16 len = static_cast<quint16>(data.length());

    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);

    out.setByteOrder(QDataStream::BigEndian);
    out<<id<<len;

    block.append(data);

    bool is_connected = _socket && _socket->state() == QAbstractSocket::ConnectedState;

    // socket 未建立连接（首次连接前 / 掉线后），先入队，待连接成功再补发
    if (!is_connected) {
        qDebug() << "[FileTcpManager] socket not connected, queue packet id =" << id
                 << ", queue size =" << (_send_queue.size() + 1);
        _send_queue.enqueue(block);

        // 若之前已获得服务器地址且当前处于未连接状态，立即发起(重)连接
        if (_socket && _socket->state() == QAbstractSocket::UnconnectedState && !_host.isEmpty()) {
            qDebug() << "[FileTcpManager] auto connect before send ->" << _host << ":" << _port;
            _socket->connectToHost(_host, _port);
        }
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
    if (id == ID_HEART_BEAT_REQ) {
        qDebug() << "[FileTcpManager] 发送心跳 write() 返回" << written << " 包长=" << _current_block.size();
    }
    if (written < 0) {
        qWarning() << "[FileTcpManager] write() failed:" << _socket->errorString();
        _pending = false;
    }
}

void FileTcpManager::flushSendQueue()
{
    if (_pending) {
        return;
    }

    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    if (_send_queue.isEmpty()) {
        return;
    }

    _current_block = _send_queue.dequeue();
    _bytes_sent = 0;
    _pending = true;
    qint64 written = _socket->write(_current_block);
    qDebug() << "[FileTcpManager] flushSendQueue write() returned" << written;
    if (written < 0) {
        qWarning() << "[FileTcpManager] flushSendQueue write() failed:" << _socket->errorString();
        _pending = false;
    }
}

void FileTcpManager::slot_tcp_connect(std::shared_ptr<ServerInfo> si)
{
    // 🌟 如果 socket 不存在，在当前线程（_file_tcp_thread）动态创建并挂载到 this
    if (!_socket) {
        initSocketHandlers();
    }

    _host = si->_res_host;
    _port = static_cast<uint16_t>(si->_res_port.toUInt());
    qDebug() << "[FileTcpManager] 发起长连接 ->" << _host << ":" << _port;

    // 重新登录/再次拿到服务器地址后，确保心跳与重连定时器处于运行状态
    if (_heart_timer && !_heart_timer->isActive()) {
        _heart_timer->start();
    }

    if (_socket->state() == QAbstractSocket::UnconnectedState) {
        _socket->connectToHost(_host, _port);
    }
}

void FileTcpManager::initSocketHandlers()
{
    _socket = new QTcpSocket(this);

    connect(_socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "[FileTcpManager] Connected to server!";
        // 连接成功：启动心跳保活，并把断线期间积压的数据(如上传分片)补发出去
        if (_heart_timer) {
            _heart_timer->start();
        }
        emit sig_con_success(true);
        flushSendQueue();
    });

    connect(_socket, &QTcpSocket::readyRead, this, [this]() {
        _buffer.append(_socket->readAll());

        forever {
            if (!_b_recv_pending) {
                if (_buffer.size() < FILE_UPLOAD_HEAD_LEN) {
                    return;
                }

                QDataStream stream(_buffer);
                stream.setVersion(QDataStream::Qt_5_0);
                stream >> _message_id >> _message_len;

                _buffer.remove(0, FILE_UPLOAD_HEAD_LEN);
                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            }

            if (_buffer.size() < _message_len) {
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody;

            _buffer = _buffer.mid(_message_len);
            handleMessage(ReqID(_message_id), _message_len, messageBody);
        }
    });

    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this,
        [this](QAbstractSocket::SocketError socketError) {
            Q_UNUSED(socketError)
            qDebug() << "[FileTcpManager] Error:" << _socket->errorString();
        });

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "[FileTcpManager] Disconnected from server.";

        // 在途数据放回队首，待重连成功后补发
        if (!_current_block.isEmpty()) {
            _send_queue.prepend(_current_block);
            _current_block.clear();
        }
        _bytes_sent = 0;
        _pending = false;

        emit sig_connection_closed();

        // 心跳定时器保持运行：若连接意外断开，下一个心跳周期会自动重连
        if (_socket->state() == QAbstractSocket::UnconnectedState && !_host.isEmpty()) {
            qDebug() << "[FileTcpManager] auto reconnect after disconnect";
            _socket->connectToHost(_host, _port);
        }
    });

    connect(_socket, &QTcpSocket::bytesWritten, this, [this](qint64 bytes) {
        _bytes_sent += bytes;
        if (_bytes_sent < _current_block.size()) {
            auto data_to_send = _current_block.mid(_bytes_sent);
            _socket->write(data_to_send);
            return;
        }

        if (_send_queue.isEmpty()) {
            _current_block.clear();
            _pending = false;
            _bytes_sent = 0;
            return;
        }

        _current_block = _send_queue.dequeue();
        _bytes_sent = 0;
        _pending = true;
        qint64 w2 = _socket->write(_current_block);
        qDebug() << "[TcpMgr] Dequeued and write() returned" << w2;
    });

    // 🌟 心跳定时器：每 10s 发一次心跳，保证 ResourceServer 侧会话不被 60s 超时回收。
    // 定时器在取得服务器地址后立即启动；未连接时由它周期重连，已连接时发心跳保活。
    _heart_timer = new QTimer(this);
    _heart_timer->setInterval(10 * 1000);
    connect(_heart_timer, &QTimer::timeout, this, [this]() {
        if (_socket && _socket->state() == QAbstractSocket::ConnectedState) {
            // 心跳无需业务数据，空 JSON 即可触发服务端刷新保活时间
            qDebug() << "[FileTcpManager] 心跳定时器触发，当前状态=Connected，发送心跳";
            QJsonObject jsonObj;
            QJsonDocument doc(jsonObj);
            SendData(ReqID::ID_HEART_BEAT_REQ, doc.toJson(QJsonDocument::Compact));
        }
        else if (_socket && _socket->state() == QAbstractSocket::UnconnectedState && !_host.isEmpty()) {
            // 掉线后由心跳周期重连，连接成功后会自动补发积压数据
            qDebug() << "[FileTcpManager] heartbeat tries to reconnect" << _host << ":" << _port;
            _socket->connectToHost(_host, _port);
        }
        else {
            qDebug() << "[FileTcpManager] 心跳定时器触发，但 socket 不在 Connected 状态，当前 state ="
                     << (_socket ? _socket->state() : -1);
        }
    });
    _heart_timer->start();
    qDebug() << "[FileTcpManager] 心跳定时器已启动(10s)";
}

void FileTcpManager::slot_tcp_close()
{
    if (_heart_timer) {
        _heart_timer->stop();
    }

    if (_socket) {
        _socket->close();
    }

    // 主动关闭时清空积压数据，避免重连后发送过期内容
    _send_queue.clear();
    _current_block.clear();
    _bytes_sent = 0;
    _pending = false;
}

FileTcpThread::FileTcpThread()
{
    FileTcpManager::GetInstance()->moveToThread(&_file_tcp_thread);
    _file_tcp_thread.start();
}

FileTcpThread::~FileTcpThread()
{
    _file_tcp_thread.quit();
    _file_tcp_thread.wait();
}
