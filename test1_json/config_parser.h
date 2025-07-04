#pragma once
#include "endpoint.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "endpoint_factory.h"
struct ChannelConfig {
    std::string name;
    nlohmann::json input;
    nlohmann::json output;
};

class ConfigParser {
public:
    static std::vector<ChannelConfig> parse(const std::string& filename);
    static std::unique_ptr<Endpoint> createEndpoint(const nlohmann::json& config);
};