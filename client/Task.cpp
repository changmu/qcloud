#include "Task.h"
#include "global.h"
#include "protocol.h"
#include "tcpsocket.h"
#include "mainwindow.h"
#include "md5/md5.h"
#include <assert.h>
#include <QMessageBox>
#include <stdio.h>

TaskUpload::TaskUpload(const QString& localPath)
  : localPath_(localPath),
    rcvLen_(0),
    rate_(0)
{
    filename_ = QFileInfo(localPath).fileName();
    assert(filename_.size());

    file_.setFileName(localPath);
    bool ret = file_.open(QIODevice::ReadOnly);
    assert(ret);
    totLen_ = file_.size();

    qDebug() << "file_.size = " << file_.size();
}

TaskUpload::~TaskUpload()
{
    delete socket_;
    delete qtime_;
}

void TaskUpload::run()
{
    socket_ = new TcpSocket;
    qtime_ = new QTime;

    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]" << "checking md5 ..."
             << QThread::currentThread();
    status_ = State::kCheckingMd5State;
    emitStatusChanged();

    QString md5 = QMD5_File(localPath_.toLocal8Bit().toStdString().c_str()).c_str();
    assert(totLen_ == 0 || md5 != QMD5SUM_NULL);
    qDebug() << "fname:" << filename_ << "fname.size:" << filename_.size() << "md5:" << md5;
    qDebug() << "fname.c_str.size:" << filename_.toStdString().size();
    status_ = State::kFileUploadingState;
    emitStatusChanged();

    socket_->setIpPort(*g_homeIp, *g_homePort);
    socket_->connectServer();
    socket_->loginHomeServer();

    QByteArray buffer;
    int32_t pktType = QCloudPktType::kUploadRequest;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = filename_.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(filename_);
    buffer.append(md5);
    uint64_t totLen = totLen_;
    totLen = qToBigEndian(totLen);
    buffer.append((char *) &totLen, sizeof(totLen));
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    assert(socket_->write(buffer) == buffer.size());

    qtime_->start();

    while (true) {
        socket_->waitForData(8);
        socket_->read((char *) &pktLen, sizeof pktLen);
        pktLen = qToBigEndian(pktLen);
        socket_->read((char *) &pktType, sizeof pktType);
        pktType = qToBigEndian(pktType);
        qDebug() << "len =" << pktLen << "type =" << pktType;

        if (pktType == kUploadBlockRequest) {
            qDebug() << "kUploadBlockRequest";
            uint64_t rcvLen;
            socket_->read((char *) &rcvLen, sizeof rcvLen);
            rcvLen = qToBigEndian(rcvLen);
            qDebug() << "offset: " << rcvLen;
            assert(rcvLen < totLen_);
            rcvLen_ = rcvLen;
            bool ret = file_.seek(rcvLen);
            assert(ret);

            char buf[BUF_LEN];
            int32_t readLen = file_.read(buf, sizeof buf);
            assert(readLen > 0);
            QByteArray data(buf, readLen);

            QByteArray buffer;
            int32_t pktType = QCloudPktType::kUploadBlock;
            pktType = qToBigEndian(pktType);
            buffer.append((char *) &pktType, sizeof(pktType));
            rcvLen = qToBigEndian(rcvLen);
            buffer.append((char *) &rcvLen, sizeof(rcvLen));
            readLen = qToBigEndian(readLen);
            buffer.append((char *) &readLen, sizeof(readLen));
            buffer.append(data);
            int32_t pktLen = buffer.size();
            pktLen = qToBigEndian(pktLen);
            buffer.prepend((char *) &pktLen, sizeof(pktLen));
            assert(socket_->write(buffer) == buffer.size());
            rate_ += buffer.size();

            if (qtime_->elapsed() >= 1000) {
                emitStatusChanged();
                rate_ = 0;
                qtime_->restart();
            }
        } else if (pktType == QCloudPktType::kMd5IsSame) {
            rcvLen_ = totLen_;
            status_ = State::kFinishedUploadState;
            emitStatusChanged();
            qDebug() << "kMd5IsSame";
            qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]"
                     << QThread::currentThread();
            emit g_controlSocket->flushFileList();
            break;
        } else if (pktType == QCloudPktType::kMd5NotSame) {
            rcvLen_ = totLen_;
            status_ = State::kMd5NotSameState;
            emitStatusChanged();
            qDebug() << "kMd5NotSame";
            break;
        } else if (pktType == QCloudPktType::kCheckingMd5) {
            rcvLen_ = totLen_;
            status_ = State::kCheckingMd5State;
            emitStatusChanged();
            qDebug() << "kCheckingMd5";
        } else if (pktType == QCloudPktType::kQuickUploadSucceed) {
            status_ = State::kQuickUploadSucceedState;
            rcvLen_ = totLen_;
            emitStatusChanged();

            qDebug() << "kQuickUploadSucceed";
            emit g_controlSocket->flushFileList();
            break;
        }
    }
}

void TaskUpload::emitStatusChanged()
{
    FileStatus fStatus(filename_, totLen_, rcvLen_, rate_, status_);
    emit g_mainWindow->statusChanged(fStatus);
}



TaskDownload::TaskDownload(const QString &remotePath, const QString &localDir)
  : remotePath_(remotePath),
    rcvLen_(0),
    rate_(0)
{
    assert(remotePath_.size());
    filename_ = QFileInfo(remotePath).fileName();
    localPath_ = localDir + filename_;
    file_.setFileName(localPath_);
    file_.open(QIODevice::WriteOnly | QIODevice::Append);
    rcvLen_ = file_.size();
    qDebug() << "rcvLen_:" << rcvLen_;
}

TaskDownload::~TaskDownload()
{
    delete socket_;
    delete qtime_;
}

void TaskDownload::run()
{
    socket_ = new TcpSocket;
    qtime_ = new QTime;
    qDebug() << __func__ << "[" << __FILE__ << ":" << __LINE__ << "]";

    socket_->setIpPort(*g_homeIp, *g_homePort);
    socket_->connectServer();
    socket_->loginHomeServer();


    QByteArray buffer;
    int32_t pktType = QCloudPktType::kDownloadRequest;
    pktType = qToBigEndian(pktType);
    buffer.append((char *) &pktType, sizeof(pktType));
    int32_t len = remotePath_.toStdString().size();
    len = qToBigEndian(len);
    buffer.append((char *) &len, sizeof(len));
    buffer.append(remotePath_);
    int32_t pktLen = buffer.size();
    pktLen = qToBigEndian(pktLen);
    buffer.prepend((char *) &pktLen, sizeof(pktLen));
    socket_->write(buffer);

    qtime_->start();
    status_ = State::kFileDownloadingState;
    emitStatusChanged();

    while (true) {
        socket_->waitForData(8);
        socket_->read((char *) &pktLen, sizeof pktLen);
        pktLen = qToBigEndian(pktLen);
        socket_->read((char *) &pktType, sizeof pktType);
        pktType = qToBigEndian(pktType);
        qDebug() << "len =" << pktLen << "type =" << pktType;

        if (pktType == QCloudPktType::kDownloadReply) {
            qDebug() << "kDownloadReply";
            char md5[33] = { 0 };
            assert(socket_->read(md5, 32) == 32);
            md5_ = md5;
            socket_->read((char *) &totLen_, sizeof totLen_);
            totLen_ = qToBigEndian(totLen_);
            qDebug() << "md5:" << md5_ << "totLen_:" << totLen_;

            if (rcvLen_ == totLen_) {
                file_.close();
                status_ = State::kCheckingMd5State;
                emitStatusChanged();
                QString md5 = QMD5_File(localPath_.toLocal8Bit().toStdString().c_str()).c_str();
                if (md5 == md5_) {
                    status_ = State::kFinishedDownloadState;
                } else {
                    status_ = State::kMd5NotSameState;
                }
                emitStatusChanged();
                qDebug() << "check md5 done.";
                return;
            }

            socket_->downloadBlockRequest(rcvLen_);
        } else if (pktType == QCloudPktType::kDownloadBlockReply) {
            qDebug() << "kDownloadBlockReply";
            uint64_t offset;
            socket_->read((char *) &offset, sizeof offset);
            offset = qToBigEndian(offset);
            int32_t dataLen;
            socket_->read((char *) &dataLen, sizeof dataLen);
            dataLen = qToBigEndian(dataLen);
            qDebug() << "offset:" << offset << "dataLen:" << dataLen << "rcvLen:" << rcvLen_;
            assert(offset == rcvLen_);
            assert(dataLen >= 0);

            rate_ += dataLen;
            while (dataLen > 0) {
                socket_->waitForData(1);

                QByteArray data;
                data = socket_->read(dataLen);
                assert(data.size());
                dataLen -= data.size();
                file_.write(data);
            }
            rcvLen_ = file_.size();
            assert(rcvLen_ <= totLen_);

            if (qtime_->elapsed() >= 1000) {
                status_ = State::kFileDownloadingState;
                emitStatusChanged();
                rate_ = 0;
                qtime_->restart();
            }

            if (rcvLen_ == totLen_) {
                file_.close();
                // checking md5
                status_ = State::kCheckingMd5State;
                emitStatusChanged();
                QString md5 = QMD5_File(localPath_.toLocal8Bit().toStdString().c_str()).c_str();
                if (md5 == md5_) {
                    status_ = State::kFinishedDownloadState;
                } else {
                    status_ = State::kMd5NotSameState;
                }
                emitStatusChanged();
                qDebug() << "check md5 done.";
                return;
            }

            socket_->downloadBlockRequest(rcvLen_);
        } else {
            qDebug() << "unknown pkt, type:" << pktType;
            return;
        }
    }
}

void TaskDownload::emitStatusChanged()
{
    FileStatus fStatus(filename_, totLen_, rcvLen_, rate_, status_);
    emit g_mainWindow->statusChanged(fStatus);
}
