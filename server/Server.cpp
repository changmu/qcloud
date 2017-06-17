#include "Server.h"
#include "ConnectionContext.h"
#include "Util.h"
#include "UserInfo.h"
#include "protocol.h"
#include "md5/md5.h"
#include <muduo/base/Logging.h>
#include "jsoncpp/json/json.h"

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <time.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

Server::Server(const std::string& configPath)
  : connectionBuckets_(NULL),
    isMaster_(false),
    listenPort_(0),
    heartBeat_(10), // default
    maxConnectionNum_(1), // default
    masterPort_(0),
    loop_(NULL),
    server_(NULL),
    client_(NULL),
    pool_(NULL),
    curConnectionNum_(0),
    onlineSlaves_(NULL),
    slave2conn_(NULL),
    user2pwd_(NULL),
    user2slave_(NULL),
    path2inode_(NULL),
    iname2inode_(NULL)
{
    /// Read Config to Memory
    assert(!configPath.empty());

    Json::Value root;
    if (!Util::loadJsonFromFile(configPath, root)) {
        LOG_FATAL << "load config failed.";
    }

    // for both master and slave
    isMaster_ = root["is_master"].asBool();
    listenPort_ = root["listen_port"].asUInt();
    heartBeat_ = root["heart_beat"].asInt();
    maxConnectionNum_ = root["max_connection_num"].asInt();

    if (!isMaster_) {
        // for slave only
        filesHashPath_ = root["files_hash_path"].asString();
        masterIp_ = root["master_addr"]["ip"].asString();
        masterPort_ = root["master_addr"]["port"].asUInt();
    } else {
        // for master only
        usersPath_ = root["users_path"].asString();
    }
}

static bool checkPktCompleteness(const TcpConnectionPtr& conn, Buffer* buffer)
{
    if (!conn->connected()) {
        LOG_WARN << "conn is DOWN.";
        return false;
    }
    /// Check pkt's completeness (pkt: len | type | data)
    if (buffer->readableBytes() < sizeof(int32_t) * 2) {
        return false;
    }
    uint32_t pktLen = buffer->peekInt32(); // buffer is big endian
    if (pktLen < QCloudLimit::kMinPacketLen || pktLen > QCloudLimit::kMaxPacketLen) {
        LOG_ERROR << "Invalid pkt, len: " << pktLen;
        // conn->shutdown(); // do cleaning in disconnect callback
        return false;
    }
    if (buffer->readableBytes() < sizeof(int32_t) + pktLen) {
        // not a complete pkt
        return false;
    }

    return true;
}

void Server::start()
{
    /// Construct Objects using Config
    loop_ = new EventLoop;
    server_ = new TcpServer(loop_, InetAddress(listenPort_), "qcloud");
    server_->start();
    connectionBuckets_ = new WeakConnectionList(heartBeat_ << 1);
    connectionBuckets_->resize(heartBeat_ << 1);
    user2pwd_ = new User2pwdList;
    if (!isMaster_) {
        // for slave only
        client_ = new TcpClient(loop_, InetAddress(masterIp_, masterPort_), "slave");
        client_->enableRetry();
        client_->connect();
        pool_ = new ThreadPool("qcloud_threadPoll");
        pool_->start(Util::numOfProcessors());
        path2inode_ = new Path2inodeList;
        iname2inode_ = new Iname2inodeList;
    } else {
        // master
        onlineSlaves_ = new OnlineSlaveList;
        slave2conn_ = new Slave2connList;
        user2slave_ = new User2SlaveList;
    }

    loadMetaData();

    /// Register Callbacks
    loop_->runEvery(1.0, [this] {
        connectionBuckets_->push_back(Bucket());
        //dumpConnectionBuckets();
    });

    server_->setConnectionCallback([this](const TcpConnectionPtr& conn) {
        LOG_INFO << "Server - " << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");
        if (conn->connected()) {
            EntryPtr entry(new Entry(conn));
            connectionBuckets_->back().insert(entry);
            dumpConnectionBuckets();

            ConnectionContext ctx;
            ctx.setWkEntryPtr(entry);
            conn->setContext(ctx);

            if (++curConnectionNum_ > maxConnectionNum_) {
                LOG_WARN << "connection num reached top limit. curConnectionNum_ = " << curConnectionNum_;

                Buffer buf;
                buf.appendInt32(QCloudPktType::kTooManyConnections);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);

                conn->shutdown();
            }
        } else {
            --curConnectionNum_;

            ConnectionContext ctx(boost::any_cast<ConnectionContext>(conn->getContext()));
            auto curState = ctx.getState();
            LOG_INFO << "Entry use_count = " << ctx.getEntryPtr().use_count()
                     << " state = " << curState;

            if (curState == ConnectionContext::kServerOnline) {
                // Remove slave from online slave list
                if (onlineSlaves_) {
                    auto slaveIp = conn->peerAddress().toIp();
                    onlineSlaves_->erase(slaveIp);
                    slave2conn_->erase(slaveIp);
                    LOG_INFO << slaveIp << " is offline";
                    dumpOnlineSlaves();
                }
            } else if (curState == ConnectionContext::kFileUploading) {
                UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
                InodeInfoPtr inodePtr = usrPtr->getInode();
                auto path = usrPtr->getFPath();
                inodePtr->removeRefPath(path);
            }
            // TODO: handle other states
        }
    });

    server_->setMessageCallback([this](const TcpConnectionPtr& conn, Buffer* buffer, Timestamp ts) {
    next_pkt:
        if (!checkPktCompleteness(conn, buffer))
            return;

        /// Pkt is completed, get pkt type
        uint32_t pktLen = buffer->readInt32();
        int32_t pktType = buffer->readInt32();
        LOG_DEBUG << "pktLen: " << pktLen << ", pktType: " << pktType;


        ConnectionContext ctx(boost::any_cast<ConnectionContext>(conn->getContext()));
        connectionBuckets_->back().insert(ctx.getEntryPtr());
        // dumpConnectionBuckets();
        // auto currentState = ctx.getState();

        /// Router
        if (pktType == QCloudPktType::kSlaveSignup) { // peer is slave
            if (!onlineSlaves_ || pktLen != 4 + 2) { // sizeof(type) + sizeof(port)
                LOG_ERROR << "Invalid kSlaveSignup pkt, pktLen = " << pktLen;
                // conn->shutdown(); // do cleaning in disconnect callback
                return;
            }

            uint16_t slavePort = buffer->readInt16();
            std::string slaveIp = conn->peerAddress().toIp();
            LOG_DEBUG << "kSlaveSignup: " << slaveIp << ":" << slavePort;
            dumpOnlineSlaves();
            onlineSlaves_->operator [](slaveIp) = slavePort;
            slave2conn_->operator [](slaveIp) = conn;

            // Buffer buf;
            // buf.appendInt32(QCloudPktType::kAccepted);
            // buf.prependInt32(buf.readableBytes());
            // conn->send(&buf);

            ctx.setState(ConnectionContext::kServerOnline);
            conn->setContext(ctx);
            LOG_INFO << "set state to kServerOnline";
        } else if (pktType == QCloudPktType::kSignup) { // client signup
            if (!isMaster_) {
                LOG_ERROR << "client signup in slave.";
                conn->shutdown();
                return;
            }
            // pkt: name_len | name | pwd_len | pwd
            uint32_t nameLen = buffer->readInt32();
            auto name = buffer->retrieveAsString(nameLen);
            uint32_t pwdLen = buffer->readInt32();
            auto pwd = buffer->retrieveAsString(pwdLen);
            // FIXME: check name and pwd
            LOG_DEBUG << "signup name: " << name << ", pwd: " << pwd;

            if (user2pwd_->find(name) == user2pwd_->end()) {
                // allocate slave
                if (onlineSlaves_->size()) {
                    /// save user info
                    auto slaveIp = getNextSlaveIp();
                    user2slave_->operator [](name) = slaveIp;
                    user2pwd_->operator [](name) = QMD5_String(pwd, true);
                    saveUsers();

                    Buffer buf;
                    buf.appendInt32(QCloudPktType::kAccepted);
                    buf.prependInt32(buf.readableBytes());
                    conn->send(&buf);
                } else {
                    // no slave available
                    Buffer buf;
                    buf.appendInt32(QCloudPktType::kNoSlaveAvailable);
                    buf.prependInt32(buf.readableBytes());
                    conn->send(&buf);
                    LOG_DEBUG << "kNoSlaveAvailable";
                }
            } else {
                // name is duplicated
                Buffer buf;
                buf.appendInt32(QCloudPktType::kDumplicatedName);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);
                LOG_DEBUG << "kDumplicatedName";
            }
        } else if (pktType == QCloudPktType::kLogin) { // client --> srv
            LOG_DEBUG << "receive login pkt.";
            // pkt format:
            // name_len:4B | name | pwd_len:4B | pwd
            uint32_t nameLen = buffer->readInt32();
            auto name = buffer->retrieveAsString(nameLen);
            uint32_t pwdLen = buffer->readInt32();
            auto pwd = buffer->retrieveAsString(pwdLen);
            LOG_DEBUG << "name: " << name << ", pwd: " << pwd;

            /// check username and password
            auto pwdMd5 = QMD5_String(pwd, true);
            assert(pwdMd5.size() == 32);
            bool ok = user2pwd_->find(name) != user2pwd_->end() &&
                      user2pwd_->operator [](name) == pwdMd5;
            /// reply
            Buffer buf;
            buf.appendInt32(ok ? QCloudPktType::kAccepted : QCloudPktType::kNameOrPwdErr);
            buf.prependInt32(buf.readableBytes());
            conn->send(&buf);

            if (ok) { // login succeed
                if (isMaster_) {
                    /// allocate slave
                    assert(user2slave_->find(name) != user2slave_->end());
                    auto slaveIp = user2slave_->operator [](name);

                    // get slave's port
                    if (onlineSlaves_->find(slaveIp) != onlineSlaves_->end()) {
                        /// inform slave client's info
                        TcpConnectionPtr slaveConn = slave2conn_->operator [](slaveIp);
                        // keep session snapshot
                        uint32_t sid = getSessionId();
                        session_[sid] = conn;
                        buf.retrieveAll();
                        buf.appendInt32(QCloudPktType::kInformSlaveUserInfo);
                        buf.appendInt32(sid);
                        buf.appendInt32(name.size());
                        buf.append(name);
                        buf.append(pwdMd5);
                        buf.prependInt32(buf.readableBytes());
                        slaveConn->send(&buf);
                    } else { // slave is not online
                        buf.retrieveAll();
                        buf.appendInt32(QCloudPktType::kSlaveNotOnline);
                        buf.prependInt32(buf.readableBytes());
                        conn->send(&buf);
                        // conn->shutdown();
                    }
                } else { // slave
                    /// change ctx
                    // UserInfoPtr usr = new UserInfo(name); // invalid
                    UserInfoPtr usr(new UserInfo(name));
                    ctx.setContext(usr);
                    ctx.setState(ConnectionContext::kLogined);
                    conn->setContext(ctx);

                    /// create user home dir if needed
                    if (!Util::fileExist(name)) {
                        assert(Util::mkdir(name));
                    }
                }
            } else { // login failed, has replied
                // conn->shutdown();
            }
        } else if (pktType == QCloudPktType::kInformSlaveUserInfoReply) {
            uint32_t sid = buffer->readInt32();
            if (session_.find(sid) != session_.end()) {
                TcpConnectionPtr clientConn = boost::any_cast<TcpConnectionPtr>(session_[sid]);
                session_.erase(sid);
                if (!clientConn->connected())
                    return;

                auto slaveIp = conn->peerAddress().toIp();
                auto slavePort = onlineSlaves_->operator [](slaveIp);
                /// reply client
                Buffer buf;
                buf.appendInt32(QCloudPktType::kInformClientSlaveInfo);
                buf.appendInt32(slaveIp.size());
                buf.append(slaveIp);
                buf.appendInt16(slavePort);
                buf.prependInt32(buf.readableBytes());
                clientConn->send(&buf);
                // clientConn->shutdown(); // client will do this
            } else {
                LOG_WARN << "could not find client conn bt sid.";
            }
        } else if (pktType == QCloudPktType::kUploadRequest) {
            // pkt: fname_len: int32_t | fname :string | md5:32B | tot_len:8B
            uint32_t fnameLen = buffer->readInt32();
            auto fname = buffer->retrieveAsString(fnameLen);
            auto md5 = buffer->retrieveAsString(32);
            uint64_t totLen = buffer->readInt64();
            LOG_DEBUG << "name: " << fname << " md5:" << md5 << " totLen: " << totLen;

            /// get context info
            UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
            auto uname = usrPtr->getName();
            InodeInfoPtr inodePtr;
            auto path = uname+"/"+fname;
            usrPtr->setFPath(path);

            /// look up whether inode exists
            std::string iName = md5+"_"+Util::itoa(totLen);
            // create new inode
            if (iname2inode_->find(iName) == iname2inode_->end()) {
                inodePtr = make_shared<InodeInfo>(iName);
                inodePtr->setMd5(md5);
                inodePtr->setTotLen(totLen);
                inodePtr->addRefPath(path, InodeInfo::kWriting);

                iname2inode_->operator [](iName) = inodePtr;
                path2inode_->operator [](path) = inodePtr;

                usrPtr->setInode(inodePtr);
            } else { // inode exists
                inodePtr = iname2inode_->operator [](iName);
                usrPtr->setInode(inodePtr);
                inodePtr->addRefPath(path, InodeInfo::kWriting);
                path2inode_->operator [](path) = inodePtr;
            }

            /// reply client according inode
            if (inodePtr->isMd5Checked()) {
                inodePtr->link(path);

                Buffer buf;
                buf.appendInt32(QCloudPktType::kQuickUploadSucceed);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);

                saveInodes();
            } else { // file incomplete
                if (inodePtr->isWriteComplete()) {
                    inodePtr->close();

                    Buffer buf;
                    buf.appendInt32(QCloudPktType::kCheckingMd5);
                    buf.prependInt32(buf.readableBytes());
                    conn->send(&buf);

                    /// check md5
                    pool_->run([this, conn, inodePtr] {
                        auto md5sum = QMD5_File(inodePtr->getInodePath().c_str());
                        if (md5sum == inodePtr->getMd5()) {
                            inodePtr->setMd5Checked(true);
                            inodePtr->linkAllRefPath();

                            Buffer buf;
                            buf.appendInt32(QCloudPktType::kMd5IsSame);
                            buf.prependInt32(buf.readableBytes());
                            conn->send(&buf);

                            saveInodes();
                        } else { // md5 not same
                            inodePtr->truncate();

                            Buffer buf;
                            buf.appendInt32(QCloudPktType::kMd5NotSame);
                            buf.prependInt32(buf.readableBytes());
                            conn->send(&buf);
                        }

                    });

                    ctx.setState(ConnectionContext::kLogined);
                    conn->setContext(ctx);
                } else {
                    auto rcvLen = inodePtr->getRcvLen();

                    Buffer buf;
                    buf.appendInt32(QCloudPktType::kUploadBlockRequest);
                    buf.appendInt64(rcvLen);
                    buf.prependInt32(buf.readableBytes());
                    conn->send(&buf);

                    ctx.setState(ConnectionContext::kFileUploading);
                    conn->setContext(ctx);
                }
            }
        } else if (pktType == QCloudPktType::kUploadBlock) {
            // pkt: offset:8B | data_len:4B | data
            uint64_t offset = buffer->readInt64();
            int32_t dataLen = buffer->readInt32();
            LOG_DEBUG << "offset: " << offset << " dataLen: " << dataLen;

            UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
            assert(usrPtr);
            InodeInfoPtr inodePtr = usrPtr->getInode();
            assert(inodePtr);

            uint64_t curOffset = inodePtr->getRcvLen();
            if (curOffset == offset) {
                inodePtr->write(buffer->peek(), dataLen);
                buffer->retrieve(dataLen);
            }

            if (inodePtr->isWriteComplete()) {
                inodePtr->close();

                Buffer buf;
                buf.appendInt32(QCloudPktType::kCheckingMd5);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);

                /// check md5
                pool_->run([this, conn, inodePtr] {
                    auto md5sum = QMD5_File(inodePtr->getInodePath().c_str());
                    if (md5sum == inodePtr->getMd5()) {
                        inodePtr->setMd5Checked(true);
                        inodePtr->linkAllRefPath();

                        Buffer buf;
                        buf.appendInt32(QCloudPktType::kMd5IsSame);
                        buf.prependInt32(buf.readableBytes());
                        conn->send(&buf);

                        saveInodes();
                    } else { // md5 not same
                        LOG_DEBUG << "md5 not same: " << md5sum << " ori is " << inodePtr->getMd5();
                        inodePtr->truncate();

                        Buffer buf;
                        buf.appendInt32(QCloudPktType::kMd5NotSame);
                        buf.prependInt32(buf.readableBytes());
                        conn->send(&buf);
                    }

                });

                ctx.setState(ConnectionContext::kLogined);
                conn->setContext(ctx);
            } else { // not complete
                curOffset = inodePtr->getRcvLen();

                Buffer buf;
                buf.appendInt32(QCloudPktType::kUploadBlockRequest);
                buf.appendInt64(curOffset);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);
            }
        } else if (pktType == QCloudPktType::kFileListRequest) {
            UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
            assert(usrPtr);
            auto usrDir = usrPtr->getName() + "/";

            /// get user's files' name
            std::unordered_map<std::string, uint64_t> fileList;
            for (const auto& it : *path2inode_) {
                if (it.first.size() > usrDir.size() &&
                    !it.first.compare(0, usrDir.size(), usrDir) &&
                    it.second->isMd5Checked()) {
                    fileList.emplace(it.first, it.second->getTotLen());
                }
            }

            /// reply, pkt: count: int32_t | f1NameLen: int32_t | f1name: string | f1TotLen: int64_t | f2NameLen...
            Buffer buf;
            buf.appendInt32(QCloudPktType::kFileListReply);
            buf.appendInt32(fileList.size());
            for (const auto& it : fileList) {
                buf.appendInt32(it.first.size());
                buf.append(it.first);
                buf.appendInt64(it.second);
            }
            buf.prependInt32(buf.readableBytes());
            conn->send(&buf);
        } else if (pktType == QCloudPktType::kDownloadRequest) {
            // pkt: path_len: 4B | path: string // "qiu/123.mkv"
            int32_t pathLen = buffer->readInt32();
            auto filePath = buffer->retrieveAsString(pathLen);
            LOG_DEBUG << "pathLen: " << pathLen << ", filePath: " << filePath;
            InodeInfoPtr inodePtr = path2inode_->operator [](filePath);
            assert(inodePtr);
            UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
            assert(usrPtr);
            usrPtr->setInode(inodePtr);
            ctx.setState(ConnectionContext::kFileDownloading);
            conn->setContext(ctx);

            /// reply pkt: md5:32B | tot_len:8B
            Buffer buf;
            buf.appendInt32(QCloudPktType::kDownloadReply);
            buf.append(inodePtr->getMd5());
            buf.appendInt64(inodePtr->getTotLen());
            buf.prependInt32(buf.readableBytes());
            conn->send(&buf);
        } else if (pktType == QCloudPktType::kDownloadBlockRequest) {
            // pkt: offset:8B
            uint64_t offset = buffer->readInt64();

            UserInfoPtr usrPtr = boost::any_cast<UserInfoPtr>(ctx.getContext());
            InodeInfoPtr inodePtr = usrPtr->getInode();
            assert(inodePtr);
            char blk[64<<10];
            int len = inodePtr->read(offset, blk, sizeof blk);
            assert(len >= 0);
            // reply pkt: offset:8B | data_len:4B | data
            Buffer buf;
            buf.appendInt32(QCloudPktType::kDownloadBlockReply);
            buf.appendInt64(offset);
            buf.appendInt32(len);
            buf.append(blk, len);
            buf.prependInt32(buf.readableBytes());
            conn->send(&buf);
        } else if (pktType == QCloudPktType::kDeleteFileRequest) {
            int32_t pathLen = buffer->readInt32();
            auto filePath = buffer->retrieveAsString(pathLen);
            LOG_DEBUG << "pathLen: " << pathLen << ", filePath: " << filePath;
            InodeInfoPtr inodePtr = path2inode_->operator [](filePath);
            assert(inodePtr);
            inodePtr->removeRefPath(filePath);
            path2inode_->erase(filePath);
            saveInodes();
            LOG_DEBUG << "kDeleteFileRequest done.";
        } else if (pktType == QCloudPktType::kUpdatePwd) {
            // pkt: nameLen: int32_t | name: string | oldPwdLen: int32_t | oldPwd: string | newPwdLen: int32_t | newPwd: string
            int32_t nameLen = buffer->readInt32();
            auto name = buffer->retrieveAsString(nameLen);
            int32_t oldPwdLen = buffer->readInt32();
            auto oldPwd = buffer->retrieveAsString(oldPwdLen);
            int32_t newPwdLen = buffer->readInt32();
            auto newPwd = buffer->retrieveAsString(newPwdLen);
            LOG_DEBUG << "kUpdatePwd: name: " << name << ", oldPwd: " << oldPwd << ", newPwd: " << newPwd;
            assert(isMaster_);

            /// check username and password
            auto oldPwdMd5 = QMD5_String(oldPwd, true);
            bool ok = user2pwd_->find(name) != user2pwd_->end() &&
                      user2pwd_->operator [](name) == oldPwdMd5;
            if (ok) {
                user2pwd_->operator [](name) = QMD5_String(newPwd, true);
                saveUsers();
            }
            /// reply
            Buffer buf;
            buf.appendInt32(ok ? QCloudPktType::kAccepted : QCloudPktType::kNameOrPwdErr);
            buf.prependInt32(buf.readableBytes());
            conn->send(&buf);
        } else if (pktType == QCloudPktType::kHeartBeat) {
            // printf("heart beat.\n");
            // dumpConnectionBuckets();
        } else {
            LOG_ERROR << "unkonwn pkt, type = " << pktType << ", conn closed.";
            conn->shutdown();
            return;
        }

        goto next_pkt;
    });

    if (!isMaster_) {
        loop_->runEvery(heartBeat_, [this] {
            if (auto con = client_->connection()) {
                Buffer buf;
                buf.appendInt32(QCloudPktType::kHeartBeat);
                buf.prependInt32(buf.readableBytes());
                con->send(&buf);
            }
        });

        client_->setConnectionCallback([this](const TcpConnectionPtr& conn) {
            LOG_INFO << "Client - " << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " is "
                     << (conn->connected() ? "UP" : "DOWN");
            if (conn->connected()) {
                ConnectionContext ctx(ConnectionContext::kServerOnline);
                conn->setContext(ctx);

                Buffer buf;
                buf.appendInt32(QCloudPktType::kSlaveSignup);
                buf.appendInt16(listenPort_);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);
            } else {
                LOG_INFO << "Client shutdown.";
            }
        });

        client_->setMessageCallback([this](const TcpConnectionPtr& conn, Buffer* buffer, Timestamp ts) {
            if (!checkPktCompleteness(conn, buffer))
                return;

            uint32_t pktLen = buffer->readInt32();
            (void) pktLen;
            int32_t pktType = buffer->readInt32();

            if (pktType == QCloudPktType::kInformSlaveUserInfo) {
                // pkt: sid | name_len | name | pwd (32 bytes md5)
                uint32_t sid = buffer->readInt32();
                uint32_t nameLen = buffer->readInt32();
                std::string name = buffer->retrieveAsString(nameLen);
                std::string pwd = buffer->retrieveAsString(32); // md5

                user2pwd_->operator[](name) = pwd;

                Buffer buf;
                buf.appendInt32(QCloudPktType::kInformSlaveUserInfoReply);
                buf.appendInt32(sid);
                buf.prependInt32(buf.readableBytes());
                conn->send(&buf);
            } else if (0) {
            } else {
                LOG_INFO << "rcv unkonwn pkt from master, type = " << pktType;
            }
        });
        // TODO:
    }
}

int Server::exec()
{
    dumpConfig();
    loop_->loop();
    return 0;
}

void Server::loadMetaData()
{
    if (isMaster_) {
        /// users info
        if (!Util::fileExist(usersPath_)) {
            LOG_WARN << "No " << usersPath_ << " exist, create an empty json file";
            bool ret = Util::writeStringToFile("{}", usersPath_); // create and write
            assert(ret);
        }
        Json::Value root;
        if(!Util::loadJsonFromFile(usersPath_, root)) {
            LOG_SYSFATAL << "load json failed.";
        }
        LOG_INFO << "json = " << root.toStyledString();
        // from json to map
        // {
        //   "name1": { "pwd": "xxx(md5)", "slave_ip": "128.xxx" },
        //    ...
        // }
        for (const auto& name : root.getMemberNames()) { // vector<string>
            user2pwd_->operator[](name) = root[name]["pwd"].asString();
            user2slave_->operator[](name) = root[name]["slave_ip"].asString();
        }
        dumpUsers();
    } else {
        // salve
        bool ret = Util::mkdir(".qcloud");
        LOG_INFO << "mkdir .qcloud " << ret;

        /// hash files info
        if (!Util::fileExist(filesHashPath_)) {
            LOG_WARN << "No " << filesHashPath_ << " exist, create an empty json file";
            ret = Util::writeStringToFile("{}", filesHashPath_); // create and write
            assert(ret);
        }
        Json::Value root;
        if(!Util::loadJsonFromFile(filesHashPath_, root)) {
            LOG_SYSFATAL << "load json failed.";
        }
        LOG_INFO << "json = " << root.toStyledString();
        // from json to map
        // {
        //     "iname1": {
        //       "md5": "xxx",
        //       "md5_checked": true,
        //       "tot_len": 102400,
        //       "rcv_len": 51200,
        //       "create_time": 12345600 // time(NULL)
        //       "ref_path": [
        //         "qiu/1.mkv",
        //         "zhang/2.mkv"
        //       ],
        //     },
        // }
        for (const auto& name : root.getMemberNames()) { // vector<string>
            InodeInfoPtr infoPtr(new InodeInfo(name));
            infoPtr->setMd5(root[name]["md5"].asString());
            infoPtr->setMd5Checked(root[name]["md5_checked"].asBool());
            infoPtr->setTotLen(root[name]["tot_len"].asUInt64());
            infoPtr->setRcvLen(root[name]["rcv_len"].asUInt64());
            infoPtr->setCreateTime(root[name]["create_time"].asUInt());

            for (Json::ArrayIndex i = 0; i < root[name]["ref_path"].size(); ++i) {
                std::string refPath = root[name]["ref_path"][i].asString();
                infoPtr->addRefPath(refPath, InodeInfo::kNoOperation);
                path2inode_->operator [](refPath) = infoPtr;
            }
            iname2inode_->operator [](name) = infoPtr;
        }
        dumpInodes();
    }
}

void Server::saveInodes()
{
    if (isMaster_) return;

    Json::Value root;
    Json::Value value;
    for (auto it = iname2inode_->begin(); it != iname2inode_->end(); ) {
        InodeInfoPtr infoPtr = it->second;
        //if (infoPtr->getRefCount()) {
            value.clear();
            value["md5"] = infoPtr->getMd5();
            value["md5_checked"] = infoPtr->isMd5Checked();
            value["tot_len"] = infoPtr->getTotLen();
            value["rcv_len"] = infoPtr->getRcvLen();
            value["create_time"] = infoPtr->getCreateTime();
            value["ref_path"].resize(0);
            if (infoPtr->isMd5Checked()) {
                for (const auto& refPath : infoPtr->getRefPath()) {
                    value["ref_path"].append(refPath.first);
                }
            }
            root[infoPtr->getInnername()] = value;

            ++it;
        //} else { // delete file in ctor
            //it = iname2inode_->erase(it);
        //}
    }

    assert(Util::writeJsonToFile(root, filesHashPath_));
}

void Server::saveUsers()
{
    if (!isMaster_) return;

    Json::Value root;
    Json::Value value;
    for (const auto& it : *user2pwd_) {
        auto name = it.first;
        auto pwd = it.second;
        auto slaveIp = user2slave_->operator [](it.first);

        value.clear();
        value["pwd"] = pwd;
        value["slave_ip"] = slaveIp;
        root[name] = value;
    }

    assert(Util::writeJsonToFile(root, usersPath_));
}

void Server::refreshEntry(const TcpConnectionPtr &conn)
{
    auto ctx = boost::any_cast<ConnectionContext>(conn->getContext());
    connectionBuckets_->back().insert(ctx.getEntryPtr());
}

std::string Server::getNextSlaveIp()
{
    assert(isMaster_);
    auto slaveCount = onlineSlaves_->size();
    assert(slaveCount);

    static unsigned id = 0;
    if (++id >= slaveCount) id = 0;

    auto it = onlineSlaves_->begin();
    for (unsigned i = 0; i < id; ++i) {
        ++it;
        assert(it != onlineSlaves_->end());
    }
    return it->first;
}

void Server::dumpConfig() const
{
    printf("dumpConfig:\n");
    printf("isMaster_: %s\n", isMaster_ ? "true" : "false");
    printf("listenPort_: %hd\n", listenPort_);
    printf("usersPath_: %s\n", usersPath_.c_str());
    printf("filesHashPath_: %s\n", filesHashPath_.c_str());
    printf("masterIp_: %s\n", masterIp_.c_str());
    printf("masterPort_: %hd\n", masterPort_);
    printf("heartBeat_: %d\n", heartBeat_);
}

void Server::dumpConnectionBuckets() const
{
    printf("dumpConnectionBuckets:\n");
    LOG_INFO << "size = " << connectionBuckets_->size();
    int idx = 0;
    for (const auto& bucket : *connectionBuckets_) {
        printf("[%d] len = %zd : ", idx++, bucket.size());
        for (const auto& entryPtr : bucket) {
            bool connectionDead = entryPtr->expired();
            printf("%p(%ld)%s, ", entryPtr.get(), entryPtr.use_count(),
                   connectionDead ? " DEAD" : "");
        }
        puts("");
    }
}

void Server::dumpOnlineSlaves() const
{
    printf("dumpOnlineSlaves:\n");
    if (onlineSlaves_ == NULL)
        return;

    printf("online slaves: \n");
    for (const auto& it : *onlineSlaves_) {
        printf("[%s:%hd]\n", it.first.c_str(), it.second);
    }
}

void Server::dumpInodes() const
{
    printf("dumpInodes:\n");
    if (!isMaster_) {
        for (const auto& it : *iname2inode_) {
            it.second->dump();
        }
    }
}

void Server::dumpUsers() const
{
    printf("dumpUsers:\n");
    if (!isMaster_) return;

    for (const auto& it : *user2pwd_) {
        printf("name: %s, pwd: %s, slave_ip: %s\n", it.first.c_str(),
               it.second.c_str(), user2slave_->operator [](it.first).c_str());
    }
}
