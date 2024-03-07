#pragma once

#include "tunnel.h"

class ClientTunnel : public Tunnel
{
public:
	ClientTunnel(char* argv[]);
	~ClientTunnel();

private:
	void initTunnel() override;
	void connectTunnel() override;
	void destroyTunnel() override;

	void WDLoop() override;
	void TCPLoop() override;
	void UDPLoop() override;
};

ClientTunnel::ClientTunnel(char* argv[])
	:Tunnel(argv)
{
	if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--client") == 0)
	{
		switchState = TUNNEL_INIT;
	}
	else {
		tunnelState = TUNNEL_ERROR;
	}
}

void ClientTunnel::initTunnel()
{
	printf("initializing client\n");
	tcp = new TCPSocket(arg);
	udp = new UDPSocket(arg);
	wd = new BaseWinDivert("!loopback and !icmp", 0);

	tcp->initTCPClient();
	udp->initUDPClient();

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_CONNECT;
}

void ClientTunnel::connectTunnel()
{
	printf("connecting client\n");
	tcp->connectClient();

	if (*tcp->getTCPState() != TCP_CONNECTED)
		throw "Error connecting socket!";

	tunnelState = TUNNEL_CONNECTED;
	switchState = TUNNEL_START;
}

void ClientTunnel::destroyTunnel()
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
	recved.clear();
	caught.clear();

	stopTunnel = true;
	switchState = TUNNEL_STOP;
	tunnelState = TUNNEL_STOP;
}

void ClientTunnel::WDLoop()
{
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	UINT packetSize = WINDIVERT_MTU_MAX;
	memset(packet.get(), 0, packetSize);
	UINT recvLen = NULL;
	UINT sendLen = NULL;
	WINDIVERT_ADDRESS addr{};
	WINDIVERT_ADDRESS injectAddr{};
	injectAddr.Outbound = 1;

	while (!stopTunnel)
	{
		printf(" wd loop ");
		if (!wd->recvPacket(packet.get(), packetSize, &recvLen, &addr))
		{
			break;
		}

		if (addr.IPv6)
		{
		}
		else if (addr.Outbound)
		{
			printf(" pushed to queue ");
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
			//injectAddr.Timestamp = addr.Timestamp; //+50000

			wd->sendPacket(packet.get(), recvLen, &sendLen, &addr);
		}

		while (!recved.empty())
		{
			packet.reset(recved.pop(recvLen));

			PM::changePacketDstIP(packet.get(), secAddr);
			PM::displayIPv4HeaderInfo(packet.get());

			PM::increaseTTL(packet.get());

			if (WinDivertHelperCalcChecksums(packet.get(), recvLen, &injectAddr,
				WINDIVERT_HELPER_NO_ICMP_CHECKSUM || WINDIVERT_HELPER_NO_ICMPV6_CHECKSUM) == FALSE)
			{
				printf("calc check sum failed %i", GetLastError());
				continue;
			}

			if (!wd->sendPacket(packet.get(), recvLen, &sendLen, &injectAddr))
			{
				printf("Error injecting recved packet\n");
			}
		}
		memset(packet.get(), 0, packetSize);
	}

	packet.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

void ClientTunnel::TCPLoop()
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

void ClientTunnel::UDPLoop()
{
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	int bufferSize = WINDIVERT_MTU_MAX;
	memset(buffer.get(), 0, bufferSize);
	int sendLen = NULL;
	UINT recvLen = NULL;
	struct sockaddr from = *reinterpret_cast<struct sockaddr*>(udp->getSocketAddr());
	int fromLen = sizeof(sockaddr_in);

	while (!stopTunnel)
	{
		printf(" udp loop ");
		if (!udp->recvBufferFrom(buffer.get(), bufferSize, &from, &fromLen, recvLen))
		{
			break;
		}
		else if (recvLen > 0) {
			recved.push(reinterpret_cast<UINT8*>(buffer.release()), recvLen);
			buffer.reset(new char[WINDIVERT_MTU_MAX]);
		}

		while (!caught.empty())
		{
			buffer.reset(reinterpret_cast<char*>(caught.pop(recvLen)));

			if (!udp->sendBufferTo(buffer.get(), recvLen, &from, fromLen, sendLen))
			{
				printf("Failed to send buffer\n");
			}
		}
		memset(buffer.get(), 0, bufferSize);
	}

	buffer.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

ClientTunnel::~ClientTunnel()
{
}
