#pragma once

#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "baseCommunication.h"
#include "packetManipulation.h"
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

    bool startThreads();
    void socketToWD();
    void WDToSocket();

private:
	SOCKET clientSocket;
	WSADATA wsaData;
	sockaddr_in serverAddr;
    std::atomic<byte> status;

	const u_short PORT;
	const char* ADDRESS;
};

ClientUDP::ClientUDP()
    :BaseCommunication(), BaseWinDivert("!loopback or !ipv6 or udp.DstPort != 312", 0),
    ADDRESS("10.20.20.20"), PORT(312),
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
    if (!startWinsock()) {
        return;
    }
    if (!startThreads()) {
        return;
    }

    while (true)
    {

    }
}

inline void ClientUDP::socketToWD()
{
    std::cout << "loop! socket to wd" << std::endl;
    status = 1;
    char packet[WINDIVERT_MTU_MAX];
    UINT8 upacket[WINDIVERT_MTU_MAX];
    UINT packetLen = sizeof(upacket);
    UINT recvLen = NULL;
    WINDIVERT_ADDRESS addr;
    int toLen = sizeof(serverAddr);
    UINT sendLen = NULL;
    int bytesRecvd = NULL;
    while (status)
    {
        PM::NULLpacket(packet, packetLen);

        bytesRecvd = recvfrom(clientSocket, packet, packetLen, 0, (struct sockaddr*)&serverAddr, &toLen);
        switch (bytesRecvd)
        {
        case 0:
            continue;
        case SOCKET_ERROR:
            std::cerr << "Error receving udp data! WSA Error code: " << WSAGetLastError() << std::endl;
            continue;
        }

        PM::NULLpacket(upacket, packetLen);

        std::memcpy(upacket, packet, bytesRecvd - sizeof(INT64));
        std::memcpy(&addr.Timestamp, packet+(recvLen-sizeof(INT64)), sizeof(INT64));

        std::cout << addr.Timestamp << std::endl;

        PM::changePacketDstIP(upacket, 10,1,1,100);

        addr.Flow.EndpointId = (UINT64)3;
        addr.Network.IfIdx = (UINT32)3;
        addr.Reflect.Timestamp = (INT64)3;
        addr.Reserved3[0] = (UINT8)3;
        addr.Socket.EndpointId = (UINT64)3;
        addr.Socket.EndpointId = (UINT64)3;

        if (WinDivertHelperCalcChecksums(upacket, packetLen - sizeof(INT64), nullptr, 0) == FALSE)
        {
            continue;
        }

        if (!sendPacket(upacket, bytesRecvd - sizeof(INT64), &sendLen, &addr)) {
            std::cerr << "Error sending packet! WD Error code: " << GetLastError() << std::endl;
        }
        std::cout << "injecting " << std::endl;
    }
}

inline void ClientUDP::WDToSocket()
{
    std::cout << "loop! wd to socket" << std::endl;
    status = 1;
    char packet[WINDIVERT_MTU_MAX];
    UINT8 upacket[WINDIVERT_MTU_MAX];
    UINT packetLen = sizeof(upacket);
    UINT recvLen = NULL;
    WINDIVERT_ADDRESS addr;
    int toLen = sizeof(serverAddr);
    int bytesSent = NULL;
    while (status)
    {
        PM::NULLpacket(upacket, packetLen);

        if (!recvPacket(upacket, packetLen, &recvLen, &addr)) {
            std::cerr << "Error receving packet! WD Error code: " << GetLastError() << std::endl;
            continue;
        }

        if (!addr.Outbound)
        {
            sendPacket(upacket, recvLen, NULL, &addr);
            continue;
        }
        if (PM::isLocalPacket(upacket))
        {
            sendPacket(upacket, recvLen, NULL, &addr);
            continue;
        }
        std::cout << "recving " << std::endl;

        PM::NULLpacket(packet, packetLen);

        std::memcpy(packet, upacket, recvLen);
        std::memcpy(packet + recvLen, &addr.Timestamp, sizeof(INT64));

        bytesSent = sendto(clientSocket, packet, recvLen + sizeof(INT64), 0, (struct sockaddr*)&serverAddr, toLen);

        switch (bytesSent)
        {
        case 0:
            std::cerr << "No data" << std::endl;
            break;
        case SOCKET_ERROR:
            std::cerr << "Error sending udp data! WSA Error code: " << WSAGetLastError() << std::endl;
        default:
            break;
        }
    }
}

inline bool ClientUDP::startThreads()
{
    std::thread t1(&ClientUDP::socketToWD, this);
    std::thread t2(&ClientUDP::WDToSocket, this);

    t1.detach();
    t2.detach();
    return true;
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