#pragma once

#include <iostream>
#include <windivert.h>

class BaseWinDivert
{
public:
	BaseWinDivert(const char* WDfilter, UINT64 WDflag);

	bool recvPacket(void* pPacket, UINT packetLen, UINT* pRecvLen, WINDIVERT_ADDRESS* pAddr);

	bool sendPacket(const void* pPacket, UINT packetLen, UINT* pSendLen, const WINDIVERT_ADDRESS* pAddr);

	~BaseWinDivert();

protected:
	bool openWinDivert();
	bool closeWinDivert();

private:
	HANDLE handle;
	const char* filter;
	UINT64 flag;
};

BaseWinDivert::BaseWinDivert(const char* WDfilter, UINT64 WDflag)
	:filter(WDfilter), flag(WDflag)
{
	handle = NULL;
}

inline bool BaseWinDivert::recvPacket(void* pPacket, UINT packetLen, UINT* pRecvLen, WINDIVERT_ADDRESS* pAddr)
{
	if (!WinDivertRecv(handle, pPacket, packetLen, pRecvLen, pAddr)) {
		std::cerr << "Error receiving packet. Error code: " << GetLastError() << std::endl;
		system("pause");
		return false;
	}
	return true;
}

inline bool BaseWinDivert::sendPacket(const void* pPacket, UINT packetLen, UINT* pSendLen, const WINDIVERT_ADDRESS* pAddr)
{
	if (!WinDivertSend(handle, pPacket, packetLen, pSendLen, pAddr)) {
		std::cerr << "Error sending packet. Error code: " << GetLastError() << std::endl;
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
	else return true;
}

inline bool BaseWinDivert::closeWinDivert()
{
	if (!WinDivertClose(handle))
	{
		std::cerr << "Error closing WinDivert handle. Error code: " << GetLastError() << std::endl;
		return false;
	}
	else return true;
}

BaseWinDivert::~BaseWinDivert()
{
	WinDivertClose(handle);
	CloseHandle(handle);
	system("sc stop windivert");
	delete filter;
}