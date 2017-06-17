#include "Server.h"

int main(int argc, char *argv[])
{
    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);

    Server server("./config.json");
    server.start();

    return server.exec();
}
