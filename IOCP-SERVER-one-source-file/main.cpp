#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

static bool InitializeWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "1. Failed to initialize Winsock." << std::endl;
        return false;
    }
    else
    {
        std::cout << "1. Winsock initialized." << std::endl;
        return true;
    }
}

static SOCKET CreateServerSocket(int port)
{
    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "2. Failed to create server socket." << std::endl;
        return INVALID_SOCKET;
    }
    else
    {
        std::cout << "2. Server socket created." << std::endl;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "3. Bind failed." << std::endl;
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    else
    {
        std::cout << "3. Bind done." << std::endl;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "4. Listen failed." << std::endl;
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    else
    {
        std::cout << "4. Server listening on port " << port << std::endl;
    }

    return listenSocket;
}

int main() 
{
    if (!InitializeWinsock()) 
    {
        return 1;
    }

    SOCKET serverSocket = CreateServerSocket(8080);
    if (serverSocket == INVALID_SOCKET) 
    {
        WSACleanup();
        return 1;
    }

    return 0;
}
