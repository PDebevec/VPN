#pragma once

#include <Windows.h>
#include <iostream>

class BasePipe
{
public:
	BasePipe();

	bool readPipe(char* buffer, DWORD bufferSize, LPDWORD bytesRead, LPOVERLAPPED lpOverlapped);
	bool writePipe(char* buffer, DWORD bufferSize, LPDWORD byetesWriten, LPOVERLAPPED lpOverlapped);

	~BasePipe();

private:
	int ConnectPipe();

private:
	HANDLE pipe;
};

BasePipe::BasePipe()
{
	pipe = NULL;
}

inline int BasePipe::ConnectPipe()
{
	while (true)
	{
		pipe = CreateFile(
			L"\\\\.\\pipe\\VPN",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		if (pipe != INVALID_HANDLE_VALUE)
			return -1;

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			std::cerr << "Could not open pipe!" << std::endl;
			return -1;
		}

		if (!WaitNamedPipe(L"\\\\.\\pipe\\VPN", 10000))
		{
			std::cerr << "Timedout trying to connect!" << std::endl;
			return -1;
		}
	}

	return TRUE;
}

inline bool BasePipe::readPipe(char* buffer, DWORD bufferSize, LPDWORD bytesRead, LPOVERLAPPED lpOverlapped)
{
	if (!ReadFile(pipe, buffer, bufferSize, bytesRead, lpOverlapped))
	{
		std::cerr << "Error reading from named pipe! Pipe error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

inline bool BasePipe::writePipe(char* buffer, DWORD bufferSize, LPDWORD byetesWriten, LPOVERLAPPED lpOverlapped)
{
	if (WriteFile(pipe, buffer, bufferSize, byetesWriten, lpOverlapped))
	{
		std::cerr << "Error writeing to named pipe! Pipe error code: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

BasePipe::~BasePipe()
{
}
