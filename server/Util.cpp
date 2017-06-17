#include "Util.h"

#include "jsoncpp/json/json.h"
#include <muduo/base/Logging.h>

#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Util {

using namespace std;
using namespace muduo;

bool loadJsonFromFile(const std::string& file, Json::Value& value)
{
    ifstream ifs(file);
    if (!ifs)
    {
        LOG_ERROR << "could not open file: " << file;
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
        LOG_ERROR << "could not open file: " << file;
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
        LOG_ERROR << "could not open or create file: " << file;
        return false;
    }

    ofs << str;
    ofs.close();

    return true;
}

bool fileExist(const std::string& filename)
{
    if (filename.empty())
    {
        return false;
    }

    return ::access(filename.c_str(), F_OK) == 0;
}

int numOfProcessors()
{
    return ::get_nprocs();
}

int32_t inet_addr(const std::string& ip)
{
    return ::inet_addr(ip.c_str());
}

std::string itoa(long long num)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%lld", num);

    return std::string(buf);
}

bool mkdir(const std::string& dirname)
{
    return ::mkdir(dirname.c_str(), 0777) == 0;
}

ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;
    ssize_t nwritten;
    char *bufp = (char *) buf;

    while (nleft > 0) {
        nwritten = write(fd, bufp, nleft);
        if (nwritten < 0) {
            if (errno == EINTR) { continue; }
            return -1;
        } else if (nwritten == 0) { continue; }

        bufp += nwritten;
        nleft -= nwritten;
    }

    return count;
}

ssize_t preadn(int fd, void *buf, size_t count, off_t offset)
{
    size_t nleft = count;
    ssize_t nread;
    char *bufp = (char *) buf;

    while (nleft > 0) {
        nread = pread(fd, bufp, nleft, offset);
        if (nread < 0) {
            if (errno == EINTR) { continue; }
            return -1;
        } else if (nread == 0) { return count - nleft; }

        offset += nread;
        bufp += nread;
        nleft -= nread;
    }

    return count;
}

} // namespace Util
