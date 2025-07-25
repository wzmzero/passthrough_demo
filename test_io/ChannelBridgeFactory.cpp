#include "ChannelBridgeFactory.h"
#include "ChannelTcpServer.h"
#include "ChannelTcpClient.h"
#include "ChannelSerial.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

ChannelBridgeFactory::ChannelBridgeFactory(const NodeConfig& node1, const NodeConfig& node2) {
    m_node1 = createChannel(node1.type, node1.param1, node1.param2);
    m_node2 = createChannel(node2.type, node2.param1, node2.param2);
    
    if (m_node1 && m_node2) {
        m_node1->setCallback([this](const char* data, size_t len) {
            forwardData(m_node2.get(), data, len);
        });
        
        m_node2->setCallback([this](const char* data, size_t len) {
            forwardData(m_node1.get(), data, len);
        });
    }
}

ChannelBridgeFactory::ChannelBridgeFactory(const BridgeConfig& config) {
    NodeConfig node1 = toNodeConfig(config.input.type, config.input.config);
    NodeConfig node2 = toNodeConfig(config.output.type, config.output.config);
    
    m_node1 = createChannel(node1.type, node1.param1, node1.param2);
    m_node2 = createChannel(node2.type, node2.param1, node2.param2);
    
    if (m_node1 && m_node2) {
        m_node1->setCallback([this](const char* data, size_t len) {
            forwardData(m_node2.get(), data, len);
        });
        
        m_node2->setCallback([this](const char* data, size_t len) {
            forwardData(m_node1.get(), data, len);
        });
    }
}

std::vector<std::unique_ptr<ChannelBridgeFactory>> 
ChannelBridgeFactory::createBridges(const std::vector<BridgeConfig>& configs) {
    std::vector<std::unique_ptr<ChannelBridgeFactory>> bridges;
    for (const auto& config : configs) {
        bridges.emplace_back(std::make_unique<ChannelBridgeFactory>(config));
    }
    return bridges;
}

void ChannelBridgeFactory::start() {
    if (m_node1) m_node1->open();
    if (m_node2) m_node2->open();
}

void ChannelBridgeFactory::stop() {
    if (m_node1) m_node1->close();
    if (m_node2) m_node2->close();
}


void ChannelBridgeFactory::forwardData(ChannelBase* target, const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 打印调试信息
    std::cout << "\n--- Forwarding Data ---\n";
    std::cout << "Target: " << target << " | Open: " << (target ? target->isOpen() : false) << "\n";
    std::cout << "Data Length: " << len << " bytes\n";
    
    // 打印前16字节的十六进制
    std::cout << "Hex Dump: ";
    for (size_t i = 0; i < std::min(len, static_cast<size_t>(16)); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(static_cast<unsigned char>(data[i])) << " ";
    }
    if (len > 16) std::cout << "...";
    std::cout << std::dec << "\n";
    
    // 打印可打印字符
    std::cout << "ASCII: ";
    for (size_t i = 0; i < std::min(len, static_cast<size_t>(32)); i++) {
        if (isprint(data[i])) {
            std::cout << data[i];
        } else {
            std::cout << '.';
        }
    }
    if (len > 32) std::cout << "...";
    std::cout << "\n----------------------\n";
    
    if (target && target->isOpen()) {
        bool success = target->write(data, len);
        if (!success) {
            std::cerr << "Write failed to target: " << target << std::endl;
        } else {
            std::cout << "Successfully forwarded " << len << " bytes\n";
        }
    } else {
        std::cerr << "Target not open or invalid. Dropping " << len << " bytes\n";
    }
}

std::unique_ptr<ChannelBase> ChannelBridgeFactory::createChannel(
    OldChannelType type, 
    const std::string& param1, 
    int param2
) {
    switch(type) {
        case TCP_SERVER:
            return std::make_unique<ChannelTcpServer>(param1, static_cast<uint16_t>(param2));
        case TCP_CLIENT:
            return std::make_unique<ChannelTcpClient>(param1, static_cast<uint16_t>(param2));
        case SERIAL:
            return std::make_unique<ChannelSerial>(param1, param2);
        case UDP_SERVER:
        case UDP_CLIENT:
            // 暂未实现，但保留扩展性
            std::cerr << "UDP channels not implemented yet: " << param1 << std::endl;
            return nullptr;
        default:
            return nullptr;
    }
}

ChannelBridgeFactory::NodeConfig ChannelBridgeFactory::toNodeConfig(
    ChannelType type, 
    const ChannelConfig& config
) {
    NodeConfig node;
    
    try {
        switch (type) {
            case ChannelType::TCP_SERVER: {
                auto c = std::get<TcpServerConfig>(config);
                node.type = TCP_SERVER;
                node.param1 = c.ip;
                node.param2 = c.port;
                break;
            }
            case ChannelType::TCP_CLIENT: {
                auto c = std::get<TcpClientConfig>(config);
                node.type = TCP_CLIENT;
                node.param1 = c.ip;
                node.param2 = c.port;
                break;
            }
            case ChannelType::SERIAL: {
                auto c = std::get<SerialConfig>(config);
                node.type = SERIAL;
                node.param1 = c.device;
                node.param2 = c.baudrate;
                break;
            }
            case ChannelType::UDP_SERVER: {
                auto c = std::get<UdpServerConfig>(config);
                node.type = UDP_SERVER;
                node.param1 = ""; // UDP服务器不需要IP
                node.param2 = c.port;
                break;
            }
            case ChannelType::UDP_CLIENT: {
                auto c = std::get<UdpClientConfig>(config);
                node.type = UDP_CLIENT;
                node.param1 = c.ip;
                node.param2 = c.port;
                break;
            }
            default:
                throw std::runtime_error("Unsupported channel type");
        }
    } catch (const std::bad_variant_access& e) {
        throw std::runtime_error("Invalid config type for channel");
    }
    
    return node;
}