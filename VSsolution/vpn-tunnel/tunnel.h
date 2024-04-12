#pragma once

#include <vector>
#include <queue>
#include "safeQueue.h"
#include "UDPSocket.h"
#include "baseWinDivert.h"
#include "codes.h"
#include "packetManipulation.h"

class Tunnel
{
public:
	Tunnel(char* argv[]);

	void tunnelLoop();
	const std::atomic<byte>* getTunnelState();
	virtual void newConnection(char* secondary, char* keys) {};
	void stopLoop();

	~Tunnel();

private:
	virtual void initTunnel() {};
	virtual void destroyTunnel() {};

	virtual void WDLoop() {};
	virtual void UDPLoop() {};

protected:
	char** arg;
	byte* secAddr;
	byte* servAddr;

	UINT8* encKey;
	UINT8* decKey;

	UDPSocket* udp;
	BaseWinDivert* wd;

protected:
	std::atomic<bool> stopTunnel;
	std::atomic<byte> switchState;
	std::atomic<byte> tunnelState;

	std::vector<std::thread*> tVec;

	SafeQueue caught;
	SafeQueue recved;
};

Tunnel::Tunnel(char* argv[])
{
	arg = argv;
	tunnelState = INIT_STATE;
	stopTunnel = true;
	udp = nullptr;
	wd = nullptr;
	secAddr = nullptr;
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
			WDLoop();
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

inline const std::atomic<byte>* Tunnel::getTunnelState()
{
	return &tunnelState;
}

inline void Tunnel::stopLoop()
{
	stopTunnel = true;
	wd->closeWinDivert();
	udp->stopUDPSocket();
}

Tunnel::~Tunnel()
{
	delete udp;
	delete wd;
	delete[] secAddr;
	for (auto* t : tVec)
	{
		t->join();
	}
	tVec.clear();
}
