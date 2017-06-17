#pragma once

#include <stdint.h>
#include <string>
#include "jsoncpp/json/json.h"

namespace Util {
bool loadJsonFromFile(const std::string& file, Json::Value& value);

bool writeJsonToFile(const Json::Value& value, const std::string& file);

bool writeStringToFile(const std::string& str, const std::string& file);

bool fileExist(const std::string& file); // can also test dir

int numOfProcessors();

int32_t inet_addr(const std::string& ip);

std::string itoa(long long num);

bool mkdir(const std::string& dirname);

ssize_t writen(int fd, const void *buf, size_t count);

ssize_t preadn(int fd, void *buf, size_t count, off_t offset);
}
