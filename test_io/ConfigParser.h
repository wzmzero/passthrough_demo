#pragma once
#include "ChannelConfig.h"
#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>

class ConfigParser {
public:
    static std::vector<BridgeConfig> parse(const std::string& filepath);
    
private:
    static ChannelType stringToType(const std::string& typeStr);
    static ChannelConfig parseTcpServer(const YAML::Node& node);
    static ChannelConfig parseTcpClient(const YAML::Node& node);
    static ChannelConfig parseUdpServer(const YAML::Node& node);
    static ChannelConfig parseUdpClient(const YAML::Node& node);
    static ChannelConfig parseSerial(const YAML::Node& node);
};