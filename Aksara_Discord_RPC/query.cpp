#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include "query.h"

namespace SAMP
{
    typedef int socklen_t;

    bool Query::info(Information& information)
    {
        char packet[QUERY_BASIC_PACKET_LENGTH];
        if (send('i', packet) < 0) return false;
        return recvInfo(information, packet);
    }

    int Query::send(const char opcode, char out[QUERY_BASIC_PACKET_LENGTH])
    {
        if (sock <= 0) return -1;
        if (!assemble(opcode, out)) return -1;

        return sendto(sock, out, QUERY_BASIC_PACKET_LENGTH, 0, reinterpret_cast<sockaddr*>(&server), sizeof(sockaddr_in));
    }

    bool Query::recvInfo(Information& info, char in[QUERY_BASIC_PACKET_LENGTH])
    {
        if (sock <= 0) return false;

        char cbuffer[QUERY_INCOMING_BUFFER_SIZE] = { 0 };
        sockaddr_in from;
        int fromlen = sizeof(from);
        int recvbytes = recvfrom(sock, cbuffer, QUERY_INCOMING_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&from), &fromlen);

        if (recvbytes > QUERY_BASIC_PACKET_LENGTH) {
            for (size_t i = 0; i < QUERY_BASIC_PACKET_LENGTH; ++i) {
                if (cbuffer[i] != in[i]) return false;
            }

            char* current = cbuffer + QUERY_BASIC_PACKET_LENGTH;
            char* end = cbuffer + recvbytes;

            if (current + sizeof(BasicInformation) > end) return false;
            info.basic = *reinterpret_cast<BasicInformation*>(current);
            current += sizeof(BasicInformation);

            auto readString = [&](std::string& outStr) {
                if (current + 4 > end) return false;
                uint32_t len = *reinterpret_cast<uint32_t*>(current);
                current += 4;

                if (current + len > end) len = (uint32_t)(end - current);
                if (len > 0) outStr.assign(current, len);
                current += len;
                return true;
                };

            readString(info.hostname);
            readString(info.gamemode);
            readString(info.language);

            return true;
        }
        return false;
    }

    bool Query::assemble(const char opcode, char out[QUERY_BASIC_PACKET_LENGTH])
    {
        out[0] = 'S'; out[1] = 'A'; out[2] = 'M'; out[3] = 'P';

        uint32_t addr = server.sin_addr.s_addr;
        out[4] = (addr & 0xFF);
        out[5] = ((addr >> 8) & 0xFF);
        out[6] = ((addr >> 16) & 0xFF);
        out[7] = ((addr >> 24) & 0xFF);

        out[8] = LOBYTE(server.sin_port);
        out[9] = HIBYTE(server.sin_port);
        out[10] = opcode;
        return true;
    }

    Query::Query(std::string ip, const unsigned short port, long timeout)
    {
        memset(&server, 0, sizeof(server));
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            sock = 0;
            return;
        }

        DWORD timeoutMs = (DWORD)timeout;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(ip.c_str());
    }

    Query::~Query() {
        if (sock) closesocket(sock);
    }
}