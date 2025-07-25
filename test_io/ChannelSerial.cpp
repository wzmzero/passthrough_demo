#include "ChannelSerial.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <sys/ioctl.h>

ChannelSerial::ChannelSerial(const std::string& device, int baudrate) 
    : m_device(device), m_baudrate(baudrate) {}

ChannelSerial::~ChannelSerial() {
    close();
}

bool ChannelSerial::open() {
    if (m_connected) return true;
    
    close();
    
    m_serialFd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_serialFd == -1) {
        perror("open serial");
        return false;
    }
    
    if (!configureSerial(m_serialFd)) {
        ::close(m_serialFd);
        m_serialFd = -1;
        return false;
    }
    
    // 创建epoll实例
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        perror("epoll_create1");
        ::close(m_serialFd);
        m_serialFd = -1;
        return false;
    }
    
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_serialFd;
    
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_serialFd, &ev) == -1) {
        perror("epoll_ctl");
        ::close(m_serialFd);
        ::close(m_epollFd);
        m_serialFd = -1;
        m_epollFd = -1;
        return false;
    }
    
    m_connected = true;
    if (!m_running) {
        startThread();
    }
    return true;
}

void ChannelSerial::close() {
    m_connected = false;
    
    if (m_serialFd != -1) {
        ::close(m_serialFd);
        m_serialFd = -1;
    }
    
    if (m_epollFd != -1) {
        ::close(m_epollFd);
        m_epollFd = -1;
    }
}

bool ChannelSerial::write(const char* data, size_t len) {
    if (!m_connected || m_serialFd == -1) return false;
    
    ssize_t sent = ::write(m_serialFd, data, len);
    return sent == static_cast<ssize_t>(len);
}

bool ChannelSerial::configureSerial(int fd) {
    termios tty{};
    if (tcgetattr(fd, &tty)) {
        perror("tcgetattr");
        return false;
    }
    
    // 设置波特率
    speed_t speed;
    switch (m_baudrate) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        default: return false;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    tty.c_cflag &= ~PARENB; // 无奇偶校验
    tty.c_cflag &= ~CSTOPB; // 1位停止位
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;     // 8位数据位
    
    tty.c_cflag &= ~CRTSCTS; // 禁用硬件流控
    tty.c_cflag |= CREAD | CLOCAL; // 启用接收
    
    tty.c_lflag &= ~ICANON; // 非规范模式
    tty.c_lflag &= ~ECHO;   // 禁用回显
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;   // 禁用信号
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // 禁用软件流控
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST; // 原始输出
    
    // 设置超时 - 100ms
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
    if (tcsetattr(fd, TCSANOW, &tty)) {
        perror("tcsetattr");
        return false;
    }
    
    return true;
}

void ChannelSerial::run() {
    constexpr int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];
    
    while (m_running) {
        if (!m_connected) {
            tryReconnect();
            continue;
        }
        
        int nfds = epoll_wait(m_epollFd, events, MAX_EVENTS, 500);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == m_serialFd) {
                // 处理串口数据
                char buffer[4096];
                ssize_t len;
                while ((len = read(m_serialFd, buffer, sizeof(buffer))) > 0) {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_callback) {
                        m_callback(buffer, len);
                    }
                }
                
                if (len == -1 && errno != EAGAIN) {
                    perror("read serial");
                    close();
                }
            }
        }
    }
}