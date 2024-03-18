#pragma once

#include "tunnel.h"

class ServerTunnel : public Tunnel
{
public:
	ServerTunnel(char* argv[]);
	~ServerTunnel();

private:
	void initTunnel() override;
	void destroyTunnel() override;

	void WDLoop() override;
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
	udp = new UDPSocket(arg);
	wd = new BaseWinDivert("!loopback and !icmp", 0);

	udp->initUDPServer();

	if (*udp->getUDPState() != UDP_INITIALIZED)
		throw "Erorr initializing UDP socket!";

	tVec.push_back(new std::thread(&ServerTunnel::UDPLoop, this));

	wd->openWinDivert();

	if (*wd->getState() != WD_OPENED)
		throw "Error opening WinDivert!";

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_LOOP;
}

void ServerTunnel::destroyTunnel()
{
	wd->closeWinDivert();
	udp->stopUDPSocket();

	for (auto *t : tVec)
	{
		if (t->joinable())
		{
			t->join();
		}
		delete t;
	}
	tVec.clear();

	udp->initUDPServer();

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_CONNECT;
}

void ServerTunnel::WDLoop()
{
	printf("WD loop\n");
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	UINT packetSize = WINDIVERT_MTU_MAX;
	UINT recvLen = NULL;
	UINT sendLen = NULL;
	WINDIVERT_ADDRESS addr{};
	WINDIVERT_ADDRESS injectAddr{};
	injectAddr.Outbound = 1;

	while (!stopTunnel)
	{
		if (!wd->recvPacket(packet.get(), packetSize, &recvLen, &addr))
		{
			break;
		}

		injectAddr.Timestamp = addr.Timestamp;

		if (!addr.Outbound && PM::isDstIP(packet.get(), secAddr))
		{
			caught.push(packet.release(), recvLen);
			packet.reset(new UINT8[WINDIVERT_MTU_MAX]);
		}
		else
		{
			injectAddr.Flow.EndpointId = addr.Flow.EndpointId;
			injectAddr.Network.IfIdx = addr.Network.IfIdx;
			injectAddr.Reflect.Timestamp = addr.Reflect.Timestamp;
			injectAddr.Reserved3[0] = addr.Reserved3[0];
			injectAddr.Socket.EndpointId = addr.Socket.EndpointId;

			if (!wd->sendPacket(packet.get(), recvLen, &sendLen, &addr)) {
				continue;
			}
		}

		while (!recved.empty())
		{
			packet.reset(recved.pop((int*)&recvLen));

			PM::changePacketSrcIP(packet.get(), secAddr);

			PM::increaseTTL(packet.get());

			if (!wd->calcualteIPChecksum(packet.get(), recvLen, &injectAddr))
				continue;

			if (!wd->sendPacket(packet.get(), recvLen, &sendLen, &injectAddr))
			{
				printf("Error injecting recved packet\n");
			}
		}
	}

	packet.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

void ServerTunnel::UDPLoop()
{
	printf("UDP loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	int bufferSize = WINDIVERT_MTU_MAX;
	int sendLen = NULL;
	int recvLen = NULL;
	struct sockaddr_in from {};
	int fromLen = sizeof(sockaddr_in);

	while (!stopTunnel)
	{
		if (!udp->recvBufferFrom(buffer.get(), bufferSize, reinterpret_cast<struct sockaddr*>(&from), &fromLen, recvLen))
		{
			break;
		}
		else if (recvLen > 0) {
			recved.push(reinterpret_cast<UINT8*>(buffer.release()), recvLen);
			buffer.reset(new char[WINDIVERT_MTU_MAX]);
		}

		while (!caught.empty())
		{
			buffer.reset(reinterpret_cast<char*>(caught.pop(&recvLen)));

			if (!udp->sendBufferTo(buffer.get(), recvLen, reinterpret_cast<sockaddr*>(&from), fromLen, sendLen))
			{
				printf("Failed to send buffer\n");
			}
		}
	}

	buffer.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

ServerTunnel::~ServerTunnel()
{
}