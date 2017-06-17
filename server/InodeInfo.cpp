#include "InodeInfo.h"
#include "Util.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

InodeInfo::InodeInfo(const std::string& iNodeName)
  : fd_(-1),
    totLen_(0),
    rcvLen_(0),
    iNodeName_(iNodeName),
    iNodePath_(".qcloud/"),
    createTime_(time(NULL)),
    md5Checked_(false)
{
    iNodePath_ += iNodeName_;
    open();
}

InodeInfo::~InodeInfo()
{
    if (fd_) {
        ::close(fd_);
        fd_ = -1;
    }
    if (refPath_.empty()) {
        ::unlink(iNodePath_.c_str());
    }
}

void InodeInfo::open()
{
    if (fd_ != -1)
        return;

    fd_ = ::open(iNodePath_.c_str(), O_RDWR | O_CREAT, 0666);
    assert(fd_ != -1);
}

void InodeInfo::write(const char *data, int len)
{
    if (!opened()) open();
    assert(len >= 0);
    int n = Util::writen(fd_, data, len);
    assert(n >= 0);
    rcvLen_ += n;
}

int InodeInfo::read(uint64_t offset, char *data, size_t len)
{
    assert(offset <= totLen_);
    uint64_t leftLen = totLen_ - offset;
    size_t readLen = leftLen < len ? leftLen : len;

    if (!opened()) open();

    int n = Util::preadn(fd_, data, readLen, offset);
    return n;
}

void InodeInfo::link(const std::string& path)
{
    if (Util::fileExist(path)) {
        ::unlink(path.c_str());
    }
    ::link(iNodePath_.c_str(), path.c_str());
}

void InodeInfo::linkAllRefPath()
{
    for (const auto& it : refPath_) {
        ::link(iNodePath_.c_str(), it.first.c_str());
    }
}

void InodeInfo::truncate()
{
    this->close();
    int ret = ::truncate(iNodePath_.c_str(), 0);
    assert(ret != -1);
    rcvLen_ = 0;
}

void InodeInfo::addRefPath(const std::string& filepath, int state)
{
    if (filepath.empty())
        return;
    refPath_[filepath] = state;
}

void InodeInfo::removeRefPath(const std::string &filepath)
{
    if (filepath.empty())
        return;
    if (refPath_.erase(filepath)) {
        ::unlink(filepath.c_str());
    }
}

std::string InodeInfo::getMd5() const
{
    return md5_;
}

void InodeInfo::setMd5(const std::string &md5)
{
    md5_ = md5;
}

uint64_t InodeInfo::getTotLen() const
{
    return totLen_;
}

void InodeInfo::setTotLen(const uint64_t &totLen)
{
    totLen_ = totLen;
}

uint64_t InodeInfo::getRcvLen() const
{
    return rcvLen_;
}

void InodeInfo::setRcvLen(const uint64_t &rcvLen)
{
    rcvLen_ = rcvLen;
}

int InodeInfo::getFd() const
{
    return fd_;
}

void InodeInfo::setFd(int fd)
{
    fd_ = fd;
}

void InodeInfo::dump() const
{
    printf("md5: %s\n", md5_.c_str());
    printf("tot_len: %llu\n", (unsigned long long) totLen_);
    printf("rcv_len: %llu\n", (unsigned long long) rcvLen_);
    printf("refPath:\n");
    for (const auto& name : refPath_) {
        printf("  %s\n", name.first.c_str());
    }
    printf("\n");
}

