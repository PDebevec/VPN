#pragma once

#include "baseWinDivert.h"
#include <iomanip>
#include <cstdint>

namespace PM {
	bool isLocalPacket(unsigned char* pPacket) {
        switch (pPacket[16]) {
        case 10:
            return true;
        case 172:
            return (pPacket[17] >= 16 && pPacket[17] <= 31);
        case 192:
            return (pPacket[17] == 168);
        default:
            return false;
        }
	}

    void displayIPv4HeaderInfo(const unsigned char* packetBuffer, UINT bufferSize) {
        // Extracting fields from the IPv4 header
        uint8_t version = (packetBuffer[0] >> 4) & 0x0F;
        uint8_t headerLength = (packetBuffer[0] & 0x0F) * 4; // in bytes
        uint8_t diffserv = packetBuffer[1];
        uint16_t totalLength = (packetBuffer[2] << 8) | packetBuffer[3];
        uint16_t identification = (packetBuffer[4] << 8) | packetBuffer[5];
        uint16_t flags = (packetBuffer[6] >> 5) & 0x07;
        uint16_t fragmentOffset = ((packetBuffer[6] & 0x1F) << 8) | packetBuffer[7];
        uint8_t timeToLive = packetBuffer[8];
        uint8_t protocol = packetBuffer[9];
        uint16_t headerChecksum = (packetBuffer[10] << 8) | packetBuffer[11];
        uint32_t sourceIPAddress = (packetBuffer[12] << 24) | (packetBuffer[13] << 16) | (packetBuffer[14] << 8) | packetBuffer[15];
        uint32_t destinationIPAddress = (packetBuffer[16] << 24) | (packetBuffer[17] << 16) | (packetBuffer[18] << 8) | packetBuffer[19];

        // Displaying IPv4 header information
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Version: " << unsigned(version) << std::endl;
        std::cout << "Header Length: " << unsigned(headerLength) << " bytes" << std::endl;
        std::cout << "Differentiated Services (DSCP): " << unsigned(diffserv) << std::endl;
        std::cout << "Total Length: " << totalLength << " bytes" << std::endl;
        std::cout << "Identification: " << identification << std::endl;
        std::cout << "Flags: " << flags << std::endl;
        std::cout << "Fragment Offset: " << fragmentOffset << std::endl;
        std::cout << "Time to Live (TTL): " << unsigned(timeToLive) << std::endl;
        std::cout << "Protocol: " << unsigned(protocol) << std::endl;
        std::cout << "Header Checksum: " << headerChecksum << std::endl;
        std::cout << "Source IP Address: " << ((sourceIPAddress >> 24) & 0xFF) << "." << ((sourceIPAddress >> 16) & 0xFF) << "." << ((sourceIPAddress >> 8) & 0xFF) << "." << (sourceIPAddress & 0xFF) << std::endl;
        std::cout << "Destination IP Address: " << ((destinationIPAddress >> 24) & 0xFF) << "." << ((destinationIPAddress >> 16) & 0xFF) << "." << ((destinationIPAddress >> 8) & 0xFF) << "." << (destinationIPAddress & 0xFF) << std::endl;
        
        for (size_t i = 0; i < bufferSize; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packetBuffer[i]) << " ";
            if ((i + 1) % 16 == 0) std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}