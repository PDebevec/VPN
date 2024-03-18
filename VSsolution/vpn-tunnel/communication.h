#pragma once

#include <iostream>
#include <windows.h>

class IPCPiep
{
public:
	IPCPiep();

    bool pipeRead(LPVOID buffer, DWORD bufferSize, LPDWORD readLen);
    bool pipeWrite(LPCVOID buffer, DWORD bufferLen, LPDWORD writeLen);

	~IPCPiep();

private:
    HANDLE pipe;
};

IPCPiep::IPCPiep()
{
    printf("pipe init\n");
    pipe = CreateFile(
        L"\\\\.\\pipe\\VPNpipe",
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        0, NULL
    );

    if (pipe == INVALID_HANDLE_VALUE)
    {
        throw "Error connecting to pipe!";
    }
}

bool IPCPiep::pipeRead(LPVOID buffer, DWORD bufferSize, LPDWORD readLen)
{
    if (ReadFile(pipe, buffer, bufferSize, readLen, NULL) == FALSE)
    {
        std::cout << "Error reading from pipe!" << std::endl;
        return false;
    }
    return true;
}

bool IPCPiep::pipeWrite(LPCVOID buffer, DWORD bufferLen, LPDWORD writeLen)
{
    if (WriteFile(pipe, buffer, bufferLen, writeLen, NULL) == FALSE)
    {
        std::cout << "error writing to pipe!" << std::endl;
        return false;
    }
    return true;
}

IPCPiep::~IPCPiep()
{
    CloseHandle(pipe);
}