#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <atomic>
#include "codes.h"

#pragma comment(lib, "ws2_32.lib")

class BaseSocket
{
public:
    BaseSocket(const char* address, u_short port);

    const std::atomic<byte>* getState();

	~BaseSocket();

protected:
	bool initWinsock(int type, IPPROTO protocol);
    void stopWinSock();

protected:
	SOCKET soc;
	sockaddr_in socketAddr;

private:
	WSADATA wsaData;

	const u_short PORT;
	const char* ADDRESS;
    
    std::atomic<byte> state;
};

BaseSocket::BaseSocket(const char* address, u_short port)
	:PORT(port), ADDRESS(address),
	wsaData(), socketAddr()
{
    soc = INVALID_SOCKET;
    state = INIT_STATE;
}

bool BaseSocket::initWinsock(int type, IPPROTO protocol)
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock. WSA Error code: " << WSAGetLastError() << std::endl;
        return false;
    }

    soc = socket(AF_INET, type, protocol);
    if (soc == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. WSA Error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ADDRESS, &socketAddr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address. WSA Error code: " << WSAGetLastError() << std::endl;
        closesocket(soc);
        WSACleanup();
        return false;
    }
    state = SOCKET_INITIALIZED;
    return true;
}

inline void BaseSocket::stopWinSock()
{
    state = SOCKET_STOPPED;
    closesocket(soc);
    WSACleanup();
}

inline const std::atomic<byte>* BaseSocket::getState()
{
    return &state;
}

BaseSocket::~BaseSocket()
{
	delete ADDRESS;
    closesocket(soc);
    WSACleanup();
}