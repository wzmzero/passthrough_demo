#pragma once
#include <string>
#include <variant>
#include <stdexcept>

enum class ChannelType {
    TCP_SERVER,
    TCP_CLIENT,
    UDP_SERVER,
    UDP_CLIENT,
    SERIAL,
    UNKNOWN
};

struct TcpServerConfig {
    std::string ip;
    uint16_t port;
};

struct TcpClientConfig {
    std::string ip;
    uint16_t port;
};

struct UdpServerConfig {
    uint16_t port;
};

struct UdpClientConfig {
    std::string ip;
    uint16_t port;
};

struct SerialConfig {
    std::string device;
    int baudrate;
};

using ChannelConfig = std::variant<
    TcpServerConfig,
    TcpClientConfig,
    UdpServerConfig,
    UdpClientConfig,
    SerialConfig
>;

struct BridgeConfig {
    struct Endpoint {
        ChannelType type;
        ChannelConfig config;
    };
    
    Endpoint input;
    Endpoint output;
};