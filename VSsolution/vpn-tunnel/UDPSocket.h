#pragma once

#include <mutex>
#include "baseSocket.h"

class UDPSocket : protected BaseSocket
{
public:
	UDPSocket(char* argv[]);

	const std::atomic<byte>* getUDPState();
	const std::atomic<byte>* getSocketState();
	sockaddr_in* getSocketAddr();
	SOCKET* getsoc() {
		return &soc;
	}

	~UDPSocket();

	bool initUDPServer();
	bool initUDPClient();

	bool recvBufferFrom(char* buf, int len, sockaddr* from, int* fromLen, UINT& recvLen);
	bool sendBufferTo(char* buf, int len, sockaddr* to, int toLen, int& sendLen);
	bool safeRecvBufferFrom(char* buf, int len, sockaddr* from, int* fromLen, int& recvLen);
	bool safeSendBufferTo(char* buf, int len, sockaddr* to, int toLen, int& sendLen);

	void stopUDPSocket();

private:
	std::mutex mtx;
	std::atomic<byte> udpState;
};

UDPSocket::UDPSocket(char* argv[])
	:BaseSocket(argv[2], std::atoi(argv[3]))
{
	udpState = INIT_STATE;
}

bool UDPSocket::initUDPServer()
{
	if (!initWinsock(SOCK_DGRAM, IPPROTO_UDP))
	{
		throw "Faild to initialize UDP Winsock socket! WSA error code: " + WSAGetLastError();
		return false;
	}

	if (bind(soc, (sockaddr*)&socketAddr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
		closesocket(soc);
		WSACleanup();
		throw "Error binding UDP socket. WSA error code: " + WSAGetLastError();
		return false;
	}

	u_long mode = 1;
	if (ioctlsocket(soc, FIONBIO, &mode) != 0) {
		std::cerr << "Failed to set socket to non-blocking mode\n";
		closesocket(soc);
		WSACleanup();
		return 1;
	}

	udpState = UDP_INITIALIZED;
	return true;
}

bool UDPSocket::initUDPClient()
{
	if (!initWinsock(SOCK_DGRAM, IPPROTO_UDP))
	{
		throw "Faild to initialize UDP Winsock socket! WSA error code: " + WSAGetLastError();
		return false;
	}

	sockaddr_in clientAddr{};
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.s_addr = INADDR_ANY;
	clientAddr.sin_port = htons(0);
	if (bind(soc, (sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR)
	{
		throw "Failed binding socket! WSA error code: " + WSAGetLastError();
		return false;
	}

	u_long mode = 1;
	if (ioctlsocket(soc, FIONBIO, &mode) != 0) {
		std::cerr << "Failed to set socket to non-blocking mode\n";
		closesocket(soc);
		WSACleanup();
		return 1;
	}

	udpState = UDP_INITIALIZED;
	return true;
}

bool UDPSocket::recvBufferFrom(char* buf, int len, sockaddr* from, int* fromLen, UINT& recvLen)
{
	recvLen = recvfrom(soc, buf, len, 0, from, fromLen);
	switch (recvLen)
	{
	case 0:
		return true;
	case SOCKET_ERROR:
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;
		std::cout << "Error recving UDP data from socket. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

bool UDPSocket::sendBufferTo(char* buf, int len, sockaddr* to, int toLen, int& sendLen)
{
	sendLen = sendto(soc, buf, len, 0, to, toLen);
	switch (sendLen)
	{
	case 0:
		return false;
	case SOCKET_ERROR:
		std::cout << "Error sending UDP data to socket. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

bool UDPSocket::safeRecvBufferFrom(char* buf, int len, sockaddr* from, int* fromLen, int& recvLen)
{
	mtx.lock();
	recvLen = recvfrom(soc, buf, len, 0, reinterpret_cast<sockaddr*>(from), fromLen);
	mtx.unlock();
	switch (recvLen)
	{
	case 0:
		return false;
	case SOCKET_ERROR:
		std::cout << "Error recving UDP data from socket. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

bool UDPSocket::safeSendBufferTo(char* buf, int len, sockaddr* to, int toLen, int& sendLen)
{
	mtx.lock();
	sendLen = sendto(soc, buf, len, 0, to, toLen);
	mtx.unlock();
	switch (sendLen)
	{
	case 0:
		return false;
	case SOCKET_ERROR:
		std::cout << "Error sending UDP data to socket. WSA error code: " << WSAGetLastError() << std::endl;
		return false;
	default:
		return true;
	}
}

inline void UDPSocket::stopUDPSocket()
{
	stopWinSock();
	udpState = UDP_STOPPED;
}

inline const std::atomic<byte>* UDPSocket::getUDPState()
{
	return &udpState;
}

inline const std::atomic<byte>* UDPSocket::getSocketState()
{
	return getState();
}

inline sockaddr_in* UDPSocket::getSocketAddr()
{
	return &socketAddr;
}

UDPSocket::~UDPSocket()
{
}