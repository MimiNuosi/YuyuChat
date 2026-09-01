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

FileTcpManager::FileTcpManager():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0),_bytes_sent(0),_pending(false)
{
    registerMetaType();
    QObject::connect(&_socket, &QTcpSocket::connected, this, [&]() {
        qDebug() << "Connected to server!";
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, this, [&]() {
        // 当有数据可读时，读取所有数据
        // 读取所有数据并追加到缓冲区
        _buffer.append(_socket.readAll());

        forever{
            //先解析头部
            if (!_b_recv_pending) {
                // 检查缓冲区中的数据是否足够解析出一个消息头（消息ID + 消息长度）
                if (_buffer.size() < FILE_UPLOAD_HEAD_LEN) {
                    return; // 数据不够，等待更多数据
                }

                // 每次都重新创建stream
                QDataStream stream(_buffer);
                stream.setVersion(QDataStream::Qt_5_0);
                stream >> _message_id >> _message_len;

                _buffer.remove(0, FILE_UPLOAD_HEAD_LEN);  // 使用remove代替mid赋值

                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;

            }

            //buffer剩余长读是否满足消息体长度，不满足则退出继续等待接受
            if (_buffer.size() < _message_len) {
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;
            // 读取消息体
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody;

            _buffer = _buffer.mid(_message_len);
            handleMessage(ReqID(_message_id),_message_len, messageBody);
        }

    });

    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
          Q_UNUSED(socketError)
          qDebug() << "Error:" << _socket.errorString();
     });

    QObject::connect(this, &FileTcpManager::sig_send_data, this, &FileTcpManager::slot_send_data);

    QObject::connect(this, &FileTcpManager::sig_tcp_close, this, &FileTcpManager::slot_tcp_close);

    QObject::connect(&_socket, &QTcpSocket::disconnected, this, [&]() {
        qDebug() << "Disconnected from server.";
        emit sig_connection_closed();
    });

    QObject::connect(&_socket, &QTcpSocket::bytesWritten, this, [this](qint64 bytes) {
        //更新发送数据
        _bytes_sent += bytes;
        //未发送完整
        if (_bytes_sent < _current_block.size()) {
            //继续发送
            auto data_to_send = _current_block.mid(_bytes_sent);
            _socket.write(data_to_send);
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
        qint64 w2 = _socket.write(_current_block);
        qDebug() << "[TcpMgr] Dequeued and write() returned" << w2;
    });

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

    qint64 written = _socket.write(_current_block);
}

void FileTcpManager::slot_tcp_connect(std::shared_ptr<ServerInfo> si)
{
    _host = si->_res_host;
    _port = static_cast<uint16_t>(si->_res_port.toUInt());
    _socket.connectToHost(_host, _port);
}

void FileTcpManager::slot_tcp_close()
{
    _socket.close();
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
