#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "baseCommunication.h"
#include "packetManipulation.h"
#include "baseWinDivert.h"

class ServerUDP : protected BaseCommunication, protected BaseWinDivert
{
public:
	ServerUDP();

	void startLoop();

	~ServerUDP();

private:
	bool startBase();
	bool stopBase();
	bool startWinsock();
	bool stopWinsock();

	bool startThreads();
	void socketToWD();
	void WDToSocket();

private:
	SOCKET serverSocket;
	WSADATA wsaData;
	sockaddr_in serverAddr;
	std::atomic<byte> status;

	const u_short PORT;
	const char* ADDRESS;
};

ServerUDP::ServerUDP()
	:BaseCommunication(), BaseWinDivert("!loopback or !ipv6 or udp.DstPort != 312", 0),
	ADDRESS("10.20.20.20"), PORT(312),
	wsaData(), serverAddr()
{
	serverSocket = NULL;
	status = 0;
}

inline void ServerUDP::startLoop()
{
	if (!startBase()) {
		return;
	};
	if (!startWinsock())
	{
		return;
	};
	if (!startThreads())
	{
		return;
	}
	while (true)
	{

	}
}

inline void ServerUDP::socketToWD()
{
	std::cout << "loop!" << std::endl;
	status = 1;
	char buffer[WINDIVERT_MTU_MAX];
	UINT8 ubuffer[WINDIVERT_MTU_MAX];
	int bufferLen = sizeof(buffer);
	sockaddr_in clientAddr;
	int fromLen = sizeof(clientAddr);
	WINDIVERT_ADDRESS addr;
	int bytesRecvd = NULL;
	UINT sendLen = NULL;
	int bytesSent = NULL;
	while (status)
	{
		PM::NULLpacket(buffer, bufferLen);

		bytesRecvd = recvfrom(serverSocket, buffer, bufferLen, 0, (struct sockaddr*)&clientAddr, &fromLen);
		switch (bytesRecvd)
		{
		case 0:
			continue;
		case SOCKET_ERROR:
			std::cerr << "Error receving udp data! WSA Error code: " << WSAGetLastError() << std::endl;
			continue;
		}
		
		PM::NULLpacket(ubuffer, bufferLen);

		std::memcpy(ubuffer, buffer, bytesRecvd - sizeof(INT64));
		std::memcpy(&addr.Timestamp, buffer + (bytesRecvd - sizeof(INT64)), sizeof(INT64));

		PM::changePacketSrcIP(ubuffer, 10,30,30,30);

		PM::increaseTTL(ubuffer);

		addr.Outbound = 1;
		addr.Flow.EndpointId = (UINT64)7;
		addr.Network.IfIdx = (UINT32)7;
		addr.Reflect.Timestamp = (INT64)7;
		addr.Reserved3[0] = (UINT8)7;
		addr.Socket.EndpointId = (UINT64)7;
		addr.Socket.EndpointId = (UINT64)7;

		if (WinDivertHelperCalcChecksums(ubuffer, bytesRecvd-sizeof(INT64), &addr, 0) == FALSE)
		{
			continue;
		}

		if (!sendPacket(ubuffer, bytesRecvd-sizeof(INT64), &sendLen, &addr)) {
			std::cerr << "Error sending packet! WD Error code: " << GetLastError() << std::endl;
		}
		std::cout << "injecting ";
	}
}

inline void ServerUDP::WDToSocket()
{
	std::cout << "loop!" << std::endl;
	status = 1;
	char buffer[WINDIVERT_MTU_MAX];
	UINT8 ubuffer[WINDIVERT_MTU_MAX];
	int bufferLen = sizeof(buffer);
	sockaddr_in clientAddr;
	int fromLen = sizeof(clientAddr);
	WINDIVERT_ADDRESS addr;
	int bytesSent = NULL;
	UINT recvLen = NULL;
	while (status)
	{
		PM::NULLpacket(ubuffer, bufferLen);

		if (!recvPacket(ubuffer, bufferLen, &recvLen, &addr)) {
			std::cerr << "Error receving packet! WD Error code: " << GetLastError() << std::endl;
			continue;
		}
		if (addr.Outbound)
		{
			sendPacket(ubuffer, recvLen, NULL, &addr);
			continue;
		}
		if (!PM::isDstIP(ubuffer, 10,30,30,30))
		{
			sendPacket(ubuffer, recvLen, NULL, &addr);
			continue;
		}
		PM::DM::destinationIPAddress(ubuffer);
		std::cout << "recving " << std::endl;

		PM::NULLpacket(buffer, bufferLen);

		std::memcpy(buffer, ubuffer, recvLen);
		std::memcpy(buffer+recvLen, &addr.Timestamp, sizeof(INT64));

		bytesSent = sendto(serverSocket, buffer, recvLen+sizeof(INT64), 0, (struct sockaddr*)&clientAddr, fromLen);

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

inline bool ServerUDP::startThreads()
{
	std::thread t1(&ServerUDP::socketToWD, this);
	std::thread t2(&ServerUDP::WDToSocket, this);

	t1.detach();
	t2.detach();
	return true;
}

inline bool ServerUDP::startWinsock()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Failed to initialize Winsock. Error code: " << WSAGetLastError() << std::endl;
		return false;
	}

	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket. Error code: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, ADDRESS, &(serverAddr.sin_addr)) <= 0) {
		std::cerr << "Invalid IP address. Error code: " << GetLastError()  << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}

	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Error binding socket. Error code: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return false;
	}

	return true;
}

inline bool ServerUDP::startBase()
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

inline bool ServerUDP::stopBase()
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

inline bool ServerUDP::stopWinsock()
{
	if (closesocket(serverSocket) != 0 || WSACleanup() != 0)
	{
		std::cerr << "Error closing UDP socket. Error code: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

ServerUDP::~ServerUDP()
{
	stopWinsock();
	stopBase();
}