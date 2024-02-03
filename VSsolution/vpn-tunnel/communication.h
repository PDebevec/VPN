#pragma once

#include <Windows.h>
#include <iostream>

class Communication
{
public:
	Communication();

	void waitForConnection();
	BOOL isConnected();

	bool establishPipe();
	BOOL disconnectPipe();

	const char* receiveMessage();

	BOOL sendMessage(const char* buffer);

	~Communication();

private:
	HANDLE pipe;
	char buffer[128];
	DWORD bytesRead;
	DWORD readBytes;
	BOOL returnVal;
};

Communication::Communication()
	:buffer{}
{
	readBytes = sizeof(buffer);
	pipe = NULL;
	bytesRead = NULL;
	returnVal = NULL;
}

inline bool Communication::establishPipe()
{
	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;

	pipe = CreateNamedPipe(
		L"\\\\.\\pipe\\VPN",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, 0, 0, 0, &sa
	);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else return true;
}

inline BOOL Communication::isConnected()
{
	return GetNamedPipeHandleState(pipe, NULL, NULL, NULL, NULL, NULL, 0);
}

inline void Communication::waitForConnection()
{
	while (!ConnectNamedPipe(pipe, nullptr))
	{}
}

inline const char* Communication::receiveMessage()
{
	if (ReadFile(pipe, (LPVOID)buffer, readBytes, &bytesRead, NULL))
	{
		return buffer;
	}
	else {
		return nullptr;
	}
}

inline BOOL Communication::sendMessage(const char* buffer)
{
	return WriteFile(pipe, (LPCVOID)buffer, strlen(buffer) + 1, NULL, NULL);
}

inline BOOL Communication::disconnectPipe()
{
	return DisconnectNamedPipe(pipe);
}

Communication::~Communication()
{
	DisconnectNamedPipe(pipe);
	CloseHandle(pipe);
}