#include "endpoint_factory.h"
#include "tcp_server_endpoint.h"
#include "tcp_client_endpoint.h"
#include "udp_server_endpoint.h"
#include "udp_client_endpoint.h"
#include "serial_endpoint.h"
 

std::unique_ptr<Endpoint> EndpointFactory::createTcpServer(uint16_t port) {
    return std::make_unique<TcpServerEndpoint>(port);
}

std::unique_ptr<Endpoint> EndpointFactory::createTcpClient(const std::string& ip, uint16_t port) {
    return std::make_unique<TcpClientEndpoint>(ip, port);
}

std::unique_ptr<Endpoint> EndpointFactory::createUdpServer(uint16_t port) {
    return std::make_unique<UdpServerEndpoint>(port);
}

std::unique_ptr<Endpoint> EndpointFactory::createUdpClient(const std::string& ip, uint16_t port) {
    return std::make_unique<UdpClientEndpoint>(ip, port);
}

std::unique_ptr<Endpoint> EndpointFactory::createSerial(const std::string& port, uint32_t baud_rate) {
    return std::make_unique<SerialEndpoint>(port, baud_rate);
}