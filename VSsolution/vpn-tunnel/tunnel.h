#pragma once

#include <vector>
#include <queue>
#include "safeQueue.h"
#include "TCPSocket.h"
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

	~Tunnel();

private:
	virtual void initTunnel() {};
	virtual void connectTunnel() {};
	void startTunnel();
	virtual void destroyTunnel() {};

	virtual void WDLoop() {};
	virtual void TCPLoop() {};
	virtual void UDPLoop() {};

protected:
	char** arg;
	byte* secAddr;

	TCPSocket* tcp;
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
	tcp = nullptr;
	udp = nullptr;
	wd = nullptr;
	secAddr = PM::ipStringToArray(argv[4]);
}

void Tunnel::tunnelLoop()
{
	if (tunnelState == TUNNEL_ERROR)
	{
		throw "Initialization error!";
		return;
	}

	stopTunnel = false;

	while (!stopTunnel)
	{
		switch (switchState)
		{
		case TUNNEL_INIT:
			initTunnel();
			break;
		case TUNNEL_CONNECT:
			connectTunnel();
			break;
		case TUNNEL_START:
			startTunnel();
			break;
		case TUNNEL_LOOP:
			TCPLoop();
			break;
		case TUNNEL_DESTORY:
			destroyTunnel();
			break;
		}
	}

	stopTunnel = true;
	switchState = TUNNEL_DESTORY;
	tunnelState = TUNNEL_STOP;
}

void Tunnel::startTunnel()
{
	printf("starting\n");
	if (!wd->openWinDivert())
		throw "Failed to open Windivert!";

	tVec.push_back(new std::thread(&Tunnel::WDLoop, this));

	if (*udp->getUDPState() != UDP_INITIALIZED)
		throw "Failed to start UDP socket!";

	tVec.push_back(new std::thread(&Tunnel::UDPLoop, this));

	tunnelState = TUNNEL_STARTED;
	switchState = TUNNEL_LOOP;
}

inline const std::atomic<byte>* Tunnel::getTunnelState()
{
	return &tunnelState;
}

Tunnel::~Tunnel()
{
	delete tcp;
	delete udp;
	delete wd;
	delete[] secAddr;
	for (auto* t : tVec)
	{
		t->join();
	}
	tVec.clear();
}
