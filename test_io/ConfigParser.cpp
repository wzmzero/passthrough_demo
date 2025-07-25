#include "ConfigParser.h"
#include <stdexcept>

ChannelType ConfigParser::stringToType(const std::string& typeStr) {
    if (typeStr == "tcp_server") return ChannelType::TCP_SERVER;
    if (typeStr == "tcp_client") return ChannelType::TCP_CLIENT;
    if (typeStr == "udp_server") return ChannelType::UDP_SERVER;
    if (typeStr == "udp_client") return ChannelType::UDP_CLIENT;
    if (typeStr == "serial") return ChannelType::SERIAL;
    return ChannelType::UNKNOWN;
}

ChannelConfig ConfigParser::parseTcpServer(const YAML::Node& node) {
    TcpServerConfig config;
    config.ip = node["ip"] ? node["ip"].as<std::string>() : "0.0.0.0";
    config.port = node["port"].as<uint16_t>();
    return config;
}

ChannelConfig ConfigParser::parseTcpClient(const YAML::Node& node) {
    TcpClientConfig config;
    config.ip = node["ip"].as<std::string>();
    config.port = node["port"].as<uint16_t>();
    return config;
}

ChannelConfig ConfigParser::parseUdpServer(const YAML::Node& node) {
    UdpServerConfig config;
    config.port = node["port"].as<uint16_t>();
    return config;
}

ChannelConfig ConfigParser::parseUdpClient(const YAML::Node& node) {
    UdpClientConfig config;
    config.ip = node["ip"].as<std::string>();
    config.port = node["port"].as<uint16_t>();
    return config;
}

ChannelConfig ConfigParser::parseSerial(const YAML::Node& node) {
    SerialConfig config;
    config.device = node["device"].as<std::string>();
    config.baudrate = node["baudrate"].as<int>();
    return config;
}

std::vector<BridgeConfig> ConfigParser::parse(const std::string& filepath) {
    auto config = YAML::LoadFile(filepath);
    std::vector<BridgeConfig> bridges;
    
    if (!config["channels"]) {
        throw std::runtime_error("Missing 'channels' section in config");
    }
    
    for (const auto& channel : config["channels"]) {
        BridgeConfig bridge;
        
        // 解析输入配置
        const auto& input = channel["input"];
        if (!input || !input["type"]) {
            throw std::runtime_error("Missing input configuration");
        }
        
        bridge.input.type = stringToType(input["type"].as<std::string>());
        
        switch (bridge.input.type) {
            case ChannelType::TCP_SERVER: 
                bridge.input.config = parseTcpServer(input); break;
            case ChannelType::TCP_CLIENT: 
                bridge.input.config = parseTcpClient(input); break;
            case ChannelType::UDP_SERVER: 
                bridge.input.config = parseUdpServer(input); break;
            case ChannelType::UDP_CLIENT: 
                bridge.input.config = parseUdpClient(input); break;
            case ChannelType::SERIAL: 
                bridge.input.config = parseSerial(input); break;
            default:
                throw std::runtime_error("Unsupported input channel type");
        }
        
        // 解析输出配置
        const auto& output = channel["output"];
        if (!output || !output["type"]) {
            throw std::runtime_error("Missing output configuration");
        }
        
        bridge.output.type = stringToType(output["type"].as<std::string>());
        
        switch (bridge.output.type) {
            case ChannelType::TCP_SERVER: 
                bridge.output.config = parseTcpServer(output); break;
            case ChannelType::TCP_CLIENT: 
                bridge.output.config = parseTcpClient(output); break;
            case ChannelType::UDP_SERVER: 
                bridge.output.config = parseUdpServer(output); break;
            case ChannelType::UDP_CLIENT: 
                bridge.output.config = parseUdpClient(output); break;
            case ChannelType::SERIAL: 
                bridge.output.config = parseSerial(output); break;
            default:
                throw std::runtime_error("Unsupported output channel type");
        }
        
        bridges.push_back(bridge);
    }
    
    return bridges;
}