#include "ChannelTcpClient.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <arpa/inet.h>

ChannelTcpClient::ChannelTcpClient(const std::string& ip, uint16_t port) 
    : m_ip(ip), m_port(port) {
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
}

ChannelTcpClient::~ChannelTcpClient() {
    close();
}

bool ChannelTcpClient::open() {
    if (m_connected) return true;
    
    close();
    
    if (!connectToServer()) {
        return false;
    }
    
    // 创建epoll实例
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        perror("epoll_create1");
        return false;
    }
    
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_socketFd;
    
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socketFd, &ev) == -1) {
        perror("epoll_ctl");
        return false;
    }
    
    m_connected = true;
    if (!m_running) {
        startThread();
    }
    return true;
}

void ChannelTcpClient::close() {
    m_connected = false;
    
    if (m_socketFd != -1) {
        ::close(m_socketFd);
        m_socketFd = -1;
    }
    
    if (m_epollFd != -1) {
        ::close(m_epollFd);
        m_epollFd = -1;
    }
}

bool ChannelTcpClient::write(const char* data, size_t len) {
    if (!m_connected || m_socketFd == -1) return false;
    
    ssize_t sent = ::send(m_socketFd, data, len, 0);
    return sent == static_cast<ssize_t>(len);
}

bool ChannelTcpClient::connectToServer() {
    m_socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socketFd == -1) {
        perror("socket");
        return false;
    }
    
    // 设置非阻塞
    int flags = fcntl(m_socketFd, F_GETFL, 0);
    fcntl(m_socketFd, F_SETFL, flags | O_NONBLOCK);
    
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_port);
    m_serverAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    
    int ret = connect(m_socketFd, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    if (ret == -1 && errno != EINPROGRESS) {
        perror("connect");
        return false;
    }
    
    // 检查连接是否完成
    fd_set set;
    FD_ZERO(&set);
    FD_SET(m_socketFd, &set);
    timeval timeout{.tv_sec = 5, .tv_usec = 0}; // 5秒超时
    
    if (select(m_socketFd + 1, nullptr, &set, nullptr, &timeout) <= 0) {
        perror("select");
        return false;
    }
    
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(m_socketFd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
        perror("getsockopt");
        return false;
    }
    
    if (error != 0) {
        errno = error;
        perror("async connect");
        return false;
    }
    
    return true;
}

void ChannelTcpClient::run() {
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
            if (events[i].data.fd == m_socketFd) {
                // 处理数据
                char buffer[4096];
                ssize_t len;
                while ((len = recv(m_socketFd, buffer, sizeof(buffer), 0))) {
                    if (len == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("recv");
                        close();
                        break;
                    } else if (len == 0) {
                        // 服务器断开
                        close();
                        break;
                    } else {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_callback) {
                            m_callback(buffer, len);
                        }
                    }
                }
            }
        }
    }
}