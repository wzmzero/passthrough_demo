#include "ChannelBridgeFactory.h"
#include "ConfigParser.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <memory>

std::atomic<bool> running{true};

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // 1. 解析配置文件
        auto configs = ConfigParser::parse("config.yml");
        std::cout << "Loaded " << configs.size() << " bridge configurations\n";
        
        // 2. 创建所有桥接
        auto bridges = ChannelBridgeFactory::createBridges(configs);
        std::cout << "Created " << bridges.size() << " bridges\n";
        
        // 3. 启动所有桥接
        for (auto& bridge : bridges) {
            bridge->start();
        }
        
        std::cout << "Data bridges started. Press Ctrl+C to exit..." << std::endl;
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 4. 停止所有桥接
        for (auto& bridge : bridges) {
            bridge->stop();
        }
        
        std::cout << "All data bridges stopped." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}