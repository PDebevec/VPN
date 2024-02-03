#pragma once

#include <atomic>
#include <thread>
#include <iostream>
#include "baseVPN.h"
#include <WS2tcpip.h>
#include <Winsock2.h>

#pragma comment(lib, "ws2_32.lib")

class UDPserver : private BaseVPN
{
public:
	UDPserver();

	void startLoop();
	void stopLoop();

	~UDPserver();

protected:
	bool initializeWinsock();
    bool startListening();
    
    void socketLoop();

private:
    SOCKET serverSocket;
    sockaddr_in serverAddr;
	std::atomic<bool> loop;

    const u_short PORT;
    const char* ADDRESS;

    std::thread T1;
    std::thread T2;
};

UDPserver::UDPserver()
    :BaseVPN("outbound"), PORT(312), ADDRESS("10.10.10.11")
{
    serverAddr = sockaddr_in();
    loop = false;
    serverSocket = NULL;
}

inline void UDPserver::startLoop()
{
    if (initializeWinsock() && startListening())
    {
        loop = true;
    }
    else return;

    if (std::thread::hardware_concurrency() < 4)
    {
        T1 = std::thread(&UDPserver::socketLoop, this);

        T1.detach();
    }
    else
    {
        T1 = std::thread(&UDPserver::socketLoop, this);
        T2 = std::thread(&UDPserver::socketLoop, this);

        T1.detach();
        T2.detach();
    }
}

inline void UDPserver::socketLoop()
{
    char buffer[1024];
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    const char* response = "Response message!";

    while (loop)
    {
        int bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Error while receiving data." << std::endl;
            return;
        }

        buffer[bytesRead] = '\0';
        std::cout << buffer << std::endl;

        int bytesSent = sendto(serverSocket, response, strlen(response), 0, (struct sockaddr*)&clientAddr, clientAddrSize);
        if (bytesSent == SOCKET_ERROR)
        {
            std::cerr << "Error while sending data." << std::endl;
            return;
        }
    }
}

inline bool UDPserver::initializeWinsock()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return false;
    }

    // Create a UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return false;
    }

    // Set up server address information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ADDRESS, &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Invalid IP address." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    return true;
}

inline bool UDPserver::startListening()
{
    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket." << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }
    std::cout << "UDP server listening on: " << ADDRESS << ":" << PORT << std::endl;

    return true;
}

inline void UDPserver::stopLoop()
{
    loop = false;

    if (T2.joinable())
    {
        T2.join();
    }
    if (T1.joinable())
    {
        T1.join();
    }

    closesocket(serverSocket);
    WSACleanup();
}

UDPserver::~UDPserver()
{
}