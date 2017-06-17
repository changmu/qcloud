#pragma once

#include "InodeInfo.h"
#include <string>
#include <memory>

class UserInfo {
public:
    UserInfo(const std::string& name);

    std::string getName() const { return name_; }

    InodeInfoPtr getInode() const { return file_; }
    void setInode(const InodeInfoPtr& file) { file_ = file; }

    std::string getFPath() const { return fpath_; }
    void setFPath(const std::string& path) { fpath_ = path; }

private:
    const std::string name_;
    std::string fpath_;
    InodeInfoPtr file_;
};

typedef std::shared_ptr<UserInfo> UserInfoPtr;
