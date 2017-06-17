#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QtNetwork>
#include <QObject>
#include <functional>

//#include <boost/any.hpp>

typedef std::function<void ()> Callback;

class TcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit TcpSocket(QObject *parent = 0);

    void connectServer();
    void setIpPort(QString ip, quint16 port);
    bool isConnected();

//    void setContext(const boost::any& ctx) { ctx_ = ctx; }
//    const boost::any& getContext() { return ctx_; }

public slots:
    void loginHomeServer();
    void downloadBlockRequest(int64_t offset);
    void requestFileList();
    void deleteFileRequest(const QString& fname);
    void waitForData(int numBytes);

signals:
    void flushFileList();
private slots:
    void onConnected();
    void onMessage();
    void onWriteComplete(qint64 len);

private:
    void timerEvent(QTimerEvent *event) override;

private:
    QString ip_;
    quint16 port_;

//    boost::any ctx_;

};

#endif // TCPCLIENT_H
