#include "util.h"
#include <QDebug>
#include <iostream>

using namespace std;

namespace Util {

bool loadJsonFromFile(const std::string& file, Json::Value& value)
{
    ifstream ifs(file);
    if (!ifs)
    {
        cout << "could not open file: " << file;
        return false;
    }

    Json::Reader reader;

    bool ok = reader.parse(ifs, value);
    ifs.close();

    return ok;
}

bool writeJsonToFile(const Json::Value& value, const std::string& file)
{
    ofstream ofs(file);
    if (!ofs)
    {
        cout << "could not open file: " << file;
        return false;
    }

    ofs << value.toStyledString();
    ofs.close();

    return true;
}

bool writeStringToFile(const std::string& str, const std::string& file)
 {
    ofstream ofs(file);
    if (!ofs)
    {
        cout << "could not open file: " << file;
        return false;
    }

    ofs << str;
    ofs.close();

    return true;
 }

} // namespace Util
