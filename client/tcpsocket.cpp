#include "tcpsocket.h"
#include "protocol.h"
#include "global.h"
#include "Model.h"

#include <QtEndian>
#include <QDebug>
#include <QMessageBox>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static constexpr int kBufSize = 512 << 10;

TcpSocket::TcpSocket(QObject *parent)
  : QTcpSocket(parent)
{
    connect(this, SIGNAL(connected()), this, SLOT(onConnected()));
#if 0
    connect(this, SIGNAL(disconnected()), this, SLOT(onConnected()));
    connect(this, SIGNAL(readyRead()), this, SLOT(onMessage()));
    connect(this, SIGNAL(bytesWritten(qint64)), this, SLOT(onWriteComplete(qint64)));
#endif
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";
    assert(startTimer(*g_heartBeat * 1000));

    connect(this, SIGNAL(flushFileList()), this, SLOT(requestFileList()));
    setSocketOption(QAbstractSocket::LowDelayOption, 1);
}

void TcpSocket::connectServer()
{
    this->connectToHost(ip_, port_);
    if (this->waitForConnected(1000)) {
        qDebug() << "connected!";
    } else {
        qDebug() << "could not connect.";
        QMessageBox::warning(0, "Not connected", "Could not connect server");
        exit(1);
    }
}

void TcpSocket::setIpPort(QString ip, quint16 port)
{
    ip_ = ip;
    port_ = port;
}

bool TcpSocket::isConnected()
{
    return this->state() == QTcpSocket::ConnectedState;
}

void TcpSocket::loginHomeServer()
{
    assert(isConnected());

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kLogin;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = g_username->toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(*g_username);
    len = g_password->toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(*g_password);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    this->write(buffer);
    // this->waitForBytesWritten();

    this->waitForData(8);
    this->read((char *) &pktLen, sizeof pktLen);
    pktLen = qToBigEndian(pktLen);
    this->read((char *) &pktType, sizeof pktType);
    pktType = qToBigEndian(pktType);
    qDebug() << "len =" << pktLen << "type =" << pktType;

    if (pktType == QCloudPktType::kAccepted) {
        qDebug() << "登录家服务器成功";
    } else if (pktType == QCloudPktType::kTooManyConnections) {
        qDebug() << "服务器繁忙";
        assert(0);
        exit(1);
    } else {
        qDebug() << "登录家服务器失败";
        assert(0);
        exit(1);
    }
}

void TcpSocket::downloadBlockRequest(int64_t offset)
{
    QByteArray buffer;
    int32_t pktType = QCloudPktType::kDownloadBlockRequest;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    offset = qToBigEndian(offset);
    buffer.append((char *) &offset, sizeof(offset));
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    this->write(buffer);
    // this->waitForBytesWritten();
}

void TcpSocket::requestFileList()
{
    assert(isConnected());
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]"
             << QThread::currentThread();

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kFileListRequest;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    this->write(buffer);
    // this->waitForBytesWritten();


    this->waitForData(8);
    this->read((char *) &pktLen, sizeof pktLen);
    pktLen = qToBigEndian(pktLen);
    this->read((char *) &pktType, sizeof pktType);
    pktType = qToBigEndian(pktType);
    qDebug() << "len =" << pktLen << "type =" << pktType;


    if (pktType == QCloudPktType::kFileListReply) {
        this->waitForData(4);
        int32_t num;
        this->read((char *) &num, sizeof num);
        num = qToBigEndian(num);
        qDebug() << "remote file's num:" << num;
        assert(num >= 0);

        g_fileList->clear();
        while (num--) {
            this->waitForData(4);
            int32_t nameLen;
            this->read((char *) &nameLen, sizeof nameLen);
            nameLen = qToBigEndian(nameLen);
            qDebug() << "nameLen:" << nameLen;
            this->waitForData(nameLen);
            QByteArray data = this->read(nameLen);
            assert(data.size() == nameLen);
            QString fname = data;
            qDebug() << "fname:" << fname;
            this->waitForData(8);
            int64_t totLen;
            this->read((char *) &totLen, sizeof totLen);
            totLen = qToBigEndian(totLen);
            qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]" << "get fileLen:" << totLen;

            RemoteFile rf;
            rf.name = fname;
            rf.size = totLen;

            g_fileList->push_back(rf);
        }
        emit g_modelFiles->layoutChanged();
    }
}

void TcpSocket::deleteFileRequest(const QString &fname)
{
    assert(isConnected());

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kDeleteFileRequest;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t nameLen = fname.toStdString().size();
    nameLen = qToBigEndian(nameLen);
    buffer.append((char *) &nameLen, sizeof(nameLen));
    buffer.append(fname);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    this->write(buffer);
    // this->waitForBytesWritten();

    qDebug() << "deleteFileRequest:" << fname << "done";
}

void TcpSocket::waitForData(int numBytes)
{
#if 1
    while (this->bytesAvailable() < numBytes) {
        this->waitForReadyRead(-1);
        qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]" << bytesAvailable();
        if (!this->isConnected()) {
            QMessageBox::warning(NULL, "From Server", "Connection closed by server.");
            exit(1);
        }
    }
#else
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    loop.connect(this, SIGNAL(readyRead()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    while (bytesAvailable() < numBytes) {
        timer.start(1000);
        loop.exec();
    }
#endif
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]" << bytesAvailable();
}

void TcpSocket::onConnected()
{
    qDebug() << this->localAddress().toString() << this->localPort() << "->"
             << this->peerAddress().toString() << this->peerPort() << "is"
             << (isConnected() ? "UP" : "DOWN");

    if (isConnected()) {
        // connected.
    } else {
        // disconnected.
    }
}

void TcpSocket::onMessage()
{
    qDebug() << "recv bytes: " << this->bytesAvailable();
    // this->readAll();
}

void TcpSocket::onWriteComplete(qint64)
{
}

void TcpSocket::timerEvent(QTimerEvent *)
{
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";

    if (!isConnected())
        return;

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kHeartBeat;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    this->write(buffer);
    // this->waitForBytesWritten();
}
