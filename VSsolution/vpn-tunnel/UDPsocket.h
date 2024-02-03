#pragma once

#include <thread>
#include "baseVPN.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class UDPsocket : private BaseVPN
{
public:
	UDPsocket();

	void startLoop();
	void stopLoop();

	~UDPsocket();

protected:
	bool initializeWinsock();

	void socketLoop();

private:
    WSADATA wsaData;
	SOCKET clientSocket;
	sockaddr_in serverAddr;
	std::atomic<bool> loop;
    int bytesSent;

	const u_short PORT;
	const char* ADDRESS;

	std::thread T1;
	std::thread T2;
};

UDPsocket::UDPsocket()
	:BaseVPN("outbound && !loopback"), PORT(312), ADDRESS("10.10.10.11")
{
    wsaData = WSADATA();
    serverAddr = sockaddr_in();
	loop = false;
    clientSocket = NULL;
    bytesSent = NULL;
}

inline void UDPsocket::startLoop()
{
    if (initializeWinsock())
    {
        loop = true;
    }
    else return;

    if (std::thread::hardware_concurrency() < 4)
    {
        T1 = std::thread(&UDPsocket::socketLoop, this);

        T1.detach();
    }
    else
    {
        T1 = std::thread(&UDPsocket::socketLoop, this);
        T2 = std::thread(&UDPsocket::socketLoop, this);

        T1.detach();
        T2.detach();
    }
}

inline void UDPsocket::socketLoop()
{
    unsigned char rPacket[MAX_PACKET_SIZE] = {};
    UINT bytesRead = NULL;

    if (!recvPacket(rPacket, &bytesRead, NULL)) {
        return;
    }

    bytesSent = sendto(clientSocket, getRecvPacket(rPacket), bytesRead, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (bytesSent == SOCKET_ERROR)
    {
        std::cout << "Error sending data." << WSAGetLastError() << std::endl;
        return;
    }

    //char buffer[1024];
    //sockaddr_in serverResponseAddr;
    //int serverResponseAddrSize = sizeof(serverResponseAddr);

    //int bytesRead = recvfrom(clientSocket, buffer, sizeof(buffer), 0,
    //    (struct sockaddr*)&serverResponseAddr, &serverResponseAddrSize);

    //if (bytesRead == SOCKET_ERROR)
    //{
    //    std::cout << "Error receving data." << WSAGetLastError() << std::endl;
    //    return;
    //}

    //buffer[bytesRead] = '\0'; // Null-terminate the received data
    //std::cout << "Received response from the server: " << buffer << std::endl;
}

inline bool UDPsocket::initializeWinsock()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return false;
    }

    // Set up server address information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);  // Use the same port as the server

    // Convert server IP address from string to binary
    if (inet_pton(AF_INET, ADDRESS, &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Invalid server IP address." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }
    return true;
}

inline void UDPsocket::stopLoop()
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

    closesocket(clientSocket);
    WSACleanup();
}

UDPsocket::~UDPsocket()
{
}