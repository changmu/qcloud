#include "global.h"
#include "login_dialog.h"
#include "signup_dialog.h"
#include "mainwindow.h"
#include "tcpsocket.h"
#include "Model.h"
#include "updatePwd_dialog.h"

const char *g_stateStr[] = {
    "计算MD5中",
    "登录中",
    "上传中",
    "下载中",
    "已完成",
    "上传成功",
    "下载成功",

    "文件校验失败",
    "秒传成功",
    NULL,
};

std::unique_ptr<QVector<RemoteFile>> g_fileList;
QMutex g_mutexFileList;
std::unique_ptr<QVector<FileStatus>> g_statusList;
QMutex g_mutexFileStatus;
std::unique_ptr<FileListModel> g_modelFiles;
std::unique_ptr<StatusModel> g_modelStatus;

std::unique_ptr<QString> g_loginIp;
std::unique_ptr<quint16> g_loginPort;
std::unique_ptr<int> g_heartBeat;

std::unique_ptr<QString> g_username;
std::unique_ptr<QString> g_password;

std::unique_ptr<QString> g_homeIp;
std::unique_ptr<quint16> g_homePort;

std::unique_ptr<TcpSocket> g_controlSocket;
std::unique_ptr<LoginDialog> g_loginDialog;
std::unique_ptr<SignupDialog> g_signupDialog;
std::unique_ptr<UpdatePwd_dialog> g_updateDialog;
std::unique_ptr<MainWindow> g_mainWindow;

std::unique_ptr<QString> g_localDir;
