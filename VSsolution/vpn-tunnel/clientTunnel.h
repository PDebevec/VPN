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

	void WDLoop(CicrularBuffer*, CicrularBuffer*) override;
	void injectLoop(std::atomic<WINDIVERT_ADDRESS*>* injectAddr, CicrularBuffer*);

	void UDPLoop(CicrularBuffer*, CicrularBuffer*) override;
	void sendLoop(std::atomic<struct sockaddr*>* from, CicrularBuffer*);

	bool letLocalRange(UINT8*) const;
private:
	UINT8* secAddr;
	UINT8* localLow;
	UINT8* localHigh;

	bool stopClient;
};

ClientTunnel::ClientTunnel(char* argv[])
	:Tunnel(argv)
{
	secAddr = nullptr;
	localLow = PM::ipStringToArray(argv[4]);
	localHigh = PM::ipStringToArray(argv[5]);
	stopClient = true;
}

void ClientTunnel::initTunnel()
{
	printf("tunnel init\n");
	udp = new UDPSocket(arg);

	std::string temp = "outbound and !loopback and !impostor and remoteAddr != ";
	temp += arg[2];

	wd = new BaseWinDivert(temp.c_str(), 0);

	udp->initUDPClient();

	if (*udp->getUDPState() != UDP_INITIALIZED)
		throw "Error initializing UDP socket!";
	
	stopClient = false;

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
	printf("destroying\n");
	wd->closeWinDivert();
	udp->stopUDPSocket();

	stopTunnel = true;
	stopClient = true;

	for (auto *t : tVec)
	{
		if (t->joinable())
		{
			t->join();
		}
		delete t;
	}

	tVec.clear();

	switchState = TUNNEL_STOP;
	tunnelState = TUNNEL_STOP;
}

bool ClientTunnel::letLocalRange(UINT8* packet) const
{
	return memcmp(packet + 16, localLow, 4) >= 0 && memcmp(packet + 16, localHigh, 4) <= 0;
}

void ClientTunnel::WDLoop(CicrularBuffer* caught, CicrularBuffer* recved)
{
	printf("WD loop\n");
	std::unique_ptr<UINT8[]> packets(new UINT8[WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX]);
	std::unique_ptr<WINDIVERT_ADDRESS[]> addrs(new WINDIVERT_ADDRESS[WINDIVERT_BATCH_MAX]);
	UINT packetLen = WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX;
	UINT addrLen = sizeof(WINDIVERT_ADDRESS) * WINDIVERT_BATCH_MAX;
	UINT recvLen = 0;
	UINT packetsCaught = 0;
	std::atomic<WINDIVERT_ADDRESS*>* injectAddr = new std::atomic<WINDIVERT_ADDRESS*>(new WINDIVERT_ADDRESS);
	WINDIVERT_ADDRESS temp{};

	std::thread* injectThread = new std::thread(std::bind(&ClientTunnel::injectLoop, this, injectAddr, recved));

	while (!stopClient)
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

			if (addrs[i].IPv6)
			{
				std::memmove(packets.get() + nextPacket, packets.get() + nextPacket + singleLen, recvLen - nextPacket - singleLen);

				std::memmove(&addrs[i], &addrs[i + 1], (packetsCaught - i - 1) * sizeof(WINDIVERT_ADDRESS));

				packetsCaught--;
				recvLen -= singleLen;
				continue;
			}
			else if (PM::isLocalPacket(packets.get() + nextPacket) == letLocalRange(packets.get() + nextPacket))
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

		*injectAddr->load() = temp;

		if (packetsCaught > 0)
		{
			wd->injectPackets(packets.get(), recvLen, NULL, addrs.get(), packetsCaught * sizeof(WINDIVERT_ADDRESS));
		}
	}

	stopClient = true;
	caught->stopWait();
	recved->stopWait();


	if (injectThread->joinable())
	{
		injectThread->join();
	}
	delete injectThread;

	delete injectAddr;
	delete recved;
	packets.reset();
	addrs.reset();

	switchState = TUNNEL_DESTORY;
}

void ClientTunnel::injectLoop(std::atomic<WINDIVERT_ADDRESS*>* injectAddr, CicrularBuffer* recved)
{
	printf("inject loop\n");
	std::unique_ptr<UINT8[]> packet(new UINT8[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> batchPacket(new UINT8[WINDIVERT_MTU_MAX * WINDIVERT_BATCH_MAX]);
	std::unique_ptr<UINT8[]> decPacket(new UINT8[WINDIVERT_MTU_MAX]);
	WINDIVERT_ADDRESS* batchAddr = new WINDIVERT_ADDRESS[WINDIVERT_BATCH_MAX];
	WINDIVERT_ADDRESS temp{};
	UINT recvLen = NULL;

	while (!stopClient)
	{
		if (batchAddr[0].Reserved3[0] != injectAddr->load()->Reserved3[0])
		{
			temp = *injectAddr->load();
			for (size_t i = 0; i < WINDIVERT_BATCH_MAX; i++)
			{
				batchAddr[i].Flow.EndpointId = temp.Flow.EndpointId;
				batchAddr[i].Network.IfIdx = temp.Network.IfIdx;
				batchAddr[i].Reflect.Timestamp = temp.Reflect.Timestamp;
				batchAddr[i].Reserved3[0] = temp.Reserved3[0];
				batchAddr[i].Socket.EndpointId = temp.Socket.EndpointId;
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

				PM::changePacketDstIP(batchPacket.get() + batchLen, secAddr);

				//PM::increaseTTL(batchPacket.get() + batchLen);
		
				wd->calcualteIPChecksum(batchPacket.get() + batchLen, recvLen, &batchAddr[packetNum]);

				batchLen += recvLen;
			} while (!recved->empty() && packetNum < 255);

			if (packetNum == 0)
			{
				break;
			}

			wd->injectPackets(batchPacket.get(), batchLen, NULL, batchAddr, packetNum * sizeof(WINDIVERT_ADDRESS));

			packetNum = 0;
			batchLen = 0;
		}

		recved->wait();
	}

	delete[] batchAddr;
	packet.reset();
	batchPacket.reset();
	decPacket.reset();
}

void ClientTunnel::UDPLoop(CicrularBuffer* caught, CicrularBuffer* recved)
{
	printf("UDP loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	int bufferSize = WINDIVERT_MTU_MAX;
	std::atomic<struct sockaddr*>* from = new std::atomic<struct sockaddr*>(reinterpret_cast<struct sockaddr*>(udp->getSocketAddr()));
	int fromLen = sizeof(sockaddr_in);
	int recvLen = NULL;

	std::thread* sendThread = new std::thread(std::bind(&ClientTunnel::sendLoop, this, from, caught));

	while (!stopClient)
	{
		if (!udp->recvBufferFrom(buffer.get(), bufferSize, from->load(), &fromLen, recvLen))
		{
			break;
		}

		recved->push(reinterpret_cast<UINT8*>(buffer.get()), recvLen);
	}

	if (sendThread->joinable())
	{
		sendThread->join();
	}
	delete sendThread;
	
	delete from;
	delete caught;
	buffer.reset();
}

void ClientTunnel::sendLoop(std::atomic<struct sockaddr*>* from, CicrularBuffer* caught)
{
	printf("send loop\n");
	std::unique_ptr<char[]> buffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<char[]> encBuffer(new char[WINDIVERT_MTU_MAX]);
	std::unique_ptr<UINT8[]> iv(new UINT8[AES_BLOCK_SIZE]);
	int recvLen = NULL;
	int fromLen = sizeof(sockaddr_in);
	int sendLen = NULL;

	while (!stopClient)
	{
		caught->wait();

		while (!caught->empty())
		{
			caught->pop((UINT8*)buffer.get(), (unsigned int&)recvLen);

			PM::aes_encrypt(reinterpret_cast<UINT8*>(buffer.get()), recvLen, encKey, iv.get(), reinterpret_cast<UINT8*>(encBuffer.get()), recvLen);

			udp->sendBufferTo(encBuffer.get(), recvLen, from->load(), fromLen, sendLen);
		}
	}

	buffer.reset();
	encBuffer.reset();
	iv.reset();
}

ClientTunnel::~ClientTunnel()
{
	delete encKey;
	delete decKey;
	delete[] secAddr;
	delete[] localLow;
	delete[] localHigh;
}
