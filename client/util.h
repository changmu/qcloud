#ifndef UTIL_H
#define UTIL_H

#include "jsoncpp/json/json.h"

#include <QDebug>

#include <fstream>
#include <string>

namespace Util {
bool loadJsonFromFile(const std::string& file, Json::Value& value);

bool writeJsonToFile(const Json::Value& value, const std::string& file);

bool writeStringToFile(const std::string& str, const std::string& file);
}
#endif // UTIL_H
