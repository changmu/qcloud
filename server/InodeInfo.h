#pragma once

#include <stdint.h>
#include <string>
#include <unistd.h>
#include <memory>
#include <unordered_map>

class InodeInfo {
    typedef std::unordered_map<std::string, int> RefList;
public:
    enum {
        kNoOperation,
        kWriting,
        kWritingQueue,
        kReading,
    };
    InodeInfo(const std::string& iNodeName);
    ~InodeInfo();

    void open();
    bool opened() const { return fd_ != -1; }
    bool isMd5Checked() { return md5Checked_; }
    void write(const char *data, int len);
    int read(uint64_t offset, char *data, size_t len);
    bool isWriteComplete() const { return totLen_ == rcvLen_; }
    void link(const std::string& path);
    void linkAllRefPath();
    void truncate();
    std::string getInodePath() const { return iNodePath_; }
    void addRefPath(const std::string& filepath, int state);
    void removeRefPath(const std::string& filepath);
    size_t getRefCount() const { return refPath_.size(); }

    std::string getMd5() const;
    void setMd5(const std::string &md5);

    uint64_t getTotLen() const;
    void setTotLen(const uint64_t &totLen);

    uint64_t getRcvLen() const;
    void setRcvLen(const uint64_t &rcvLen);
    void increaseRcvLen(uint32_t inc) { rcvLen_ += inc; }

    int getFd() const;
    void setFd(int fd);
    void close() { if (fd_ != -1) ::close(fd_); fd_ = -1; }

    RefList& getRefPath() { return refPath_; }
    //void setInnername(const std::string& name) { innerFilename_ = name; }
    std::string getInnername() const { return iNodeName_; }

    uint32_t getCreateTime() const { return createTime_; }
    void setCreateTime(uint32_t createTime) { createTime_ = createTime; }
    void setMd5Checked(bool checked) { md5Checked_ = checked; }
    void dump() const;

private:
    int fd_;
    std::string md5_;
    uint64_t totLen_;
    uint64_t rcvLen_;
    RefList refPath_; // path->file_state

    std::string iNodeName_; // md5_len
    std::string iNodePath_; // .qloud/xxx
    uint32_t createTime_;
    bool md5Checked_;
};

typedef std::shared_ptr<InodeInfo> InodeInfoPtr;
