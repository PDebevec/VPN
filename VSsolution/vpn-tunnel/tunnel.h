#pragma once

#include <vector>
#include <functional>
#include "circularBuffer.h"
#include "UDPSocket.h"
#include "baseWinDivert.h"
#include "codes.h"
#include "packetManipulation.h"

constexpr unsigned short TUNNEL_BATCH_SIZE = 512;
constexpr unsigned short TUNNEL_MTU_SIZE = 1500 * 2 + 40;

class Tunnel
{
public:
	Tunnel(char* argv[]);

	void tunnelLoop();
	const std::atomic<byte>* getTunnelState();
	virtual void newConnection(char* secondary, char* keys) {};
	virtual void closeConnection(char* secondary) {};
	void stopLoop();

	~Tunnel();

private:
	virtual void initTunnel() {};
	virtual void destroyTunnel() {};

	void threadLoop();

	virtual void WDLoop(CicrularBuffer*, CicrularBuffer*) {};
	
	virtual void UDPLoop(CicrularBuffer*, CicrularBuffer*) {};

protected:
	char** arg;
	UINT8* servAddr;

	UINT8* encKey;
	UINT8* decKey;

	UDPSocket* udp;
	BaseWinDivert* wd;

protected:
	std::atomic<bool> stopTunnel;
	std::atomic<byte> switchState;
	std::atomic<byte> tunnelState;

	std::vector<std::thread*> tVec;
};

Tunnel::Tunnel(char* argv[])
{
	arg = argv;
	tunnelState = INIT_STATE;
	stopTunnel = true;
	udp = nullptr;
	wd = nullptr;
	encKey = nullptr;
	decKey = nullptr;
	
	char* copyPtr = new char[strlen(argv[2]) + 1];
	strcpy_s(copyPtr, strlen(argv[2]) + 1, argv[2]);
	servAddr = PM::ipStringToArray(copyPtr);
	delete[] copyPtr;
}

void Tunnel::tunnelLoop()
{
	stopTunnel = false;

	while (!stopTunnel)
	{
		switch (switchState)
		{
		case TUNNEL_INIT:
			initTunnel();
			break;
		case TUNNEL_LOOP:
			threadLoop();
			break;
		case TUNNEL_DESTORY:
			destroyTunnel();
			break;
		default:
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}

	stopTunnel = true;
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_STOP;
}

void Tunnel::threadLoop()
{
	unsigned int threadCount = std::thread::hardware_concurrency();

	if(threadCount > 3)
	{
		CicrularBuffer* t1c = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
		CicrularBuffer* t1r = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
		tVec.push_back(new std::thread(std::bind(&Tunnel::UDPLoop, this, t1c, t1r)));
		tVec.push_back(new std::thread(std::bind(&Tunnel::WDLoop, this, t1c, t1r)));
		if (threadCount > 5)
		{
			t1c = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
			t1r = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
			tVec.push_back(new std::thread(std::bind(&Tunnel::UDPLoop, this, t1c, t1r)));
			tVec.push_back(new std::thread(std::bind(&Tunnel::WDLoop, this, t1c, t1r)));
			/*if (threadCount > 5)
			{
				t1c = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
				t1r = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
				tVec.push_back(new std::thread(std::bind(&Tunnel::UDPLoop, this, t1c, t1r)));
				tVec.push_back(new std::thread(std::bind(&Tunnel::WDLoop, this, t1c, t1r)));
			}*/
		}
	}

	CicrularBuffer* tc = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
	CicrularBuffer* tr = new CicrularBuffer(TUNNEL_BATCH_SIZE, TUNNEL_MTU_SIZE);
	tVec.push_back(new std::thread(std::bind(&Tunnel::UDPLoop, this, tc, tr)));

	WDLoop(tc, tr);

	switchState = TUNNEL_DESTORY;
}

inline const std::atomic<byte>* Tunnel::getTunnelState()
{
	return &tunnelState;
}

inline void Tunnel::stopLoop()
{
	wd->closeWinDivert();
	system("sc stop windivert");
	udp->stopUDPSocket();

	switchState = TUNNEL_DESTORY;
}

Tunnel::~Tunnel()
{
	delete udp;
	delete wd;

	for (auto* t : tVec)
	{
		t->join();
	}

	tVec.clear();
}
