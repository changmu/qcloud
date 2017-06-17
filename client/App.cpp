#include "App.h"
#include "Util.h"
#include "protocol.h"
#include "Model.h"
#include "updatePwd_dialog.h"
#include <QMetaType>
#include <QMessageBox>
#include <assert.h>

App::App(const std::string& configPath)
{
    qRegisterMetaType<FileStatus>("FileStatus");

    Json::Value root;
    bool ret = Util::loadJsonFromFile(configPath, root);
    assert(ret);

    g_fileList.reset(new QVector<RemoteFile>);
    g_statusList.reset(new QVector<FileStatus>);
    g_modelFiles.reset(new FileListModel);
    g_modelStatus.reset(new StatusModel);

    g_loginIp.reset(new QString(root["ip"].asCString()));
    g_loginPort.reset(new quint16(root["port"].asUInt()));
    g_heartBeat.reset(new int(root["heart_beat"].asInt()));

    g_loginDialog.reset(new LoginDialog);
    g_signupDialog.reset(new SignupDialog);
    g_updateDialog.reset(new UpdatePwd_dialog);
    g_mainWindow.reset(new MainWindow);
    g_localDir.reset(new QString("."));
    g_controlSocket.reset(new TcpSocket);
    g_controlSocket->setIpPort(*g_loginIp, *g_loginPort);

    // signup btn clicked
    connect(g_loginDialog.get(), LoginDialog::singupBtnClicekd, [this] {
        g_loginDialog->hide();
        g_signupDialog->show();
    });

    // login btn clicked
    connect(g_loginDialog.get(), SIGNAL(checkUsernamePassword(QString,QString)),
                     this, SLOT(on_checkUsernamePassword(QString,QString)));

    connect(g_signupDialog.get(), SIGNAL(signup(QString,QString)),
                     this, SLOT(on_signup(QString,QString)));
    connect(g_updateDialog.get(), SIGNAL(updatePwd(QString,QString,QString)),
            this, SLOT(on_updatePwd(QString,QString,QString)));
}

void App::start()
{
#if 0
    g_statusList->push_back(FileStatus("file1", 100, 0, 0, 0));
    g_statusList->push_back(FileStatus("file2", 200, 100, 10, 1));
    g_statusList->push_back(FileStatus("file3", 100, 30, 10, 2));
#endif

    g_controlSocket->connectServer();
    g_loginDialog->show();
}

void App::on_checkUsernamePassword(QString name, QString pwd)
{
    g_username.reset(new QString(name));
    g_password.reset(new QString(pwd));

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kLogin;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = name.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(name);
    len = pwd.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(pwd);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    g_controlSocket->write(buffer);

    while (g_controlSocket->bytesAvailable() < 8) {
        g_controlSocket->waitForReadyRead(-1);
        if (!g_controlSocket->isConnected()) {
            QMessageBox::warning(NULL, "From Server", "Connection closed by server.");
            exit(1);
        }
    }
    // qDebug() << "rcv:" << g_controlSocket->readAll();
    qDebug() << "bytesAvailable:" << g_controlSocket->bytesAvailable();

    g_controlSocket->read((char *) &pktLen, sizeof pktLen);
    pktLen = qToBigEndian(pktLen);
    g_controlSocket->read((char *) &pktType, sizeof pktType);
    pktType = qToBigEndian(pktType);
    qDebug() << "len =" << pktLen << "type =" << pktType;

    if (pktType == QCloudPktType::kAccepted) {
        qDebug() << "正在分配家服务器";

        while (g_controlSocket->bytesAvailable() < 8) {
            g_controlSocket->waitForReadyRead(-1);
            if (!g_controlSocket->isConnected()) {
                QMessageBox::warning(NULL, "From Server", "Connection closed by server.");
                exit(1);
            }
        }

        g_controlSocket->read((char *) &pktLen, sizeof pktLen);
        pktLen = qToBigEndian(pktLen);
        g_controlSocket->read((char *) &pktType, sizeof pktType);
        pktType = qToBigEndian(pktType);
        qDebug() << "len =" << pktLen << "type =" << pktType;

        if (pktType == QCloudPktType::kInformClientSlaveInfo) {
            uint32_t ipLen;
            g_controlSocket->read((char *) &ipLen, sizeof ipLen);
            ipLen = qToBigEndian(ipLen);
            qDebug() << "ipLen =" << ipLen;
            while (g_controlSocket->bytesAvailable() < ipLen + 2) {
                g_controlSocket->waitForReadyRead(-1);
                if (!g_controlSocket->isConnected()) {
                    QMessageBox::warning(NULL, "From Server", "Connection closed by server.");
                    exit(1);
                }
            }
            QString ip(g_controlSocket->read(ipLen));
            uint16_t port;
            g_controlSocket->read((char *) &port, sizeof port);
            port = qToBigEndian(port);
            qDebug() << "ip:" << ip << "port:" << port;

            g_homeIp.reset(new QString(ip));
            g_homePort.reset(new quint16(port));

            g_controlSocket->disconnectFromHost();
            g_controlSocket->state() == QTcpSocket::UnconnectedState
                    || g_controlSocket->waitForDisconnected();
            g_controlSocket->setIpPort(ip, port);
            qDebug() << "正在登录家服务器 ...";
            g_controlSocket->connectServer();
            qDebug() << "连接家服务器成功，即将登录";

            g_controlSocket->loginHomeServer();
            g_loginDialog->hide();
            g_mainWindow->setWindowTitle(*g_username + "@QCloud");
            g_mainWindow->show();
            g_controlSocket->requestFileList();
        } else if (pktType == QCloudPktType::kSlaveNotOnline) {
            g_loginDialog->setText("家服务器不在线", "red");
        } else {
            qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";
        }

    } else if (pktType == QCloudPktType::kNameOrPwdErr) {
        g_loginDialog->setText("用户名或密码错误", "red");
    } else {
        QMessageBox::warning(NULL, "From Server", "未知错误");
    }
}

void App::on_signup(QString name, QString pwd)
{
    g_username.reset(new QString(name));
    g_password.reset(new QString(pwd));

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kSignup;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = name.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(name);
    len = pwd.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(pwd);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    g_controlSocket->write(buffer);

    // waiting reply
    while (g_controlSocket->bytesAvailable() < 8) {
        g_controlSocket->waitForReadyRead(-1);
        if (!g_controlSocket->isConnected()) {
            QMessageBox::warning(NULL, "From Server", "Connection closed by server.");
            exit(1);
        }
    }
    g_controlSocket->read((char *) &pktLen, sizeof pktLen);
    pktLen = qToBigEndian(pktLen);
    g_controlSocket->read((char *) &pktType, sizeof pktType);
    pktType = qToBigEndian(pktType);
    qDebug() << "len =" << pktLen << "type =" << pktType;

    if (pktType == QCloudPktType::kAccepted) {
        QMessageBox::information(NULL, "From Server", "注册成功");

        g_signupDialog->hide();
        g_loginDialog->show();
    } else if (pktType == QCloudPktType::kNoSlaveAvailable) {
        QMessageBox::warning(NULL, "From Server", "No Slave Available");
        exit(1);
    } else if (pktType == QCloudPktType::kDumplicatedName) {
        QMessageBox::warning(NULL, "From Server", "用户名已存在");
    }
}

void App::on_updatePwd(QString name, QString oldPwd, QString newPwd)
{
    QByteArray buffer;
    int32_t pktType = QCloudPktType::kUpdatePwd;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = name.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(name);
    len = oldPwd.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(oldPwd);
    len = newPwd.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(newPwd);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    g_controlSocket->write(buffer);

    g_controlSocket->waitForData(8);

    g_controlSocket->read((char *) &pktLen, sizeof pktLen);
    pktLen = qToBigEndian(pktLen);
    g_controlSocket->read((char *) &pktType, sizeof pktType);
    pktType = qToBigEndian(pktType);
    qDebug() << "len =" << pktLen << "type =" << pktType;

    if (pktType == QCloudPktType::kAccepted) {
        QMessageBox::information(NULL, "Update Password", "Update Succeed.");
        g_updateDialog->hide();
        g_loginDialog->show();
    } else if (pktType == QCloudPktType::kNameOrPwdErr) {
        g_updateDialog->setText("用户名或原密码错误");
    }
}
