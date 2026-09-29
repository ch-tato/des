// network.hpp
// -----------------------------------------------------------------------
// Minimal POSIX (Linux/macOS) TCP socket helpers so sender.cpp/receiver.cpp
// stay focused on the DES logic instead of socket boilerplate.
//
// NOTE: this uses BSD sockets (sys/socket.h). On Windows you'd need to
// swap this file for one using Winsock (winsock2.h) instead.
// -----------------------------------------------------------------------
#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace net
{

inline int createServerSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all local interfaces
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Bind failed - port may already be in use");
    }
    if (listen(sockfd, 1) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Listen failed");
    }
    return sockfd;
}

inline int acceptClient(int serverSock, std::string &clientIP)
{
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    int clientSock = accept(serverSock, reinterpret_cast<sockaddr *>(&clientAddr), &len);
    if (clientSock < 0)
        throw std::runtime_error("Accept failed");
    clientIP = inet_ntoa(clientAddr.sin_addr);
    return clientSock;
}

inline int connectToServer(const std::string &ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("Failed to create socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
    {
        close(sockfd);
        throw std::runtime_error("Invalid IP address: " + ip);
    }
    if (connect(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        close(sockfd);
        throw std::runtime_error("Connection failed - is the receiver running and reachable?");
    }
    return sockfd;
}

inline void sendAll(int sock, const void *data, size_t len)
{
    const char *ptr = static_cast<const char *>(data);
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(sock, ptr + sent, len - sent, 0);
        if (n <= 0)
            throw std::runtime_error("Send failed / connection lost");
        sent += static_cast<size_t>(n);
    }
}

inline void recvAll(int sock, void *data, size_t len)
{
    char *ptr = static_cast<char *>(data);
    size_t received = 0;
    while (received < len)
    {
        ssize_t n = recv(sock, ptr + received, len - received, 0);
        if (n <= 0)
            throw std::runtime_error("Connection closed unexpectedly / recv failed");
        received += static_cast<size_t>(n);
    }
}

} // namespace net
