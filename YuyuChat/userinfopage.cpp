#include "userinfopage.h"
#include "ui_userinfopage.h"
#include "usermanager.h"
#include "filetcpmanager.h"
#include "imagecropperdialog.h"
#include "tcpmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

UserInfoPage::UserInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserInfoPage)
{
    ui->setupUi(this);

    auto icon = UserManager::GetInstance()->GetIcon();
    QPixmap pixmap = Utils::GetAvatarPixmap(icon);
    QPixmap scaledPixmap = pixmap.scaled(ui->head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_label->setPixmap(scaledPixmap);
    ui->head_label->setScaledContents(true);

    // 获取基本信息
    auto sex = UserManager::GetInstance()->GetSex();
    auto name = UserManager::GetInstance()->GetName();
    auto desc = UserManager::GetInstance()->GetDesc();
    QString sexStr = (sex == 1) ? "男" : "女";
    ui->sex_edit->setText(sexStr);
    ui->name_edit->setText(name);
    ui->desc_edit->setText(desc);

    connect(ui->up_button, &QPushButton::clicked, this, &UserInfoPage::slot_upload_avatar);
    connect(ui->submit_button, &QPushButton::clicked, this, &UserInfoPage::on_submit_button_clicked);
}

UserInfoPage::~UserInfoPage()
{
    delete ui;
}

QString UserInfoPage::generateUniqueIconName() {
    return QString("head_%1_%2.png")
    .arg(UserManager::GetInstance()->GetUid())
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

// 点击上传：裁剪、本地保存、并发出第 1 个分片
void UserInfoPage::slot_upload_avatar()
{
    // 1. 选择图片文件
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)")
        );
    if (filename.isEmpty())
        return;

    // 2. 加载图片
    QPixmap inputImage;
    if (!inputImage.load(filename)) {
        QMessageBox::critical(
            this,
            tr("错误"),
            tr("加载图片失败！请确认已部署图片插件。"),
            QMessageBox::Ok
            );
        return;
    }

    // 3. 圆形裁剪并更新到当前 UI
    QPixmap image = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (image.isNull())
        return;

    QPixmap scaledPixmap = image.scaled(ui->head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_label->setPixmap(scaledPixmap);
    ui->head_label->setScaledContents(true);

    // 4. 构建用户私有头像目录: AppDataLocation/user/<uid>/avatars/
    QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto uid = UserManager::GetInstance()->GetUid();
    QDir dir(storageDir + "/user/" + QString::number(uid) + "/avatars");
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建存储目录，请检查权限。"));
        return;
    }

    // 5. 生成唯一文件名并保存为 PNG
    QString uniqueFileName = generateUniqueIconName();
    QString filePath = dir.filePath(uniqueFileName);

    if (!scaledPixmap.save(filePath, "PNG")) {
        QMessageBox::warning(this, tr("保存失败"), tr("头像本地保存失败！"));
        return;
    }
    qDebug() << "头像已保存到：" << filePath;

    // 6. 读取文件计算 MD5
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << file.errorString();
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
        qWarning() << "计算 MD5 失败:" << filePath;
        file.close();
        return;
    }
    QString file_md5 = hash.result().toHex();

    // 7. 读取第 1 个切片数据
    auto fileInfo = std::make_shared<QFileInfo>(filePath);
    qint64 total_size = fileInfo->size();
    int last_seq = (total_size % MAX_FILE_LEN) ? (total_size / MAX_FILE_LEN + 1) : (total_size / MAX_FILE_LEN);

    file.seek(0);
    QByteArray buffer = file.read(MAX_FILE_LEN);
    file.close();

    int seq = 1;
    qint64 trans_size = buffer.size();

    // 8. 构造首包 JSON 协议
    QJsonObject jsonObj;
    jsonObj["md5"] = file_md5;
    jsonObj["name"] = uniqueFileName;
    jsonObj["seq"] = seq;
    jsonObj["trans_size"] = trans_size;
    jsonObj["total_size"] = total_size;
    jsonObj["token"] = UserManager::GetInstance()->GetToken();
    jsonObj["uid"] = uid;
    jsonObj["last"] = (trans_size >= total_size) ? 1 : 0;
    jsonObj["data"] = QString::fromUtf8(buffer.toBase64());
    jsonObj["last_seq"] = last_seq;

    // 9. 登记本地文件映射并发送第 1 个包
    UserManager::GetInstance()->SetIcon(uniqueFileName);
    UserManager::GetInstance()->AddUploadFile(uniqueFileName, fileInfo);

    QJsonDocument doc(jsonObj);
    FileTcpManager::GetInstance()->SendData(ID_UPLOAD_HEAD_ICON_REQ, doc.toJson(QJsonDocument::Compact));

    emit sig_reset_head();
}

void UserInfoPage::on_submit_button_clicked()
{
    QString name = ui->name_edit->text().trimmed();
    QString desc = ui->desc_edit->text().trimmed();
    QString sexStr = ui->sex_edit->text().trimmed();
    int sex = (sexStr == "男") ? 1 : 0;

    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("用户名不能为空！"));
        return;
    }

    auto user_info = UserManager::GetInstance()->GetUserInfo();
    if (!user_info) return;

    // 更新本地内存
    user_info->_name = name;
    user_info->_desc = desc;
    user_info->_sex = sex;

    // 构造修改个人信息的 TCP / HTTP 协议包并发送
    QJsonObject jsonObj;
    jsonObj["uid"] = user_info->_uid;
    jsonObj["name"] = name;
    jsonObj["desc"] = desc;
    jsonObj["sex"] = sex;
    jsonObj["icon"] = UserManager::GetInstance()->GetIcon();

    QJsonDocument doc(jsonObj);
    //emit TcpManager::GetInstance()->sig_send_data(ReqID::ID_UPDATE_USER_INFO_REQ, doc.toJson(QJsonDocument::Compact));

    // 通知界面刷新
    emit sig_reset_head();
    QMessageBox::information(this, tr("成功"), tr("资料已提交更新！"));
}