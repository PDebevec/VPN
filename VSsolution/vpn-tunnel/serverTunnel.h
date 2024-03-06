#pragma once

#include "tunnel.h"

class ServerTunnel : public Tunnel
{
public:
	ServerTunnel(char* argv[]);
	~ServerTunnel();

private:
	void initTunnel() override;
	void connectTunnel() override;
	void destroyTunnel() override;

	void WDLoop() override;
	void TCPLoop() override;
	void UDPLoop() override;
};

ServerTunnel::ServerTunnel(char* argv[])
	:Tunnel(argv)
{
	if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--server") == 0)
	{
		switchState = TUNNEL_INIT;
	}
	else {
		tunnelState = TUNNEL_ERROR;
	}
}

void ServerTunnel::initTunnel()
{
	printf("initializing server\n");
	tcp = new TCPSocket(arg);
	udp = new UDPSocket(arg);
	wd = new BaseWinDivert("!loopback and !icmp", 0);

	tcp->initTCPServer();
	udp->initUDPServer();

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_CONNECT;
}

void ServerTunnel::connectTunnel()
{
	printf("connecting server\n");
	tcp->serverListen();

	if (*tcp->getTCPState() != TCP_CONNECTED)
		throw "Error connecting socket!";

	tunnelState = TUNNEL_CONNECTED;
	switchState = TUNNEL_START;
}

void ServerTunnel::destroyTunnel()
{
	printf("destroying\n");
	wd->closeWinDivert();
	udp->stopUDPSocket();
	tcp->stopTCPSocket();

	for (auto *t : tVec)
	{
		if (t->joinable())
		{
			t->join();
		}
		delete t;
	}
	tVec.clear();

	tcp->initTCPServer();
	udp->initUDPServer();

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_CONNECT;
}

void ServerTunnel::WDLoop()
{
	UINT8* packet = new UINT8[WINDIVERT_MTU_MAX];
	UINT packetSize = WINDIVERT_MTU_MAX;
	memset(packet, 0, packetSize);
	int recvLen = NULL;
	UINT sendLen = NULL;
	WINDIVERT_ADDRESS addr{};
	WINDIVERT_ADDRESS injectAddr{};
	injectAddr.IPChecksum = 1;
	injectAddr.Outbound = 1;

	while (!stopTunnel)
	{
		if (!wd->recvPacket(packet, packetSize, (UINT*)recvLen, &addr))
		{
			break;
		}

		if (!addr.Outbound && PM::isDstIP(packet, secAddr))
		{
			caught.push(packet, recvLen);
		}
		else
		{
			injectAddr.Flow.EndpointId = addr.Flow.EndpointId;
			injectAddr.Network.IfIdx = addr.Network.IfIdx;
			injectAddr.Reflect.Timestamp = addr.Reflect.Timestamp;
			injectAddr.Reserved3[0] = addr.Reserved3[0];
			injectAddr.Socket.EndpointId = addr.Socket.EndpointId;
			injectAddr.Timestamp = addr.Timestamp; //+50000

			wd->sendPacket(packet, recvLen, &sendLen, &addr);
		}

		while (!recved.empty())
		{
			delete[] packet;
			packet = recved.pop(recvLen);

			PM::changePacketSrcIP(packet, secAddr);

			if (!wd->sendPacket(packet, recvLen, &sendLen, &injectAddr))
			{
				printf("Error injecting recved packet\n");
			}
		}
		memset(packet, 0, packetSize);
	}

	delete[] packet;
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

void ServerTunnel::TCPLoop()
{
	char* buffer = new char[WINDIVERT_MTU_MAX];
	int bufferSize = WINDIVERT_MTU_MAX;
	memset(buffer, 0, bufferSize);
	int recvLen = NULL;
	int sendLen = NULL;

	while (!stopTunnel)
	{
		if (!tcp->recvBuffer(buffer, bufferSize, recvLen))
		{
			break;
		}

		if (!tcp->sendBuffer(buffer, recvLen, sendLen))
		{
		}
	}

	delete[] buffer;
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

void ServerTunnel::UDPLoop()
{
	char* buffer = new char[WINDIVERT_MTU_MAX];
	int bufferSize = WINDIVERT_MTU_MAX;
	memset(buffer, 0, bufferSize);
	int sendLen = NULL;
	int recvLen = NULL;
	struct sockaddr_in from {};
	int fromLen = sizeof(sockaddr_in);

	while (!stopTunnel)
	{
		if (!udp->recvBufferFrom(buffer, bufferSize, reinterpret_cast<struct sockaddr*>(&from), &fromLen, recvLen))
		{
			break;
		}

		recved.push(reinterpret_cast<UINT8*>(buffer), recvLen);

		while (!caught.empty())
		{
			delete[] buffer;
			buffer = reinterpret_cast<char*>(caught.pop(recvLen));
			if (!udp->sendBufferTo(buffer, recvLen, reinterpret_cast<sockaddr*>(&from), fromLen, sendLen))
			{
				printf("Failed to send buffer\n");
			}
		}
		memset(buffer, 0, bufferSize);
	}

	delete[] buffer;
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

ServerTunnel::~ServerTunnel()
{
}