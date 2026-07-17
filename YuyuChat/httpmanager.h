#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
class HttpManager :public QObject,public Singleton<HttpManager>,public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager();
    void PostHttpReq(QUrl url,QJsonObject json,ReqID req_id,Modules mod);
private:
    friend class Singleton<HttpManager>;
    HttpManager();
    QNetworkAccessManager _manager;

private slots:
    void slot_http_finish(ReqID id,QString res,ErrorCodes err,Modules mod);
signals:
    void sig_http_finish(ReqID id,QString res,ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqID id,QString res,ErrorCodes err);
    void sig_reset_mod_finish(ReqID id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqID id, QString res, ErrorCodes err);
};

#endif // HTTPMANAGER_H
