#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <process.h>
#include <time.h>
#include <string>

#include "discord_rpc.h"
#include "query.h"
#include "client.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "discord-rpc.lib")

void UpdateDiscord(void*) {
    SAMP::ServerData data;
    char clientId[64], largeKey[64], largeText[128], smallKey[64], smallText[128], discordUrl1[256], btnLabel1[64], discordUrl2[256], btnLabel2[64], filterName[64];
    const char* iniPath = ".\\discord_rpc_settings.ini";

    GetPrivateProfileStringA("Settings", "ClientID", "1400667687020920914", clientId, sizeof(clientId), iniPath);
    GetPrivateProfileStringA("Settings", "LargeImageKey", "aksara_logo", largeKey, sizeof(largeKey), iniPath);
    GetPrivateProfileStringA("Settings", "LargeImageText", "Aksara Roleplay", largeText, sizeof(largeText), iniPath);
    GetPrivateProfileStringA("Settings", "SmallImageKey", "samp", smallKey, sizeof(smallKey), iniPath);
    GetPrivateProfileStringA("Settings", "SmallImageText", "Samp Icon", smallText, sizeof(smallText), iniPath);
    GetPrivateProfileStringA("Settings", "DiscordURL1", "", discordUrl1, sizeof(discordUrl1), iniPath);
    GetPrivateProfileStringA("Settings", "ButtonLabel1", "", btnLabel1, sizeof(btnLabel1), iniPath);
    GetPrivateProfileStringA("Settings", "DiscordURL2", "", discordUrl2, sizeof(discordUrl2), iniPath);
    GetPrivateProfileStringA("Settings", "ButtonLabel2", "", btnLabel2, sizeof(btnLabel2), iniPath);

    GetPrivateProfileStringA("Settings", "FilterName", "aksara", filterName, sizeof(filterName), iniPath);
    int enableFilter = GetPrivateProfileIntA("Settings", "EnableFilter", 1, iniPath);

    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(clientId, &handlers, 1, NULL);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    if (SAMP::readServerData(GetCommandLineA(), data)) {
        auto startTime = time(0);
        SAMP::Query query(data.address, (unsigned short)std::stoi(data.port));

        while (true) {
            SAMP::Query::Information info;
            if (query.info(info)) {
                std::string lowerHostname = info.hostname;
                for (auto& c : lowerHostname) c = tolower(c);

                std::string filterTarget = filterName;
                for (auto& c : filterTarget) c = tolower(c);

                if (enableFilter == 0 || lowerHostname.find(filterTarget) != std::string::npos) {
                    DiscordRichPresence rpc;
                    memset(&rpc, 0, sizeof(rpc));

                    std::string playerStatus = std::to_string(info.basic.players) + "/" + std::to_string(info.basic.maxPlayers) + " Players Online";
                    rpc.details = playerStatus.c_str();
                    rpc.state = info.hostname.c_str();
                    rpc.startTimestamp = startTime;

                    rpc.largeImageKey = largeKey;
                    rpc.largeImageText = largeText;

                    rpc.smallImageKey = smallKey;
                    rpc.smallImageText = smallText;

                    if (strlen(btnLabel1) > 0 && strlen(discordUrl1) > 0) {
                        rpc.button1_label = btnLabel1;
                        rpc.button1_url = discordUrl1;
                    }

                    if (strlen(btnLabel2) > 0 && strlen(discordUrl2) > 0) {
                        rpc.button2_label = btnLabel2;
                        rpc.button2_url = discordUrl2;
                    }

                    Discord_UpdatePresence(&rpc);
                }
                else {
                    Discord_ClearPresence();
                }
            }
            Discord_RunCallbacks();
            Sleep(10000);
        }
    }
    Discord_Shutdown();
    WSACleanup();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        _beginthread(&UpdateDiscord, 0, NULL);
    }
    return TRUE;
}