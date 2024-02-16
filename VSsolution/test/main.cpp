#include <windows.h>
#include <iostream>
#include <chrono>
#include <ctime>

int main()
{
    long long time_since_epoch = 376859988327;

    // Convert the signed long long to std::time_t
    std::time_t time_t_value = static_cast<std::time_t>(time_since_epoch);

    // Use ctime_s for safer string manipulation
    char buffer[26];
    if (ctime_s(buffer, sizeof(buffer), &time_t_value) == 0) {
        // Print the converted time
        std::cout << "Converted time: " << buffer;
    }
    else {
        std::cerr << "Error converting time. " << GetLastError() << std::endl;
    }

    return 0;

    //HANDLE pipe = CreateFile(
    //    L"\\\\.\\pipe\\VPN",  // Pipe name
    //    GENERIC_READ | GENERIC_WRITE,  // Access mode (read/write)
    //    0,                              // No sharing
    //    nullptr,                        // Default security attributes
    //    OPEN_EXISTING,                  // Opens existing pipe
    //    0,                              // No attributes
    //    nullptr);                       // No template file

    //if (pipe == INVALID_HANDLE_VALUE) {
    //    std::cerr << "Failed to connect to the pipe. " << GetLastError() << std::endl;
    //    return 1;
    //}

    //char* message = new char[128];
    //DWORD bytesWritten;
    //while (true)
    //{
    //    delete message;
    //    message = new char[128];
    //    std::cout << "msg> ";
    //    std::cin.getline(message, sizeof(message));

    //    message += '\0';

    //    WriteFile(pipe, message, strlen(message), &bytesWritten, nullptr);
    //}

    //// Close the pipe handle when done.
    //CloseHandle(pipe);

    //return 0;
}