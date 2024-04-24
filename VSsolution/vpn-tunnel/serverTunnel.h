#pragma once

#include <unordered_set>
#include "tunnel.h"

class ServerTunnel : public Tunnel
{
public:
	ServerTunnel(char* argv[]);

	void newConnection(char* secondary, char* keys) override;
	void closeConnection(char* secondary) override;

	~ServerTunnel();

private:
	void initTunnel() override;
	void destroyTunnel() override;

	void WDLoop() override;
	void injectLoop(std::atomic<WINDIVERT_ADDRESS*>* injectAddr);

	void UDPLoop() override;
	void sendLoop(std::atomic<struct sockaddr*>* from);
private:
	UINT8* secAddr;
};

ServerTunnel::ServerTunnel(char* argv[])
	:Tunnel(argv)
{
	secAddr = nullptr;
}

void ServerTunnel::initTunnel()
{
	printf("tunnel init\n");
	udp = new UDPSocket(arg);
	wd = new BaseWinDivert("inbound and !loopback and !icmp", 0); //WINDIVERT_FLAG_SNIFF

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

inline void ServerTunnel::newConnection(char* secondary, char* keys)
{
	if (switchState != TUNNEL_LOOP)
	{
		secAddr = PM::ipStringToArray(secondary);

		decKey = new UINT8[32];
		std::memcpy(decKey, keys, 32);

		encKey = new UINT8[32];
		std::memcpy(encKey, keys + 32, 32);

		switchState = TUNNEL_INIT;
	}

}

void ServerTunnel::closeConnection(char* secondary)
{
	std::cout << secondary << std::endl;
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

void ServerTunnel::WDLoop() {
	printf("WD loop\n");
	std::unique_ptr<UINT8[]> packets(new UINT8[WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX]);
	std::unique_ptr<WINDIVERT_ADDRESS[]> addrs(new WINDIVERT_ADDRESS[WINDIVERT_BATCH_MAX]);
	UINT packetLen = WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX;
	UINT addrLen = sizeof(WINDIVERT_ADDRESS) * WINDIVERT_BATCH_MAX;
	UINT recvLen = 0;
	UINT packetsCaught = 0;
	std::atomic<WINDIVERT_ADDRESS*>* injectAddr = new std::atomic<WINDIVERT_ADDRESS*>(new WINDIVERT_ADDRESS);
	injectAddr->load()->Outbound = 1;

	std::thread* injectThread = new std::thread(std::bind(&ServerTunnel::injectLoop, this, injectAddr));

	while (!stopTunnel)
	{
		if (!wd->catchPackets(packets.get(), packetLen, &recvLen, addrs.get(), &addrLen))
		{
			break;
		}

		packetsCaught = addrLen / sizeof(WINDIVERT_ADDRESS);

		injectAddr->load()->Timestamp = addrs[static_cast<size_t>(packetsCaught) - 1].Timestamp;

		UINT nextPacket = 0;
		UINT singleLen = 0;

		for (size_t i = 0; i < packetsCaught; i++)
		{
			singleLen = (packets.get()[2 + nextPacket] << 8) | packets.get()[3 + nextPacket];
			
			//if (!addrs[i].Outbound && PM::isDstIP(packets.get() + nextPacket, secAddr))
			if (PM::isDstIP(packets.get() + nextPacket, secAddr))
			{
				caught.push(PM::getSinglePacket(packets.get() + nextPacket, singleLen), singleLen);
				//caught.push(packet.release(), recvLen);
				//packet.reset(new UINT8[WINDIVERT_MTU_MAX]);
			}
			else
			{
				injectAddr->load()->Flow.EndpointId = addrs[i].Flow.EndpointId;
				injectAddr->load()->Network.IfIdx = addrs[i].Network.IfIdx;
				injectAddr->load()->Reflect.Timestamp = addrs[i].Reflect.Timestamp;
				injectAddr->load()->Reserved3[0] = addrs[i].Reserved3[0];
				injectAddr->load()->Socket.EndpointId = addrs[i].Socket.EndpointId;

				if (!wd->sendPacket(packets.get() + nextPacket, singleLen, nullptr, &addrs[i])) {
				}
			}

			nextPacket += singleLen;
		}
	}

	if (injectThread->joinable())
	{
		injectThread->join();
	}

	delete injectThread;
	delete injectAddr;
	packets.reset();
	addrs.reset();
}

void ServerTunnel::injectLoop(std::atomic<WINDIVERT_ADDRESS*>* injectAddr)
{
	printf("inject loop\n");
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> decPacket(new UINT8[WINDIVERT_MTU_MAX]);
	UINT recvLen = NULL;

	while (!stopTunnel)
	{
		recved.wait();

		while (!recved.empty())
		{
			packet.reset(recved.pop((int*)&recvLen));

			PM::aes_decrypt(packet.get(), (int&)recvLen, decKey, decPacket.get(), (int&)recvLen);
		
			PM::changePacketSrcIP(decPacket.get(), secAddr);
		
			PM::increaseTTL(decPacket.get());

			if (!wd->calcualteIPChecksum(decPacket.get(), recvLen, injectAddr->load())) {
				printf("ip check sum failed\n");
			}

			if (!wd->sendPacket(decPacket.get(), recvLen, nullptr, injectAddr->load()))
			{
				printf("Error injecting recved packet\n");
			}
		}
	}

	packet.reset();
	decPacket.reset();
}

//void ServerTunnel::WDLoop()
//{
//	printf("WD loop\n");
//	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
//	std::unique_ptr<UINT8[]> decPacket(new UINT8[WINDIVERT_MTU_MAX]);
//	UINT packetSize = WINDIVERT_MTU_MAX;
//	UINT recvLen = NULL;
//	UINT sendLen = NULL;
//	WINDIVERT_ADDRESS addr{};
//	WINDIVERT_ADDRESS injectAddr{};
//	injectAddr.Outbound = 1;
//
//	while (!stopTunnel)
//	{
//		if (!wd->recvPacket(packet.get(), packetSize, &recvLen, &addr))
//		{
//			break;
//		}
//
//		injectAddr.Timestamp = addr.Timestamp;
//
//		if (!addr.Outbound && PM::isDstIP(packet.get(), secAddr))
//		{
//			caught.push(packet.release(), recvLen);
//			packet.reset(new UINT8[WINDIVERT_MTU_MAX]);
//		}
//		else
//		{
//			injectAddr.Flow.EndpointId = addr.Flow.EndpointId;
//			injectAddr.Network.IfIdx = addr.Network.IfIdx;
//			injectAddr.Reflect.Timestamp = addr.Reflect.Timestamp;
//			injectAddr.Reserved3[0] = addr.Reserved3[0];
//			injectAddr.Socket.EndpointId = addr.Socket.EndpointId;
//
//			if (!wd->sendPacket(packet.get(), recvLen, &sendLen, &addr)) {
//				continue;
//			}
//		}
//
//		while (!recved.empty())
//		{
//			packet.reset(recved.pop((int*)&recvLen));
//
//			PM::aes_decrypt(packet.get(), (int&)recvLen, decKey, decPacket.get(), (int&)recvLen);
//
//			PM::changePacketSrcIP(decPacket.get(), secAddr);
//
//			PM::increaseTTL(decPacket.get());
//
//			if (!wd->calcualteIPChecksum(decPacket.get(), recvLen, &injectAddr))
//				continue;
//
//			if (!wd->sendPacket(decPacket.get(), recvLen, &sendLen, &injectAddr))
//			{
//				printf("Error injecting recved packet\n");
//			}
//		}
//	}
//
//	packet.reset();
//	switchState = TUNNEL_DESTORY;
//	tunnelState = TUNNEL_DESTORY;
//}

void ServerTunnel::UDPLoop() {
	printf("UDP loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	int bufferSize = WINDIVERT_MTU_MAX;
	std::atomic<struct sockaddr*>* from = new std::atomic<struct sockaddr*>(reinterpret_cast<struct sockaddr*>(udp->getSocketAddr()));
	int fromLen = sizeof(sockaddr_in);
	int recvLen = NULL;

	std::thread* sendThread = new std::thread(std::bind(&ServerTunnel::sendLoop, this, from));

	while (!stopTunnel)
	{
		if (!udp->recvBufferFrom(buffer.get(), bufferSize, from->load(), &fromLen, recvLen))
		{
			break;
		}

		recved.push(reinterpret_cast<UINT8*>(buffer.release()), recvLen);
		buffer.reset(new char[WINDIVERT_MTU_MAX]);
	}

	if (sendThread->joinable())
	{
		sendThread->join();
	}

	delete sendThread;
	buffer.reset();
}

void ServerTunnel::sendLoop(std::atomic<struct sockaddr*>* from)
{
	printf("send loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<char[]> encBuffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> iv(new UINT8[AES_BLOCK_SIZE]);
	int recvLen = NULL;
	int fromLen = sizeof(sockaddr_in);
	int sendLen = NULL;

	while (!stopTunnel)
	{
		caught.wait();

		while (!caught.empty())
		{
			buffer.reset(reinterpret_cast<char*>(caught.pop(&recvLen)));

			PM::aes_encrypt(reinterpret_cast<UINT8*>(buffer.get()), recvLen, encKey, iv.get(), reinterpret_cast<UINT8*>(encBuffer.get()), recvLen);

			if (!udp->sendBufferTo(encBuffer.get(), recvLen, from->load(), fromLen, sendLen))
			{
				std::cerr << WSAGetLastError() << ":" << recvLen << ":" << recvLen - 16 << std::endl;
			}
		}
	}

	buffer.reset();
	encBuffer.reset();
	iv.reset();
}

//void ServerTunnel::UDPLoop()
//{
//	printf("UDP loop\n");
//	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
//	std::unique_ptr<char[]> encBuffer(new char[WINDIVERT_MTU_MAX]);
//	std::unique_ptr<UINT8[]> iv(new UINT8[AES_BLOCK_SIZE]);
//	int bufferSize = WINDIVERT_MTU_MAX;
//	int sendLen = NULL;
//	int recvLen = NULL;
//	struct sockaddr_in from {};
//	int fromLen = sizeof(sockaddr_in);
//
//	while (!stopTunnel)
//	{
//		if (!udp->recvBufferFrom(buffer.get(), bufferSize, reinterpret_cast<struct sockaddr*>(&from), &fromLen, recvLen))
//		{
//			break;
//		}
//		else if (recvLen > 0) {
//			recved.push(reinterpret_cast<UINT8*>(buffer.release()), recvLen);
//			buffer.reset(new char[WINDIVERT_MTU_MAX]);
//		}
//		
//		while (!caught.empty())
//		{
//			buffer.reset(reinterpret_cast<char*>(caught.pop(&recvLen)));
//
//			PM::aes_encrypt(reinterpret_cast<UINT8*>(buffer.get()), recvLen, encKey, iv.get(), reinterpret_cast<UINT8*>(encBuffer.get()), recvLen);
//
//			if (!udp->sendBufferTo(encBuffer.get(), recvLen, reinterpret_cast<sockaddr*>(&from), fromLen, sendLen))
//			{
//				std::cerr << WSAGetLastError() << ":" << recvLen << ":" << recvLen - 16 << std::endl;
//			}
//		}
//	}
//
//	buffer.reset();
//	switchState = TUNNEL_DESTORY;
//	tunnelState = TUNNEL_DESTORY;
//}

ServerTunnel::~ServerTunnel()
{
}