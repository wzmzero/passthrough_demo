#pragma once
#include "ChannelBase.h"
#include <string>
#include <sys/epoll.h>
#include <netinet/in.h>

class ChannelTcpClient : public ChannelBase {
public:
    ChannelTcpClient(const std::string& ip, uint16_t port);
    ~ChannelTcpClient() override;
    
    bool open() override;
    void close() override;
    bool write(const char* data, size_t len) override;

private:
    void run() override;
    bool connectToServer();
    
    std::string m_ip;
    uint16_t m_port;
    int m_socketFd = -1;
    int m_epollFd = -1;
    struct sockaddr_in m_serverAddr{};
};