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

	void WDLoop(CicrularBuffer*, CicrularBuffer*) override;
	void injectLoop(std::atomic<WINDIVERT_ADDRESS>& injectAddr, CicrularBuffer*);

	void UDPLoop(CicrularBuffer*, CicrularBuffer*) override;
	void sendLoop(std::atomic<struct sockaddr>& from, CicrularBuffer*);
private:
	UINT8* secAddr;

	bool stopServer;
};

ServerTunnel::ServerTunnel(char* argv[])
	:Tunnel(argv)
{
	secAddr = nullptr;
	stopServer = true;
}

void ServerTunnel::initTunnel()
{
	printf("tunnel init\n");
	if (udp == nullptr && wd == nullptr)
	{
		udp = new UDPSocket(arg);
		wd = new BaseWinDivert("inbound and !loopback and ip.SrcAddr != 0.0.0.0 and !impostor", 0);
	}

	udp->initUDPServer();

	if (*udp->getUDPState() != UDP_INITIALIZED)
		throw "Erorr initializing UDP socket!";

	stopServer = false;

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
		stopServer = false;
	}
}

void ServerTunnel::closeConnection(char* secondary)
{
	printf("user disconnected\n");
	stopServer = true;
	wd->closeWinDivert();
	udp->stopUDPSocket();
}

void ServerTunnel::destroyTunnel()
{
	printf("destroying\n");
	wd->closeWinDivert();
	udp->stopUDPSocket();

	for (std::thread* t : tVec)
	{
		if (t->joinable())
		{
			t->join();
			delete t;
		}
	}
	tVec.clear();

	delete encKey;
	delete decKey;
	delete[] secAddr;

	tunnelState = TUNNEL_INITIALIZED;
	switchState = INIT_STATE;
}

void ServerTunnel::WDLoop(CicrularBuffer* caught, CicrularBuffer* recved) {
	printf("WD loop\n");
	std::unique_ptr<UINT8[]> packets(new UINT8[WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX]);
	std::unique_ptr<WINDIVERT_ADDRESS[]> addrs(new WINDIVERT_ADDRESS[WINDIVERT_BATCH_MAX]);
	UINT packetLen = WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX;
	UINT addrLen = sizeof(WINDIVERT_ADDRESS) * WINDIVERT_BATCH_MAX;
	UINT recvLen = 0;
	UINT packetsCaught = 0;
	std::atomic<WINDIVERT_ADDRESS> injectAddr;
	WINDIVERT_ADDRESS temp{};
	injectAddr.store(temp);
	temp.Outbound = 1;

	std::thread* injectThread = new std::thread(&ServerTunnel::injectLoop, this, std::ref(injectAddr), recved);

	while (!stopServer)
	{
		if (!wd->catchPackets(packets.get(), packetLen, &recvLen, addrs.get(), &addrLen))
		{
			break;
		}

		packetsCaught = addrLen / sizeof(WINDIVERT_ADDRESS);

		temp.Timestamp = addrs[static_cast<size_t>(packetsCaught) - 1].Timestamp;

		UINT nextPacket = 0;
		UINT singleLen = 0;

		for (size_t i = 0; i < packetsCaught; i++)
		{
			singleLen = (packets.get()[2 + nextPacket] << 8) | packets.get()[3 + nextPacket];

			if (PM::isDstIP(packets.get() + nextPacket, secAddr))
			{
				caught->push(packets.get() + nextPacket, singleLen);

				std::memmove(packets.get() + nextPacket, packets.get() + nextPacket + singleLen, recvLen - nextPacket - singleLen);

				std::memmove(&addrs[i], &addrs[i + 1], (packetsCaught - i - 1) * sizeof(WINDIVERT_ADDRESS));

				packetsCaught--;
				recvLen -= singleLen;
				continue;
			}
			else
			{
				temp.Flow.EndpointId = addrs[i].Flow.EndpointId;
				temp.Network.IfIdx = addrs[i].Network.IfIdx;
				temp.Reflect.Timestamp = addrs[i].Reflect.Timestamp;
				temp.Reserved3[0] = addrs[i].Reserved3[0];
				temp.Socket.EndpointId = addrs[i].Socket.EndpointId;

				//wd->sendPacket(packets.get() + nextPacket, singleLen, nullptr, &addrs[i]);
			}

			nextPacket += singleLen;
		}

		injectAddr.store(temp);

		if (packetsCaught > 0)
		{
			wd->injectPackets(packets.get(), recvLen, NULL, addrs.get(), packetsCaught * sizeof(WINDIVERT_ADDRESS));
		}
	}

	stopServer = true;
	recved->stopWait();

	if (injectThread->joinable())
	{
		injectThread->join();
	}
	delete injectThread;

	delete recved;
	packets.reset();
	addrs.reset();
}

void ServerTunnel::injectLoop(std::atomic<WINDIVERT_ADDRESS>& injectAddr, CicrularBuffer* recved)
{
	printf("inject loop\n");
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> batchPacket(new UINT8[WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX]);
	std::unique_ptr<UINT8[]> decPacket(new UINT8[WINDIVERT_MTU_MAX]);
	WINDIVERT_ADDRESS* batchAddr = new WINDIVERT_ADDRESS[WINDIVERT_BATCH_MAX];
	WINDIVERT_ADDRESS temp{};
	UINT recvLen = NULL;

	while (!stopServer)
	{
		if (batchAddr[0].Reserved3[0] != injectAddr.load().Reserved3[0])
		{
			temp = injectAddr.load();
			std::memset(batchAddr, 0, sizeof(WINDIVERT_ADDRESS) * WINDIVERT_BATCH_MAX);
			for (size_t i = 0; i < WINDIVERT_BATCH_MAX; i++)
			{
				batchAddr[i].Flow.EndpointId = temp.Flow.EndpointId;
				batchAddr[i].Network.IfIdx = temp.Network.IfIdx;
				batchAddr[i].Reflect.Timestamp = temp.Reflect.Timestamp;
				batchAddr[i].Reserved3[0] = temp.Reserved3[0];
				batchAddr[i].Socket.EndpointId = temp.Socket.EndpointId;
				batchAddr[i].Outbound = 1;
			}
		}

		UINT packetNum = 0;
		UINT batchLen = 0;

		while (!recved->empty())
		{
			do {
				if (!recved->pop(packet.get(), recvLen))
				{
					break;
				}
				packetNum++;

				PM::aes_decrypt(packet.get(), (int&)recvLen, decKey, batchPacket.get() + batchLen, (int&)recvLen);

				PM::changePacketSrcIP(batchPacket.get() + batchLen, secAddr);

				//PM::increaseTTL(batchPacket.get() + batchLen);

				wd->calcualteIPChecksum(batchPacket.get() + batchLen, recvLen, &batchAddr[packetNum]);

				batchLen += recvLen;
			} while (!recved->empty() && packetNum < 255);

			if (packetNum == 0)
			{
				break;
			}

			if (!wd->injectPackets(batchPacket.get(), batchLen, NULL, batchAddr, packetNum * sizeof(WINDIVERT_ADDRESS))) {
			}

			packetNum = 0;
			batchLen = 0;
		}

		recved->wait();
	}

	delete[] batchAddr;
	packet.reset();
	decPacket.reset();
	batchPacket.reset();
}

void ServerTunnel::UDPLoop(CicrularBuffer* caught, CicrularBuffer* recved) {
	printf("UDP loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	int bufferSize = WINDIVERT_MTU_MAX;
	std::atomic<struct sockaddr> from;
	sockaddr temp = *(sockaddr*)udp->getSocketAddr();
	from.store(temp);
	int fromLen = sizeof(sockaddr_in);
	int recvLen = NULL;

	std::thread* sendThread = new std::thread(&ServerTunnel::sendLoop, this, std::ref(from), caught);

	while (!stopServer)
	{
		if (!udp->recvBufferFrom(buffer.get(), bufferSize, &temp, &fromLen, recvLen))
		{
			break;
		}

		from.store(temp);

		recved->push(reinterpret_cast<UINT8*>(buffer.get()), recvLen);
	}

	stopServer = true;
	caught->stopWait();

	if (sendThread->joinable())
	{
		sendThread->join();
	}
	delete sendThread;

	delete caught;
	buffer.reset();
}

void ServerTunnel::sendLoop(std::atomic<struct sockaddr>& from, CicrularBuffer* caught)
{
	printf("send loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<char[]> encBuffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> iv(new UINT8[AES_BLOCK_SIZE]);
	sockaddr temp{};
	int recvLen = NULL;
	int fromLen = sizeof(sockaddr_in);
	int sendLen = NULL;

	while (!stopServer)
	{
		temp = from.load();

		caught->wait();

		while (!caught->empty())
		{
			caught->pop((UINT8*)buffer.get(), (unsigned int&)recvLen);

			PM::aes_encrypt(reinterpret_cast<UINT8*>(buffer.get()), recvLen, encKey, iv.get(), reinterpret_cast<UINT8*>(encBuffer.get()), recvLen);

			udp->sendBufferTo(encBuffer.get(), recvLen, &temp, fromLen, sendLen);
		}
	}

	buffer.reset();
	encBuffer.reset();
	iv.reset();
}

ServerTunnel::~ServerTunnel()
{
	delete[] secAddr;
}