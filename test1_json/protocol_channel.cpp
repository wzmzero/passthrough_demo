#include "protocol_channel.h"
#include <iostream>
#include <iomanip>
#include <ctime>

ProtocolChannel::ProtocolChannel(const std::string& name,
                               std::unique_ptr<Endpoint> input, 
                               std::unique_ptr<Endpoint> output)
    : name_(name), input_(std::move(input)), output_(std::move(output)) 
{
    // 设置日志回调
    input_->setLogCallback([this](const std::string& msg) {
        logHandler("[INPUT] " + msg);
    });
    output_->setLogCallback([this](const std::string& msg) {
        logHandler("[OUTPUT] " + msg);
    });
    
    // 设置输入端点接收回调
    input_->setReceiveCallback([this](const uint8_t* data, size_t len) {
        if (!forward_buffer_.push(data, len)) {
            logHandler("Forward buffer overflow, dropped " + std::to_string(len) + " bytes");
        }
    });
    
    // 设置输出端点接收回调
    output_->setReceiveCallback([this](const uint8_t* data, size_t len) {
        if (!reverse_buffer_.push(data, len)) {
            logHandler("Reverse buffer overflow, dropped " + std::to_string(len) + " bytes");
        }
    });
}

ProtocolChannel::~ProtocolChannel() {
    stop();
}

void ProtocolChannel::start() {
    if (running_) return;
    
    running_ = true;
    
    logHandler("Starting channel");
    
    // 打开端点
    if (!input_->open()) {
        logHandler("Failed to open input endpoint");
        return;
    }
    
    if (!output_->open()) {
        logHandler("Failed to open output endpoint");
        input_->close();
        return;
    }
    
    // 启动工作线程
    forward_thread_ = std::thread(&ProtocolChannel::inputToOutputWorker, this);
    reverse_thread_ = std::thread(&ProtocolChannel::outputToInputWorker, this);
    
    logHandler("Channel started");
}

void ProtocolChannel::stop() {
    if (!running_) return;
    
    running_ = false;
    logHandler("Stopping channel");
    
    // 停止缓冲区
    forward_buffer_.stop();
    reverse_buffer_.stop();
    
    // 停止线程
    if (forward_thread_.joinable()) forward_thread_.join();
    if (reverse_thread_.joinable()) reverse_thread_.join();
    
    // 关闭端点
    input_->close();
    output_->close();
    
    logHandler("Channel stopped");
}

void ProtocolChannel::inputToOutputWorker() {
    logHandler("Input->Output worker started");
    while (running_) {
        auto data = forward_buffer_.pop(4096); // 每次最多取4KB
        if (data) {
            output_->send(data->data(), data->size());
        }
    }
    logHandler("Input->Output worker exited");
}

void ProtocolChannel::outputToInputWorker() {
    logHandler("Output->Input worker started");
    while (running_) {
        auto data = reverse_buffer_.pop(4096); // 每次最多取4KB
        if (data) {
            input_->send(data->data(), data->size());
        }
    }
    logHandler("Output->Input worker exited");
}

void ProtocolChannel::logHandler(const std::string& msg) {
    // 获取当前时间
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    // 格式化输出
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << name_ << "] " << msg << std::endl;
}