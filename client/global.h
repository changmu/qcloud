#pragma once

#include <memory>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>

enum State {
    kCheckingMd5State,
    kLoginingState, // master
    kFileUploadingState,
    kFileDownloadingState,
    kFinishedState,
    kFinishedUploadState,
    kFinishedDownloadState,

    kMd5NotSameState,
    kQuickUploadSucceedState,
};

extern const char *g_stateStr[];

// for model's usage
struct RemoteFile {
    QString name;
    uint64_t size;
    uint32_t ctime;
};

struct FileStatus {
    FileStatus(const QString& _name = "None", uint64_t _totLen = 0,
               uint64_t _rcvLen = 0, int _rate = 0, int _status = kFinishedState)
      : name(_name),
        totLen(_totLen),
        rcvLen(_rcvLen),
        rate(_rate),
        status(_status)
    {}
    QString name;
    uint64_t totLen;
    uint64_t rcvLen;
    int rate;
    int status;
};

class FileListModel;
class StatusModel;
extern std::unique_ptr<QVector<RemoteFile>> g_fileList;
extern QMutex g_mutexFileList;
extern std::unique_ptr<QVector<FileStatus>> g_statusList;
extern QMutex g_mutexFileStatus;
extern std::unique_ptr<FileListModel> g_modelFiles;
extern std::unique_ptr<StatusModel> g_modelStatus;

extern std::unique_ptr<QString> g_loginIp;
extern std::unique_ptr<quint16> g_loginPort;
extern std::unique_ptr<int> g_heartBeat;

extern std::unique_ptr<QString> g_username;
extern std::unique_ptr<QString> g_password;

extern std::unique_ptr<QString> g_homeIp;
extern std::unique_ptr<quint16> g_homePort;

class TcpSocket;
class LoginDialog;
class SignupDialog;
class UpdatePwd_dialog;
class MainWindow;
extern std::unique_ptr<TcpSocket> g_controlSocket;
extern std::unique_ptr<LoginDialog> g_loginDialog;
extern std::unique_ptr<SignupDialog> g_signupDialog;
extern std::unique_ptr<UpdatePwd_dialog> g_updateDialog;
extern std::unique_ptr<MainWindow> g_mainWindow;

extern std::unique_ptr<QString> g_localDir;
