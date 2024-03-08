#pragma once

#include <iostream>
#include <windivert.h>
#include "codes.h"

class BaseWinDivert
{
public:
	BaseWinDivert(const char* WDfilter, UINT64 WDflag);

	const std::atomic<byte>* getState();

	bool recvPacket(void* pPacket, UINT packetLen, UINT* pRecvLen, WINDIVERT_ADDRESS* pAddr);
	bool sendPacket(const void* pPacket, UINT packetLen, UINT* pSendLen, const WINDIVERT_ADDRESS* pAddr);
	bool calcualteIPChecksum(void* pPacket, UINT packetLen, WINDIVERT_ADDRESS* pAddr);


	~BaseWinDivert();

	bool openWinDivert();
	bool closeWinDivert();

private:
	HANDLE handle;
	const char* filter;
	UINT64 flag;

	std::atomic<byte> state;
};

BaseWinDivert::BaseWinDivert(const char* WDfilter, UINT64 WDflag)
	:filter(WDfilter), flag(WDflag)
{
	handle = NULL;
	state = INIT_STATE;
}

inline bool BaseWinDivert::recvPacket(void* pPacket, UINT packetLen, UINT* pRecvLen, WINDIVERT_ADDRESS* pAddr)
{
	if (!WinDivertRecv(handle, pPacket, packetLen, pRecvLen, pAddr)) {
		std::cerr << "Error receiving packet. WD Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseWinDivert::sendPacket(const void* pPacket, UINT packetLen, UINT* pSendLen, const WINDIVERT_ADDRESS* pAddr)
{
	if (!WinDivertSend(handle, pPacket, packetLen, pSendLen, pAddr)) {
		std::cerr << "Error sending packet. WD Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseWinDivert::calcualteIPChecksum(void* pPacket, UINT packetLen, WINDIVERT_ADDRESS* pAddr)
{
	if (WinDivertHelperCalcChecksums(pPacket, packetLen, pAddr,
		WINDIVERT_HELPER_NO_ICMP_CHECKSUM || WINDIVERT_HELPER_NO_ICMPV6_CHECKSUM) == FALSE) {
		std::cerr << "Faild to calcualte checksum! WD error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseWinDivert::openWinDivert()
{
	handle = WinDivertOpen(filter, WINDIVERT_LAYER_NETWORK, WINDIVERT_PRIORITY_HIGHEST, flag);
	if (handle == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Error oppening handle. Error code: " << GetLastError() << std::endl;
		return false;
	}
	state = WD_OPENED;
	return true;
}

inline bool BaseWinDivert::closeWinDivert()
{
	if (!WinDivertClose(handle))
	{
		std::cerr << "Error closing WinDivert handle. Error code: " << GetLastError() << std::endl;
		system("sc stop windivert");
		return false;
	}
	state = WD_CLOSED;
	return true;
}

inline const std::atomic<byte>* BaseWinDivert::getState()
{
	return &state;
}

BaseWinDivert::~BaseWinDivert()
{
	WinDivertClose(handle);
	CloseHandle(handle);
	system("sc stop windivert");
	delete filter;
}