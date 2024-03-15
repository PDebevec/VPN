#pragma once

#include <iostream>

class StandardIO
{
public:
	StandardIO();

    void changeWaitTime(int);

    void waitToRecv(char*, size_t);
	bool recvIO(char*, size_t);
	void sendIO(const char*);

	~StandardIO();

private:
    int timeToWait;
    HANDLE standardInput;
};

StandardIO::StandardIO()
{
    standardInput = GetStdHandle(STD_INPUT_HANDLE);
    timeToWait = 10000;
}

void StandardIO::waitToRecv(char* buffer, size_t bufferSize)
{
    std::cin.getline(buffer, bufferSize);
}

bool StandardIO::recvIO(char* buffer, size_t bufferSize)
{
    if (WaitForSingleObject(standardInput, timeToWait) == WAIT_OBJECT_0) {
        std::cin.getline(buffer, bufferSize);
        return true;
    }
    else {
        return false;
    }
}

inline void StandardIO::sendIO(const char* buffer)
{
    std::cout << buffer << std::endl;
}

inline void StandardIO::changeWaitTime(int seconds)
{
    timeToWait = seconds * 1000;
}

StandardIO::~StandardIO()
{
}