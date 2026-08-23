#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sys/types.h>

#pragma comment(lib, "ws2_32.lib")

static const unsigned short kPort = 54000;
static const uint32_t kMaxMessage = 256;

static bool SendAll(SOCKET s, const char* data, int len)
{
    int total = 0;

    while (total < len)
    {
        const int n = send(s, data + total, len - total, 0);

        if (n == SOCKET_ERROR)
        {
            fprintf(stderr, "send failed: %d\n", WSAGetLastError());
            return false;
        }

        total += n;
    }

    return true;
}

// Returns 1 once len bytes are in, 0 if the peer closed cleanly, -1 on error.
// Those three cases need to stay distinct: a clean close is normal shutdown,
// an error is not.
static int RecvExact(SOCKET s, char* out, int len)
{
    int total = 0;

    while (total < len)
    {
        const int n = recv(s, out + total, len - total, 0);

        if (n == 0) return 0;
        if (n == SOCKET_ERROR) return -1;

        total += n;
    }

    return 1;
}

static bool SendFramed(SOCKET s, const char* text)
{
    const uint32_t len = static_cast<uint32_t>(strlen(text));
    const uint32_t netLen = htonl(len);

    if (!SendAll(s, reinterpret_cast<const char*>(&netLen), sizeof(netLen)))
    {
        return false;
    }

    return SendAll(s, text, static_cast<int>(len));
}

static void EnableTCPNoDelay(SOCKET sockfd) {
    int flag = 1;

    int result = setsockopt(sockfd, 
                            IPPROTO_TCP, 
                            TCP_NODELAY, 
                            reinterpret_cast<const char*>(&flag), 
                            sizeof(flag));

    if (result == SOCKET_ERROR) 
    {
        fprintf(stderr, "Failed to set TCP_NODELAY. Error code: %d\n", WSAGetLastError());
    } 
    else 
    {
        printf("Success: TCP_NODELAY enabled.\n");
    }
}

int RunServer()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != 0)
    {
        fprintf(stderr, "WSA startup failed %d\n", iResult);
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(kPort);

    iResult = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (iResult == SOCKET_ERROR) 
    {
        fprintf(stderr, "Bind failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    iResult = listen(listenSocket, SOMAXCONN);

    if (iResult == SOCKET_ERROR) 
    {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %u...\n", kPort);

    sockaddr_in clientAddr;
    int clientSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientSize);

    if (clientSocket == INVALID_SOCKET) 
    {
        fprintf(stderr, "Accept failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    EnableTCPNoDelay(clientSocket);
    printf("Client connected successfully.\n");

    for (;;)
    {
        uint32_t netLen = 0;
        const int header = RecvExact(clientSocket,
                                     reinterpret_cast<char*>(&netLen),
                                     sizeof(netLen));

        if (header == 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        if (header < 0)
        {
            fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
            break;
        }

        const uint32_t len = ntohl(netLen);

        if (len > kMaxMessage)
        {
            fprintf(stderr, "Message of %u bytes exceeds the %u byte cap.\n",
                    len, kMaxMessage);
            break;
        }

        char buffer[kMaxMessage + 1];

        if (RecvExact(clientSocket, buffer, static_cast<int>(len)) != 1)
        {
            fprintf(stderr, "Connection dropped mid-message.\n");
            break;
        }

        buffer[len] = '\0';
        printf("recv (%u bytes): %s\n", len, buffer);
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}

int RunClient() 
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != 0)
    {
        fprintf(stderr, "WSA startup failed %d\n", iResult);
        return 1;
    }

    SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(kPort);

    iResult = inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (iResult <= 0) 
    {
        fprintf(stderr, "Inavlid address / Address not supported\n");
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Connecting to server...\n");
    iResult = connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (iResult == SOCKET_ERROR) 
    {
        fprintf(stderr, "Connection failed: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    EnableTCPNoDelay(connectSocket);
    printf("Connected to server successfully.\n");

    const char* messages[] = {
        "hello from client",
        "second message",
        "third and last",
    };

    for (const char* text : messages)
    {
        if (!SendFramed(connectSocket, text))
        {
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("sent: %s\n", text);
    }

    closesocket(connectSocket);
    WSACleanup();
    return 0;
}