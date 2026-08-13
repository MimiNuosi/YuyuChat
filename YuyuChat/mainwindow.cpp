#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "QMessageBox.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _ui_state = LOGIN_UI;
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    connect(_login_dlg,&LoginDialog::switchRegister,this,&MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_switch_chatdialog, this, &MainWindow::SlotSwitchChat);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_notify_offline,this,&MainWindow::SlotOffline);
    connect(TcpManager::GetInstance().get(),&TcpManager::sig_connection_close,this,&MainWindow::SlotConnectionClose);
    //emit TcpManager::GetInstance()->sig_switch_chatdialog();

    _chat_dlg->loadChatList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::offlineLogin()
{
    if(_ui_state == LOGIN_UI){
        return;
    }
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _chat_dlg->hide();
    this->setMaximumSize(300,500);
    this->setMinimumSize(300,500);
    this->resize(300,500);
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_state = LOGIN_UI;
}

void MainWindow::SlotSwitchReg()
{
    _ui_state = REGISTER_UI;
    _reg_dlg = new RegisterDialog(this);

    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    connect(_reg_dlg, &RegisterDialog::signSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    _reg_dlg->show();
}

void MainWindow::SlotSwitchLogin()
{
    _ui_state = LOGIN_UI;
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    _ui_state = RESET_UI;
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

    _reset_dlg->show();
    //注册返回登录信号和槽函数
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    _ui_state = LOGIN_UI;
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    _login_dlg->show();
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    _ui_state = CHAT_UI;
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void MainWindow::SlotOffline()
{

    QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
    TcpManager::GetInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::SlotConnectionClose()
{
    QMessageBox::information(this, "网络异常", "与服务器的连接已断开，请检查网络设置！");
    TcpManager::GetInstance()->CloseConnection();
    offlineLogin();
}