#pragma once

#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "baseCommunication.h"
#include "baseWinDivert.h"

#pragma comment(lib, "ws2_32.lib")


class ClientUDP : protected BaseCommunication, protected BaseWinDivert
{
public:
	ClientUDP();

    void startLoop();

	~ClientUDP();

private:
    bool startBase();
    bool stopBase();
	bool startWinsock();
    bool stopWinsock();

    void socketLoop();

private:
	SOCKET clientSocket;
	WSADATA wsaData;
	sockaddr_in serverAddr;
    std::atomic<byte> status;

	const u_short PORT;
	const char* ADDRESS;

	std::thread T1;
	std::thread T2;
};

ClientUDP::ClientUDP()
	:BaseCommunication(), BaseWinDivert("outbound && !loopback && udp.DstPort != 312", 0),
	PORT(312), ADDRESS("10.10.10.12"),
    wsaData(), serverAddr()
{
    clientSocket = NULL;
    status = 0;
}

inline void ClientUDP::startLoop()
{
    if (!startBase()) {
        return;
    }
    if(!startWinsock()) {
        return;
    }
    socketLoop();
}

inline void ClientUDP::socketLoop()
{
    std::cout << "loop!" << std::endl;
    char packet[MAX_PACKET_SIZE] = {};
    UINT packetLen = sizeof(packet);
    UINT recvLen = NULL;
    WINDIVERT_ADDRESS addr;
    int sizeofAddr = sizeof(addr);
    int toLen = sizeof(serverAddr);
    sockaddr* to = (struct sockaddr*)&serverAddr;
    int bytesSent = NULL;

    while (status)
    {
        if (!recvPacket(packet, packetLen, &recvLen, &addr)) {
            std::cerr << "Error receving packet. Error code: " << GetLastError() << std::endl;
            continue;
        }

        bytesSent = sendto(clientSocket, packet, recvLen, 0, to, toLen);

        switch (bytesSent)
        {
        case 0:
            std::cerr << "No data" << std::endl;
            break;
        case SOCKET_ERROR:
            std::cerr << "socket error :" << WSAGetLastError() << std::endl;
            break;
        }
    }
}

inline bool ClientUDP::startWinsock()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock. Error code: " << WSAGetLastError() << std::endl;
        return false;
    }

    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Set up server address information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);  // Use the same port as the server

    // Convert server IP address from string to binary
    if (inet_pton(AF_INET, ADDRESS, &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Invalid server IP address. Error code: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }

    return true;
}

inline bool ClientUDP::startBase()
{
    if (!openPipe())
    {
        //return false;
    }

    if (!openWinDivert())
    {
        return false;
    }
    return true;
}

inline bool ClientUDP::stopBase()
{
    if (!closePipe())
    {
        return false;
    }

    if (!closeWinDivert())
    {
        return false;
    }
    return true;
}

inline bool ClientUDP::stopWinsock()
{
    if (closesocket(clientSocket) != 0 || WSACleanup() != 0)
    {
        std::cerr << "Error closing UDP socket. Error code: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

ClientUDP::~ClientUDP()
{
    stopWinsock();
    stopBase();
}