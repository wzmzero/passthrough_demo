#pragma once
#include "ChannelBase.h"
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>

class ChannelTcpServer : public ChannelBase {
public:
    ChannelTcpServer(const std::string& ip, uint16_t port);
    ~ChannelTcpServer() override;
    
    bool open() override;
    void close() override;
    bool write(const char* data, size_t len) override;

private:
    void run() override;
    bool setupServer();
    
    std::string m_ip;
    uint16_t m_port;
    int m_serverFd = -1;
    int m_clientFd = -1;
    int m_epollFd = -1;
    struct sockaddr_in m_serverAddr{};
};