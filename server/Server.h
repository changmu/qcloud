#pragma once

#include "ConnectionContext.h"
#include "InodeInfo.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/ThreadPool.h>
#include <string>
#include <memory>
#include <stdint.h>

class Server : muduo::noncopyable {
public:
    Server(const std::string& configPath); // read config

    // construct objects and register callback
    void start();
    int exec();

private:
    void loadMetaData(); // users info and files hash.
    void saveInodes();
    void saveUsers();
    void refreshEntry(const muduo::net::TcpConnectionPtr& conn);
    uint32_t getSessionId() { static uint32_t id; return id++; }
    std::string getNextSlaveIp();

    void dumpConfig() const;
    void dumpConnectionBuckets() const;
    void dumpOnlineSlaves() const;
    void dumpInodes() const;
    void dumpUsers() const;


    // business logic

private:
    WeakConnectionList *connectionBuckets_;


    // config
    bool isMaster_;
    uint16_t listenPort_;
    int heartBeat_;
    int maxConnectionNum_;
    std::string usersPath_; // master
    std::string filesHashPath_; // slave
    std::string masterIp_; // for slave only
    uint16_t masterPort_; // for slave only



    // Server use
    muduo::net::EventLoop *loop_;
    muduo::net::TcpServer *server_;
    muduo::net::TcpClient *client_; // slave
    muduo::ThreadPool *pool_; // for slave only
    int curConnectionNum_;

    typedef std::unordered_map<std::string, uint16_t> OnlineSlaveList;
    OnlineSlaveList *onlineSlaves_; // master, ip->port
    typedef std::unordered_map<std::string, muduo::net::TcpConnectionPtr> Slave2connList;
    Slave2connList *slave2conn_; // master
    typedef std::unordered_map<std::string, std::string> User2pwdList;
    User2pwdList *user2pwd_; // name->pwd
    typedef std::unordered_map<std::string, std::string> User2SlaveList;
    User2SlaveList *user2slave_; // master, name->ip

    typedef std::unordered_map<std::string, InodeInfoPtr> Path2inodeList;
    Path2inodeList *path2inode_; // slave
    typedef std::unordered_map<std::string, InodeInfoPtr> Iname2inodeList;
    Iname2inodeList *iname2inode_; // slave

    std::unordered_map<uint32_t, boost::any> session_;
};
