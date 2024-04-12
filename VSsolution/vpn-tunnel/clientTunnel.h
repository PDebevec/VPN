#pragma once

#include "tunnel.h"

class ClientTunnel : public Tunnel
{
public:
	ClientTunnel(char* argv[]);

	void newConnection(char*, char*) override;

	~ClientTunnel();

private:
	void initTunnel() override;
	void destroyTunnel() override;

	void WDLoop() override;
	void UDPLoop() override;
};

ClientTunnel::ClientTunnel(char* argv[])
	:Tunnel(argv)
{
}

void ClientTunnel::initTunnel()
{
	printf("tunnel init\n");
	udp = new UDPSocket(arg);

	std::string temp = "outbound and !loopback and !icmp and remoteAddr != ";
	temp += arg[2];

	wd = new BaseWinDivert(temp.c_str(), 0); //WINDIVERT_FLAG_SNIFF

	udp->initUDPClient();

	if (*udp->getUDPState() != UDP_INITIALIZED)
		throw "Error initializing UDP socket!";

	tVec.push_back(new std::thread(&ClientTunnel::UDPLoop, this));

	wd->openWinDivert();

	if (*wd->getState() != WD_OPENED)
		throw "Error opening WinDivert!";

	tunnelState = TUNNEL_INITIALIZED;
	switchState = TUNNEL_LOOP;
}

inline void ClientTunnel::newConnection(char* secondary, char* keys)
{
	secAddr = PM::ipStringToArray(secondary);

	encKey = new UINT8[32];
	std::memcpy(encKey, keys, 32);

	decKey = new UINT8[32];
	std::memcpy(decKey, keys + 32, 32);

	switchState = TUNNEL_INIT;
}

void ClientTunnel::destroyTunnel()
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
	recved.clear();
	caught.clear();

	stopTunnel = true;
	switchState = TUNNEL_STOP;
	tunnelState = TUNNEL_STOP;
}

void ClientTunnel::WDLoop()
{
	printf("WD loop\n");
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> decPacket(new UINT8[WINDIVERT_MTU_MAX]);
	UINT packetSize = WINDIVERT_MTU_MAX;
	UINT recvLen = NULL;
	UINT sendLen = NULL;
	WINDIVERT_ADDRESS addr{};
	WINDIVERT_ADDRESS injectAddr{};

	while (!stopTunnel)
	{
		if (!wd->recvPacket(packet.get(), packetSize, &recvLen, &addr))
		{
			break;
		}

		injectAddr.Timestamp = addr.Timestamp;

		if (addr.IPv6)
		{
		}
		else if (addr.Outbound && !PM::isDstIP(packet.get(), servAddr) && !PM::isLocalPacket(packet.get()))
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

			PM::aes_decrypt(packet.get(), (int&)recvLen, decKey, decPacket.get(), (int&)recvLen);

			PM::changePacketDstIP(decPacket.get(), secAddr);

			PM::increaseTTL(decPacket.get());

			if (!wd->calcualteIPChecksum(decPacket.get(), recvLen, &injectAddr))
				continue;

			if (!wd->sendPacket(decPacket.get(), recvLen, &sendLen, &injectAddr))
			{
				printf("Error injecting recved packet\n");
			}
		}
	}

	packet.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

void ClientTunnel::UDPLoop()
{
	printf("UDP loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<char[]> encBuffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> iv(new UINT8[AES_BLOCK_SIZE]);
	int bufferSize = WINDIVERT_MTU_MAX;
	int sendLen = NULL;
	int recvLen = NULL;
	struct sockaddr from = *reinterpret_cast<struct sockaddr*>(udp->getSocketAddr());
	int fromLen = sizeof(sockaddr_in);

	while (!stopTunnel)
	{
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
			buffer.reset(reinterpret_cast<char*>(caught.pop((int*)&recvLen)));

			PM::aes_encrypt(reinterpret_cast<UINT8*>(buffer.get()), recvLen, encKey, iv.get(), reinterpret_cast<UINT8*>(encBuffer.get()), recvLen);

			if (!udp->sendBufferTo(encBuffer.get(), recvLen, &from, fromLen, sendLen))
			{
				std::cerr << WSAGetLastError() << ":" << recvLen << ":" << recvLen - 16 << std::endl;
			}
		}
	}

	buffer.reset();
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_DESTORY;
}

ClientTunnel::~ClientTunnel()
{
}
