#pragma once

#include <regex>
#include "clientTunnel.h"
#include "serverTunnel.h"
#include "communication.h"

class VPN
{
public:
	VPN(int argc, char* argv[]);
	
	void startVPN(int argc, char* argv[]);
	void communicationLoop();
	
	~VPN();

private:
	void pipeLoop();

	bool isValidIP(const char* ipStr);
	bool isValidPort(const char* portStr);

	void stopVPN();

private:
	std::atomic<bool> comsLoop;
	std::atomic<bool> vpnLoop;
	std::atomic<byte> comsState;

	std::thread* tunnelT;
	Tunnel* vpnTunnel;
	IPCPiep* coms;
};

VPN::VPN(int argc, char* argv[])
{
	tunnelT = nullptr;
	vpnTunnel = nullptr;
	coms = new IPCPiep();

	if (argc == 5 && isValidIP(argv[2]) && isValidIP(argv[4]) && isValidPort(argv[3]))
	{
		vpnLoop = true;
	}
	else vpnLoop = false;
	
	comsState = VPN_INIT;
}

inline void VPN::startVPN(int argc, char* argv[])
{
	if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--client") == 0)
	{
		vpnTunnel = new ClientTunnel(argv);
	}
	else if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--server") == 0)
	{
		vpnTunnel = new ServerTunnel(argv);
	}
	else
	{
		comsLoop = false;
		comsState = VPN_STOP;
		return;
	}

	tunnelT = new std::thread(&Tunnel::tunnelLoop, vpnTunnel);

	comsLoop = true;
	comsState = VPN_STARED;
}

inline void VPN::communicationLoop()
{
	while (vpnLoop)
	{
		switch (comsState)
		{
		case VPN_STARED:
			pipeLoop();
			break;
		case VPN_DESTORY:
			stopVPN();
			break;
		default:
			return;
		}
	}
	comsState = VPN_ERROR;
}

void VPN::pipeLoop()
{
	printf("pipe\n");
	char* buffer = new char[5000];
	size_t bufferSize = 5000;
	DWORD readLen = NULL;
	DWORD writeLen = NULL;

	while (comsLoop)
	{
		if (!coms->pipeRead(buffer, bufferSize, &readLen))
		{
			comsLoop = false;
			comsState = VPN_DESTORY;
		}


	}
}

inline void VPN::stopVPN()
{
	printf("stoping VPN\n");
	comsState = VPN_STOP;
	comsLoop = false;
	vpnLoop = false;
	vpnTunnel->stopLoop();
	if (tunnelT->joinable())
	{
		tunnelT->join();
	}
	delete tunnelT;
	delete vpnTunnel;
}

inline bool VPN::isValidIP(const char* ipStr) {
	static const std::regex ipv4Pattern{ "^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}$" };
	return std::regex_match(ipStr, ipv4Pattern);
}

inline bool VPN::isValidPort(const char* portStr) {
	int port = std::atoi(portStr);
	return (port >= 1 && port <= 65535);
}

VPN::~VPN()
{
	delete coms;
	delete tunnelT;
	delete vpnTunnel;
}