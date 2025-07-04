#pragma once
#include "endpoint.h"
#include <cstdint>
#include <memory>
#include <string>

class EndpointFactory {
public:
    static std::unique_ptr<Endpoint> createTcpServer(uint16_t port);
    static std::unique_ptr<Endpoint> createTcpClient(const std::string& ip, uint16_t port);
    static std::unique_ptr<Endpoint> createUdpServer(uint16_t port);
    static std::unique_ptr<Endpoint> createUdpClient(const std::string& ip, uint16_t port);
    static std::unique_ptr<Endpoint> createSerial(const std::string& port, uint32_t baud_rate);
};