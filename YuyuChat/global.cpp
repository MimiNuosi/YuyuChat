#include "global.h"
#include "usermanager.h"
#include "filetcpmanager.h"
#include <QStandardPaths>
#include <QCryptographicHash>
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

QString gate_url_prefix = "";

std::function<QString(QString)> xorString = [](QString input){
    QString result = input;
    int length = input.length();
    length %= 255;
    for(int i=0;i<length;i++){
        result[i] = QChar(static_cast<ushort>(input[i].unicode()^static_cast<ushort>(length)));
    }
    return result;
};

namespace Utils {
bool CheckEmailValid(const QString& email, QString& err_msg) {
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    if (!regex.match(email).hasMatch()) {
        err_msg = "邮箱地址不正确";
        return false;
    }
    return true;
}

bool CheckPassValid(const QString& pass, QString& err_msg) {
    if (pass.length() < 6 || pass.length() > 15) {
        err_msg = "密码长度应为6~15";
        return false;
    }
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    if (!regExp.match(pass).hasMatch()) {
        err_msg = "不能包含非法字符";
        return false;
    }
    return true;
}

bool CheckUserValid(const QString& user, QString& err_msg) {
    if (user.isEmpty()) {
        err_msg = "用户名不能为空";
        return false;
    }
    return true;
}

bool CheckVerifyValid(const QString& verify, QString& err_msg) {
    if (verify.isEmpty()) {
        err_msg = "验证码不能为空";
        return false;
    }
    return true;
}

void LoadAvatarOrDownload(const QString& icon_str, QLabel* target_label) {
    if (!target_label) return;

    // 1. 空或默认头像 (:/res/head_X.jpg)
    if (icon_str.isEmpty()) {
        QPixmap defaultPix(":/res/head_1.jpg");
        target_label->setPixmap(defaultPix.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        target_label->setScaledContents(true);
        return;
    }

    QRegularExpression regex("^:/res/head_(\\d+)\\.jpg$");
    if (regex.match(icon_str).hasMatch()) {
        QPixmap pixmap(icon_str);
        if (!pixmap.isNull()) {
            target_label->setPixmap(pixmap.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            target_label->setScaledContents(true);
            return;
        }
    }

    // 2. 本地私有路径查找
    QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto uid = UserManager::GetInstance()->GetUid();
    QDir avatarsDir(storageDir + "/user/" + QString::number(uid) + "/avatars");
    if (!avatarsDir.exists()) {
        avatarsDir.mkpath(".");
    }

    QString fileName = QFileInfo(icon_str).fileName();
    QString avatarPath = avatarsDir.filePath(fileName);
    QPixmap pixmap(avatarPath);

    // 2.1 本地已存在该图
    if (!pixmap.isNull()) {
        target_label->setPixmap(pixmap.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        target_label->setScaledContents(true);
        return;
    }

    // 2.2 本地缺失：登记回调 QLabel + 贴默认图兜底 + 触发断点拉取
    QPixmap defaultPix(":/res/head_1.jpg");
    target_label->setPixmap(defaultPix.scaled(target_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    target_label->setScaledContents(true);

    UserManager::GetInstance()->AddLabelToReset(avatarPath, target_label);

    if (UserManager::GetInstance()->IsDownLoading(fileName)) {
        return; // 已在下载中，无需重复请求
    }

    auto download_info = std::make_shared<DownloadInfo>();
    download_info->_name = fileName;
    download_info->_current_size = 0;
    download_info->_seq = 1;
    download_info->_total_size = 0;
    download_info->_client_path = avatarPath;

    UserManager::GetInstance()->AddDownloadFile(fileName, download_info);
    FileTcpManager::GetInstance()->SendDownloadInfo(download_info);
}

QString calculateFileHash(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QCryptographicHash hash(QCryptographicHash::Md5);

    // 分块计算哈希，避免大文件占用过多内存
    const qint64 chunkSize = 1024 * 1024; // 1MB
    while (!file.atEnd())
    {
        hash.addData(file.read(chunkSize));
    }
    file.close();

    return hash.result().toHex();
}

QString generateUniqueFileName(const QString& originalName){

    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QFileInfo fileInfo(originalName);
    QString extension = fileInfo.suffix();
    return uuid + (extension.isEmpty() ? "" : "." + extension);
}

}

