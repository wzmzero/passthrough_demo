#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

class ChannelBase {
public:
    using DataCallback = std::function<void(const char*, size_t)>;
    
    ChannelBase();
    virtual ~ChannelBase();
    
    virtual bool open() = 0;
    virtual void close() = 0;  // 保持为纯虚函数
    virtual bool write(const char* data, size_t len) = 0;
    
    void setCallback(DataCallback cb);
    bool isOpen() const;

protected:
    virtual void run() = 0;
    void startThread();
    void stopThread();
    void tryReconnect();
    
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    DataCallback m_callback = nullptr;
    int m_reconnectInterval = 3000;
    mutable std::mutex m_mutex;
};