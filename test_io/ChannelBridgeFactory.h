#pragma once
#include "ChannelBase.h"
#include "ChannelConfig.h"
#include <memory>
#include <mutex>
#include <vector>

class ChannelBridgeFactory {
public:
    enum OldChannelType { TCP_SERVER, TCP_CLIENT, SERIAL, UDP_SERVER, UDP_CLIENT };
    
    struct NodeConfig {
        OldChannelType type;
        std::string param1;
        int param2;
    };
    
    // 原始构造函数
    ChannelBridgeFactory(const NodeConfig& node1, const NodeConfig& node2);
    
    // 新构造函数
    explicit ChannelBridgeFactory(const BridgeConfig& config);
    
    // 添加管理多个桥接的方法
    static std::vector<std::unique_ptr<ChannelBridgeFactory>> createBridges(
        const std::vector<BridgeConfig>& configs
    );
    
    void start();
    void stop();
    
private:
    void forwardData(ChannelBase* target, const char* data, size_t len);
    
    std::unique_ptr<ChannelBase> createChannel(OldChannelType type, 
                                             const std::string& param1, 
                                             int param2);
    
    NodeConfig toNodeConfig(ChannelType type, const ChannelConfig& config);
    
    std::unique_ptr<ChannelBase> m_node1;
    std::unique_ptr<ChannelBase> m_node2;
    std::mutex m_mutex;
};