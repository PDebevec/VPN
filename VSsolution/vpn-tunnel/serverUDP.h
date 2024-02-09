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

	void socketLoop();

private:
	SOCKET serverSocket;
	WSADATA wsaData;
	sockaddr_in serverAddr;
	std::atomic<byte> status;

	const u_short PORT;
	const char* ADDRESS;

	std::thread T1;
	std::thread T2;
};

ServerUDP::ServerUDP()
	:BaseCommunication(), BaseWinDivert("true", WINDIVERT_FLAG_SNIFF),
	ADDRESS("10.10.10.12"), PORT(312),
	serverAddr(), wsaData()
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
	socketLoop();
}

inline void ServerUDP::socketLoop()
{
	std::cout << "loop!" << std::endl;
	status = 1;
	char buffer[WINDIVERT_MTU_MAX];
	unsigned char ubuffer[WINDIVERT_MTU_MAX];
	int bufferLen = sizeof(buffer);
	sockaddr_in clientAddr;
	int fromLen = sizeof(clientAddr);
	WINDIVERT_ADDRESS addr;
	int bytesRecvd = NULL;
	UINT sendLen = NULL;

	while (status)
	{
		bytesRecvd = recvfrom(serverSocket, buffer, bufferLen, 0, (struct sockaddr*)&clientAddr, &fromLen);
		switch (bytesRecvd)
		{
		case 0:
			continue;
			break;
		case SOCKET_ERROR:
			std::cerr << "Error receving udp data! Error code: " << WSAGetLastError() << std::endl;
			continue;
			break;
		}

		std::memcpy(ubuffer, buffer, bytesRecvd);

		PM::displayIPv4HeaderInfo(ubuffer, bytesRecvd);

		if (!sendPacket(ubuffer, bufferLen, &sendLen, &addr)) {
			std::cerr << "Error sending packet! Error code: " << GetLastError() << std::endl;
		}
		return;
	}
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