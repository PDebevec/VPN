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

	void handleComms(char*);

	bool isValidIP(const char* ipStr);
	bool isValidPort(const char* portStr);

	void stopVPN();

private:
	std::atomic<bool> comsLoop;
	std::atomic<bool> vpnLoop;
	std::atomic<byte> comsState;

	std::thread* tunnelT;
	Tunnel* vpnTunnel;
	IPCPipe* coms;
};

VPN::VPN(int argc, char* argv[])
{
	vpnLoop = false;
	tunnelT = nullptr;
	vpnTunnel = nullptr;
	coms = new IPCPipe();
	
	if (argc >= 4 && isValidIP(argv[2]) && isValidPort(argv[3]))
	{
		vpnLoop = true;
		comsLoop = true;
	}
	else throw "Invalid arguments!";

	comsState = VPN_INIT;
}

inline void VPN::startVPN(int argc, char* argv[])
{
	printf("starting VPN\n");
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

	comsState = VPN_STARTED;
}

inline void VPN::communicationLoop()
{
	printf("comms loop\n");
	while (vpnLoop)
	{
		switch (comsState)
		{
		case VPN_STARTED:
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

void VPN::handleComms(char* buffer)
{
	static byte init = 0x0;

	std::cout << buffer << std::endl;

	if (!init && std::strcmp(buffer, "ACK") == 0)
	{
		init++;
	}
	else if (init)
	{
		if (isValidIP(buffer + 64))
		{
			vpnTunnel->newConnection(buffer+64, buffer);
			return;
		}

		if (std::strncmp(buffer, "FIN", 3) == 0)
		{
			vpnTunnel->closeConnection(buffer + 3);
			return;
		}

		init = 0x1;
		strcpy_s(buffer, 128, "RST\0");
	}
}

void VPN::pipeLoop()
{
	printf("pipe loop\n");
	char* buffer = new char[128];
	DWORD bufferSize = 128;
	DWORD readLen = NULL;
	DWORD writeLen = NULL;

	while (comsLoop)
	{
		if (!coms->pipeRead(buffer, bufferSize, &readLen))
		{
			comsLoop = false;
			comsState = VPN_DESTORY;
			return;
		}

		handleComms(buffer);

		coms->pipeWrite(buffer, (DWORD)strlen(buffer), &writeLen);
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