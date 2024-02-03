#pragma once

#include <iostream>
#include <Windows.h>

#define pause system("pause")

class BaseCommunication
{
public:
	BaseCommunication();

	bool recvMessage(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
	bool sendMessage(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);

	~BaseCommunication();

protected:
	bool openPipe();
	bool closePipe();

private:
	HANDLE pipe;
};

BaseCommunication::BaseCommunication()
{
	pipe = NULL;
}

inline bool BaseCommunication::openPipe()
{
	pipe = CreateFile(
		L"\\\\.\\pipe\\VPN",
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Error connecting to named pipe. Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseCommunication::recvMessage(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead = NULL, LPOVERLAPPED lpOverlapped = nullptr)
{
	if (ReadFile(pipe, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped) == 0)
	{
		std::cerr << "Error reading from named pipe. Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseCommunication::sendMessage(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten = NULL, LPOVERLAPPED lpOverlapped = nullptr)
{
	if (WriteFile(pipe, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped) == 0)
	{
		std::cerr << "Error sending threw named pipe. Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BaseCommunication::closePipe()
{
	if (!CloseHandle(pipe)) {
		std::cerr << "Error closing named pipe handle. Error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

BaseCommunication::~BaseCommunication()
{
	closePipe();
}