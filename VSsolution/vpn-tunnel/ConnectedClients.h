#pragma once

#include <unordered_map>
#include <thread>

struct client
{
	std::atomic<bool> stopLoop;
	uint32_t socketAddr;
	unsigned char* secondaryAddr;
	std::thread* catchThread;
	std::thread* recvThread;
};