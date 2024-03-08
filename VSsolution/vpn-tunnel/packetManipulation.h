#pragma once

#include "baseWinDivert.h"
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <chrono>

namespace PM {
    namespace DI {
        inline void version(UINT8* packetBuffer) {
            std::cout << "Version: " << unsigned((packetBuffer[0] >> 4) & 0x0F) << std::endl;
        }
        inline void headerLen(UINT8* packetBuffer) {
            std::cout << "Header Length: " << unsigned((packetBuffer[0] & 0x0F) * 4) << " bytes" << std::endl;
        }
        inline void DSCP(UINT8* packetBuffer) {
            std::cout << "Differentiated Services (DSCP): " << unsigned(packetBuffer[1]) << std::endl;
        }
        inline void totalLen(UINT8* packetBuffer) {
            std::cout << "Total Length: " << ((packetBuffer[2] << 8) | packetBuffer[3]) << " bytes" << std::endl;
        }
        inline void identification(UINT8* packetBuffer) {
            std::cout << "Identification: " << ((packetBuffer[4] << 8) | packetBuffer[5]) << std::endl;
        }
        inline void falgs(UINT8* packetBuffer) {
            std::cout << "Flags: " << ((packetBuffer[6] >> 5) & 0x07) << std::endl;
        }
        inline void fragmentOffset(UINT8* packetBuffer) {
            std::cout << "Fragment Offset: " << (((packetBuffer[6] & 0x1F) << 8) | packetBuffer[7]) << std::endl;
        }
        inline void TTL(UINT8* packetBuffer) {
            std::cout << "Time to Live (TTL): " << unsigned(packetBuffer[8]) << std::endl;
        }
        inline void protocol(UINT8* packetBuffer) {
            std::cout << "Protocol: " << unsigned(packetBuffer[9]) << std::endl;
        }
        inline void headerChecksum(UINT8* packetBuffer) {
            std::cout << "Header Checksum: " << ((packetBuffer[10] << 8) | packetBuffer[11]) << std::endl;
        }
        inline void sourceIPAddress(UINT8* packetBuffer) {
            std::cout << "Source IP Address: " << unsigned(packetBuffer[12]) << "." << unsigned(packetBuffer[13]) << "." << unsigned(packetBuffer[14]) << "." << unsigned(packetBuffer[15]) << std::endl;
        }
        inline void destinationIPAddress(UINT8* packetBuffer) {
            std::cout << "Destination IP Address: " << unsigned(packetBuffer[16]) << "." << unsigned(packetBuffer[17]) << "." << unsigned(packetBuffer[18]) << "." << unsigned(packetBuffer[19]) << std::endl;
        }
    }

	bool isLocalPacket(UINT8* pPacket) {
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

    byte* ipStringToArray(char* ipString) {
        byte* byteArray = new byte[4];

        char* nextToken = nullptr;
        char* token = strtok_s(ipString, ".", &nextToken);
        int i = 0;
        while (token != nullptr && i < 4) {
            byteArray[i++] = atoi(token);
            token = strtok_s(nullptr, ".", &nextToken);
        }

        delete token;
        return byteArray;
    }

    void changePacketDstIP(UINT8* packet, byte* ip) {
        packet[16] = ip[0];
        packet[17] = ip[1];
        packet[18] = ip[2];
        packet[19] = ip[3];
    }

    void changePacketSrcIP(UINT8* packet, byte* ip) {
        packet[12] = ip[0];
        packet[13] = ip[1];
        packet[14] = ip[2];
        packet[15] = ip[3];
    }

    void increaseTTL(unsigned char* packet) {
        packet[8] += 5;
    }

    inline bool isDstIP(unsigned char* packet, byte* ip) {
        return (
            static_cast<byte>(packet[16]) == ip[0] &&
            static_cast<byte>(packet[17]) == ip[1] &&
            static_cast<byte>(packet[18]) == ip[2] &&
            static_cast<byte>(packet[19]) == ip[3]);
    }
    
    inline bool isSrcIP(unsigned char* packet, byte* ip) {
        return (
            static_cast<byte>(packet[12]) == ip[0] &&
            static_cast<byte>(packet[13]) == ip[1] &&
            static_cast<byte>(packet[14]) == ip[2] &&
            static_cast<byte>(packet[15]) == ip[3]);
    }

    UINT16 calculateChecksum(const UINT8* packet, size_t packetSize) {
        UINT32 sum = 0;

        for (size_t i = 0; i < packetSize; i += 2) {
            if (i != 10) {
                sum += (packet[i] << 8) + packet[i + 1];
            }
        }

        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        return static_cast<UINT16>(~sum);
    }

    template <typename T>
    void displayBits(const char* string, const T& value) {
        std::bitset<sizeof(T)*8> bits(value);
        std::cout << string << bits << std::endl;
    }

    void WDaddressBinInfo(WINDIVERT_ADDRESS* addr) {
        std::cout << "----------------------------------------" << std::endl;
        displayBits("Event ", addr->Event);
        displayBits("F EndpointId ", addr->Flow.EndpointId);
        displayBits("F LocalAddr ", addr->Flow.LocalAddr);
        displayBits("F LocalPort ", addr->Flow.LocalPort);
        displayBits("F ParentEndpointId ", addr->Flow.ParentEndpointId);
        displayBits("F ProcessId ", addr->Flow.ProcessId);
        displayBits("F Protocol ", addr->Flow.Protocol);
        displayBits("F RemoteAddr ", addr->Flow.RemoteAddr);
        displayBits("F RemotePort ", addr->Flow.RemotePort);
        displayBits("Impostor ", addr->Impostor);
        displayBits("IPChecksum ", addr->IPChecksum);
        displayBits("IPv6 ", addr->IPv6);
        displayBits("Layer ", addr->Layer);
        displayBits("Loopback ", addr->Loopback);
        displayBits("N IfIdx ", addr->Network.IfIdx);
        displayBits("N SubIfIdx ", addr->Network.SubIfIdx);
        displayBits("Outbound ", addr->Outbound);
        displayBits("R falgs ", addr->Reflect.Flags);
        displayBits("R layer ", addr->Reflect.Layer);
        displayBits("R priority ", addr->Reflect.Priority);
        displayBits("R processid ", addr->Reflect.ProcessId);
        displayBits("R timestamp ", addr->Reflect.Timestamp);
        displayBits("Reserved1 ", addr->Reserved1);
        displayBits("Reserved2 ", addr->Reserved2);
        //displayBits("Reserved3 ", addr->Reserved3);
        displayBits("Sniffed ", addr->Sniffed);
        displayBits("S endpoint ", addr->Socket.EndpointId);
        displayBits("S address ", addr->Socket.LocalAddr);
        displayBits("S localport ", addr->Socket.LocalPort);
        displayBits("S parentendpointid ", addr->Socket.ParentEndpointId);
        displayBits("S processid ", addr->Socket.ProcessId);
        displayBits("S protocol ", addr->Socket.Protocol);
        displayBits("S remoteaddr ", addr->Socket.RemoteAddr);
        displayBits("S remoteport ", addr->Socket.RemotePort);
        displayBits("TCPChecksum ", addr->TCPChecksum);
        displayBits("Timestamp ", addr->Timestamp);
        displayBits("UDPChecksum ", addr->UDPChecksum);
    }

    void WDaddressInfo(WINDIVERT_ADDRESS* addr) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Event: " << addr->Event << std::endl;
        std::cout << "F endpointID: " << addr->Flow.EndpointId << std::endl;///
        std::cout << "F localaddr: " << addr->Flow.LocalAddr[0] << addr->Flow.LocalAddr[1] << addr->Flow.LocalAddr[2] << addr->Flow.LocalAddr[3] << std::endl;
        std::cout << "F localport: " << addr->Flow.LocalPort << std::endl;
        std::cout << "F parentendpointid: " << addr->Flow.ParentEndpointId << std::endl;
        std::cout << "F processid: " << addr->Flow.ProcessId << std::endl;
        std::cout << "F protocol: " << addr->Flow.Protocol << std::endl;
        std::cout << "F remoteaddr: " << addr->Flow.RemoteAddr[0] << addr->Flow.RemoteAddr[1] << addr->Flow.RemoteAddr[2] << addr->Flow.RemoteAddr[3] << std::endl;
        std::cout << "F remoteport: " << addr->Flow.RemotePort << std::endl;
        std::cout << "Imposter: " << addr->Impostor << std::endl;
        std::cout << "IPchecksum: " << addr->IPChecksum << std::endl;
        std::cout << "IPv6: " << addr->IPv6 << std::endl;
        std::cout << "Layer: " << addr->Layer << std::endl;
        std::cout << "Loopback: " << addr->Loopback << std::endl;
        std::cout << "N ifidx: " << addr->Network.IfIdx << std::endl;///
        std::cout << "N subifidx: " << addr->Network.SubIfIdx<< std::endl;
        std::cout << "Outbound: " << addr->Outbound << std::endl;
        std::cout << "R flags: " << addr->Reflect.Flags << std::endl;
        std::cout << "R layer: " << addr->Reflect.Layer << std::endl;
        std::cout << "R priority: " << addr->Reflect.Priority << std::endl;
        std::cout << "R process: " << addr->Reflect.ProcessId << std::endl;
        std::cout << "R timestamp: " << addr->Reflect.Timestamp << std::endl;///
        std::cout << "Reserved1: " << addr->Reserved1 << std::endl;
        std::cout << "Reserved2: " << addr->Reserved2 << std::endl;
        std::cout << "Reserved3: ";
        for (size_t i = 0; i < 64; i++)
        {
            std::cout << static_cast<unsigned int>(addr->Reserved3[i]);
        }
        std::cout << std::endl;///
        std::cout << "Snniffed: " << addr->Sniffed << std::endl;
        std::cout << "S endpointID: " << addr->Socket.EndpointId << std::endl;///
        std::cout << "S localaddr: " << addr->Socket.LocalAddr[0] << addr->Socket.LocalAddr[1] << addr->Socket.LocalAddr[2] << addr->Socket.LocalAddr[3] << std::endl;
        std::cout << "S localport: " << addr->Socket.LocalPort << std::endl;
        std::cout << "S parentendpointid: " << addr->Socket.ParentEndpointId << std::endl;
        std::cout << "S processid: " << addr->Socket.ProcessId << std::endl;
        std::cout << "S protocol: " << addr->Socket.Protocol << std::endl;
        std::cout << "S remoteaddr: " << addr->Socket.RemoteAddr[0] << addr->Socket.RemoteAddr[1] << addr->Socket.RemoteAddr[2] << addr->Socket.RemoteAddr[3] << std::endl;
        std::cout << "S remoteport: " << addr->Socket.RemotePort << std::endl;
        std::cout << "TCPchecksum: " << addr->TCPChecksum << std::endl;
        std::cout << "Timestamp: " << addr->Timestamp << std::endl;////
        std::cout << "UDPchecksum: " << addr->UDPChecksum<< std::endl;
    }

    void displayIPv4HeaderInfo(UINT8* packetBuffer) {
        std::cout << "----------------------------------------" << std::endl;
        DI::version(packetBuffer);
        DI::headerLen(packetBuffer);
        DI::DSCP(packetBuffer);
        DI::totalLen(packetBuffer);
        DI::identification(packetBuffer);
        DI::falgs(packetBuffer);
        DI::fragmentOffset(packetBuffer);
        DI::TTL(packetBuffer);
        DI::protocol(packetBuffer);
        DI::headerChecksum(packetBuffer);
        DI::sourceIPAddress(packetBuffer);
        DI::destinationIPAddress(packetBuffer);
    }

    void displayPacketData(const UINT8* packetBuffer, UINT bufferSize) {
        for (size_t i = 0; i < bufferSize; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packetBuffer[i]) << " ";
            if ((i + 1) % 16 == 0) std::cout << std::endl;
        }
        std::cout << std::dec << std::setw(0) << std::setfill(' ') << std::endl;
    }
}