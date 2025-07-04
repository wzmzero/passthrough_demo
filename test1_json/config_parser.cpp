#include "config_parser.h"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

std::vector<ChannelConfig> ConfigParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filename);
    }
    
    json config;
    file >> config;
    
    std::vector<ChannelConfig> channels;
    for (const auto& channel : config["channels"]) {
        ChannelConfig ch{
            .name = channel["name"].get<std::string>(),
            .input = channel["input"],
            .output = channel["output"]
        };
        channels.push_back(ch);
    }
    
    return channels;
}

std::unique_ptr<Endpoint> ConfigParser::createEndpoint(const json& config) {
    std::string type = config["type"].get<std::string>();
    
    if (type == "tcp_server") {
        return EndpointFactory::createTcpServer(config["listen_port"].get<uint16_t>());
    }
    else if (type == "udp_server") {
        return EndpointFactory::createUdpServer(config["listen_port"].get<uint16_t>());
    }
    else if (type == "tcp_client") {
        return EndpointFactory::createTcpClient(
            config["server_ip"].get<std::string>(),
            config["server_port"].get<uint16_t>()
        );
    }
    else if (type == "udp_client") {
        return EndpointFactory::createUdpClient(
            config["server_ip"].get<std::string>(),
            config["server_port"].get<uint16_t>()
        );
    }
    else if (type == "serial") {
        return EndpointFactory::createSerial(
            config["port"].get<std::string>(),
            config["baud_rate"].get<uint32_t>()
        );
    }
    
    throw std::runtime_error("Unknown endpoint type: " + type);
}