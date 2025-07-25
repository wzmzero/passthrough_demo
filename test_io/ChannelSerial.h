#pragma once
#include "ChannelBase.h"
#include <string>
#include <termios.h>
#include <sys/epoll.h>

class ChannelSerial : public ChannelBase {
public:
    ChannelSerial(const std::string& device, int baudrate);
    ~ChannelSerial() override;
    
    bool open() override;
    void close() override;
    bool write(const char* data, size_t len) override;

private:
    void run() override;
    bool configureSerial(int fd);
    
    std::string m_device;
    int m_baudrate;
    int m_serialFd = -1;
    int m_epollFd = -1;
};