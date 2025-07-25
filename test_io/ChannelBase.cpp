#include "ChannelBase.h"
#include <chrono>
#include <iostream>

ChannelBase::ChannelBase() = default;

ChannelBase::~ChannelBase() {
    // 移除对close()的调用
    m_running = false;
    stopThread();
}

void ChannelBase::setCallback(DataCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = cb;
}

void ChannelBase::setReconnectInterval(int ms) {
    m_reconnectInterval = ms;
}

bool ChannelBase::isOpen() const {
    return m_connected.load();
}

void ChannelBase::tryReconnect() {
    while (m_running && !m_connected) {
        std::cout << "[" << this << "] Trying to reconnect...\n";
        if (open()) {
            std::cout << "[" << this << "] Reconnected successfully!\n";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectInterval));
    }
}

void ChannelBase::startThread() {
    std::cout << "[" << this << "] Starting channel thread\n";
    m_running = true;
    m_thread = std::thread(&ChannelBase::run, this);
}

void ChannelBase::stopThread() {
    std::cout << "[" << this << "] Stopping channel thread\n";
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
        std::cout << "[" << this << "] Channel thread stopped\n";
    }
}