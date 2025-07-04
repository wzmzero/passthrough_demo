#pragma once
#include "endpoint.h"
#include "ring_buffer.h"
#include <memory>
#include <thread>
#include <atomic>
#include <string>

class ProtocolChannel {
public:
    ProtocolChannel(const std::string& name,
                   std::unique_ptr<Endpoint> input, 
                   std::unique_ptr<Endpoint> output);
    
    ~ProtocolChannel();
    void start();
    void stop();
    const std::string& getName() const { return name_; }

private:
    void inputToOutputWorker();
    void outputToInputWorker();
    void logHandler(const std::string& msg);
    
    std::string name_;
    std::unique_ptr<Endpoint> input_;
    std::unique_ptr<Endpoint> output_;
    RingBuffer forward_buffer_{1024 * 1024}; // 1MB buffer
    RingBuffer reverse_buffer_{1024 * 1024}; // 1MB buffer
    std::thread forward_thread_;
    std::thread reverse_thread_;
    std::atomic<bool> running_{false};
};