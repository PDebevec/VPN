#pragma once

#include "baseSocket.h"

class TCPSocket : protected BaseSocket
{
public:
	TCPSocket(char* argv[]);

	const std::atomic<byte>* getTCPState();
	const std::atomic<byte>* getSocketState();
	sockaddr_in getClientAddr();

	~TCPSocket();

	bool initTCPServer();
	bool initTCPClient();

	bool connectClient();
	bool serverListen();

	bool recvBuffer(char* buf, int len, int& recvLen);
	bool sendBuffer(char* buf, int len, int& sendLen);

	void stopTCPSocket();

private:
	SOCKET client;
	SOCKET* curentSock;

	using BaseSocket::soc;

	std::atomic<byte> tcpState;
};

TCPSocket::TCPSocket(char* argv[])
	:BaseSocket(argv[2], std::atoi(argv[3]))
{
	curentSock = nullptr;
	tcpState = INIT_STATE;
	client = SOCKET_ERROR;
}

bool TCPSocket::initTCPServer()
{
	if (!initWinsock(SOCK_STREAM, IPPROTO_TCP))
	{
		throw "Failed to initialize TCP Winsock socket! WSA error code: " + WSAGetLastError();
		return false;
	}
	
	if (bind(soc, (struct sockaddr*)&socketAddr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
		closesocket(soc);
		WSACleanup();
		throw "Failed to bind TCP server socket. WSA error code: " + WSAGetLastError();
		return false;
	}
	tcpState = TCP_INITIALIZED;
	return true;
}

bool TCPSocket::initTCPClient()
{
	if (!initWinsock(SOCK_STREAM, IPPROTO_TCP))
	{
		throw "Failed to initialize TCP Winsock socket! " + WSAGetLastError();
		return false;
	}
	tcpState = TCP_INITIALIZED;
	return true;
}

bool TCPSocket::connectClient()
{
	if (connect(soc, (struct sockaddr*)&socketAddr, sizeof(sockaddr_in)) < 0) {
		closesocket(soc);
		WSACleanup();
		throw "Failed to connect to a TCP server. WSA Error code: " + WSAGetLastError();
		return false;
	}
	curentSock = &soc;
	tcpState = TCP_CONNECTED;
	return true;
}

bool TCPSocket::serverListen()
{
	if (listen(soc, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(soc);
		WSACleanup();
		throw "Socket error while listening! WSA error code: " + WSAGetLastError();
		return false;
	}

	tcpState = TCP_LISTENING;

	client = accept(soc, NULL, NULL);
	if (client == INVALID_SOCKET || client == SOCKET_ERROR) {
		closesocket(soc);
		WSACleanup();
		throw "Invalid clinet connection! WSA error code: " + WSAGetLastError();
		return false;
	}
	curentSock = &client;
	tcpState = TCP_CONNECTED;
	return true;
}

bool TCPSocket::recvBuffer(char* buf, int len, int& recvLen)
{
	recvLen = recv(*curentSock, buf, len, 0);
	switch (recvLen)
	{
	case 0:
		return false;
	case SOCKET_ERROR:
		std::cout << "Error recving TCP data. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

bool TCPSocket::sendBuffer(char* buf, int len, int& sendLen)
{
	sendLen = send(*curentSock, buf, len, 0);
	switch (sendLen)
	{
	case SOCKET_ERROR:
		std::cout << "Error sending TCP data. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

void TCPSocket::stopTCPSocket()
{
	stopWinSock();
	if (client != SOCKET_ERROR)
	{
		closesocket(client);
	}
	tcpState = TCP_STOPPED;
}

inline const std::atomic<byte>* TCPSocket::getTCPState()
{
	return &tcpState;
}

inline const std::atomic<byte>* TCPSocket::getSocketState()
{
	return getState();
}

sockaddr_in TCPSocket::getClientAddr()
{
	sockaddr_in clientAddr{};
	int clientAddrLen = sizeof(clientAddr);
	if (getpeername(client, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen) == SOCKET_ERROR) {
		closesocket(client);
		WSACleanup();
		throw "Failed to get client address! WSA error code: " + WSAGetLastError();
	}
	return clientAddr;
}

TCPSocket::~TCPSocket()
{
	stopTCPSocket();
	delete curentSock;
}