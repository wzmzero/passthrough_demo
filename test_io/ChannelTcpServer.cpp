#include "ChannelTcpServer.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <arpa/inet.h>

ChannelTcpServer::ChannelTcpServer(const std::string& ip, uint16_t port) 
    : m_ip(ip), m_port(port) {
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
}

ChannelTcpServer::~ChannelTcpServer() {
    close();
}

bool ChannelTcpServer::open() {
    if (m_connected) return true;
    
    close();
    
    if (!setupServer()) {
        return false;
    }
    
    // 创建epoll实例
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        perror("epoll_create1");
        return false;
    }
    
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = m_serverFd;
    
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_serverFd, &ev) == -1) {
        perror("epoll_ctl server");
        return false;
    }
    
    m_connected = true;
    if (!m_running) {
        startThread();
    }
    return true;
}

void ChannelTcpServer::close() {
    m_connected = false;
    
    if (m_clientFd != -1) {
        ::close(m_clientFd);
        m_clientFd = -1;
    }
    
    if (m_serverFd != -1) {
        ::close(m_serverFd);
        m_serverFd = -1;
    }
    
    if (m_epollFd != -1) {
        ::close(m_epollFd);
        m_epollFd = -1;
    }
}

bool ChannelTcpServer::write(const char* data, size_t len) {
    if (!m_connected || m_clientFd == -1) return false;
    
    ssize_t sent = ::send(m_clientFd, data, len, 0);
    return sent == static_cast<ssize_t>(len);
}

bool ChannelTcpServer::setupServer() {
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd == -1) {
        perror("socket");
        return false;
    }
    
    int opt = 1;
    if (setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return false;
    }
    
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_port);
    m_serverAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    
    if (bind(m_serverFd, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) == -1) {
        perror("bind");
        return false;
    }
    
    if (listen(m_serverFd, 5) == -1) {
        perror("listen");
        return false;
    }
    
    return true;
}

void ChannelTcpServer::run() {
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
            if (events[i].data.fd == m_serverFd) {
                // 接受新连接
                sockaddr_in clientAddr{};
                socklen_t addrLen = sizeof(clientAddr);
                int clientFd = accept(m_serverFd, (struct sockaddr*)&clientAddr, &addrLen);
                if (clientFd == -1) {
                    perror("accept");
                    continue;
                }
                
                // 设置非阻塞
                int flags = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
                
                // 关闭旧连接
                if (m_clientFd != -1) {
                    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, m_clientFd, nullptr);
                    ::close(m_clientFd);
                }
                
                m_clientFd = clientFd;
                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = m_clientFd;
                
                if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_clientFd, &ev) == -1) {
                    perror("epoll_ctl client");
                    ::close(m_clientFd);
                    m_clientFd = -1;
                }
            } 
            else if (events[i].data.fd == m_clientFd) {
                // 处理客户端数据
                char buffer[4096];
                ssize_t len;
                while ((len = recv(m_clientFd, buffer, sizeof(buffer), 0))) {
                    if (len == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("recv");
                        close();
                        break;
                    } else if (len == 0) {
                        // 客户端断开
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