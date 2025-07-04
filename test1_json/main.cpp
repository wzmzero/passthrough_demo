#include "channel_manager.h"
#include "config_parser.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running{true};

void signalHandler(int signal) {
    running = false;
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // 加载配置
        auto channels = ConfigParser::parse("config/my_config.json");
        
        ChannelManager manager;

        // 创建通道
        for (const auto& config : channels) {
            auto input = ConfigParser::createEndpoint(config.input);
            auto output = ConfigParser::createEndpoint(config.output);
            manager.addChannel(
                std::make_unique<ProtocolChannel>(
                    config.name, // 传递通道名称
                    std::move(input), 
                    std::move(output)
                )
            );
            std::cout << "Created channel: " << config.name << std::endl;
        }
        
        // 启动所有通道
        manager.startAll();
        std::cout << "Protocol converter started. Press Ctrl+C to stop." << std::endl;
        
        // 主循环
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 停止所有通道
        std::cout << "Stopping all channels..." << std::endl;
        manager.stopAll();
        std::cout << "Protocol converter stopped." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}