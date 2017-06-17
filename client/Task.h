#pragma once

#include <QRunnable>
#include "tcpsocket.h"
#include "global.h"
#include <string>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QTime>

class TaskUpload : public QRunnable {
public:
    TaskUpload(const QString& localPath);
    ~TaskUpload();

    void run() override;

private:
    void emitStatusChanged();

private:
    QString localPath_;
    QString filename_;
    TcpSocket *socket_;
    QFile file_;
    uint64_t totLen_;
    uint64_t rcvLen_;
    int rate_; // Byte/Sec
    QTime *qtime_;
    QTimer *timer_;
    State status_;
};

class TaskDownload : public QRunnable {
public:
    TaskDownload(const QString& remotePath, const QString &localDir);
    ~TaskDownload();

    void run() override;

private:
    void emitStatusChanged();

private:
    QString remotePath_;
    QString localPath_;
    QString filename_;
    TcpSocket *socket_;
    QFile file_;
    QString md5_;
    uint64_t totLen_;
    uint64_t rcvLen_;
    int rate_; // Byte/Sec
    QTime *qtime_;
    QTimer *timer_;
    State status_;
};
