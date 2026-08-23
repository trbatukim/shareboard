#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>

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
    serverAddr.sin_port = htons(54000);

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

    printf("Server is listening on port 54000...\n");

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

    printf("Client connected successfully.\n");

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}