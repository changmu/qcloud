#pragma once

#include <muduo/net/TcpServer.h>
#include <boost/circular_buffer.hpp>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <boost/any.hpp>

// for conn heart beat check use
class Entry;
typedef std::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;
typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> WeakEntryPtr;
typedef std::unordered_set<EntryPtr> Bucket;
typedef boost::circular_buffer<Bucket> WeakConnectionList;
class Entry : muduo::copyable {
public:
    Entry(const WeakTcpConnectionPtr& weakConn)
      : conn_(weakConn)
    {}
    ~Entry() {
        auto conn = conn_.lock();
        if (conn) {
            conn->shutdown();
        }
    }
    bool expired() const { return conn_.expired(); }
private:
    WeakTcpConnectionPtr conn_;
};


class ConnectionContext : muduo::copyable {
public:
    enum State {
        kNotLogin,
        kServerOnline,
        kLogining, // master
        kLogined,
        kFileUploading,
        kFileDownloading,
        kCheckingMd5,
        kClosing,
    };

    ConnectionContext(State state = kNotLogin);

    State getState() const;
    void setState(const State &state);

    EntryPtr getEntryPtr() const;
    void setWkEntryPtr(const EntryPtr &entryPtr);

    void setContext(const boost::any& context) { context_ = context; }
    const boost::any& getContext() const { return context_; }

private:
    State state_;
    WeakEntryPtr wkEntryPtr_;
    boost::any context_; // depends on state
};

