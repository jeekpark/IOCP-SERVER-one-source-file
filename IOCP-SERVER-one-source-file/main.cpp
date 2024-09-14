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

static HANDLE CreateIOCP()
{
    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (iocp == NULL)
    {
        std::cerr << "5. Failed to create IOCP." << std::endl;
    }
    else
    {
        std::cout << "5. IOCP created." << std::endl;
    }
    return iocp;
}

static bool AssociateSocketWithIOCP(HANDLE iocp, SOCKET socket, ULONG_PTR completionKey)
{
    HANDLE result = CreateIoCompletionPort((HANDLE)socket, iocp, completionKey, 0);
    if (result == NULL)
    {
        std::cerr << "Failed to associate socket with IOCP." << std::endl;
        return false;
    }
    return true;
}

struct ClientContext
{
    SOCKET socket;
    WSAOVERLAPPED overlapped;
    WSABUF buffer;
    char dataBuffer[1024];
    DWORD bytesReceived;
};

static bool ReceiveData(ClientContext* clientContext)
{
    ZeroMemory(&clientContext->overlapped, sizeof(WSAOVERLAPPED));
    clientContext->buffer.buf = clientContext->dataBuffer;
    clientContext->buffer.len = sizeof(clientContext->dataBuffer);

    DWORD flags = 0;
    int result = WSARecv(clientContext->socket, &clientContext->buffer, 1, NULL, &flags, &clientContext->overlapped, NULL);

    if (result == SOCKET_ERROR) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "WSARecv failed." << std::endl;
            return false;
        }
    }
    return true;
}

static void WorkerThread(HANDLE iocp)
{
    DWORD bytesTransferred;
    ClientContext* clientContext{};
    LPOVERLAPPED overlapped;

    while (true)
    {
        BOOL result = GetQueuedCompletionStatus(iocp, &bytesTransferred, (PULONG_PTR)&clientContext, &overlapped, INFINITE);
        if (result == FALSE || bytesTransferred == 0)
        {
            std::cerr << "Client disconnected." << std::endl;
            closesocket(clientContext->socket);
            delete clientContext;
            continue;
        }
        std::cout << "Received data: " << clientContext->dataBuffer << std::endl;
        ReceiveData(clientContext);
    }
}



int main() 
{
    if (InitializeWinsock() == false)
    {
        return 1;
    }

    SOCKET serverSocket = CreateServerSocket(8080);
    if (serverSocket == INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }

    HANDLE iocp = CreateIOCP();
    if (iocp == NULL)
    {
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i)
    { 
        workers.push_back(std::thread(WorkerThread, iocp));
    }

    while (true)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Accept failed." << std::endl;
            break;
        }
        else
        {
            std::cout << "Client connected." << std::endl;
        }   
        ClientContext* clientContext = new ClientContext;
        clientContext->socket = clientSocket;

        if (!AssociateSocketWithIOCP(iocp, clientSocket, (ULONG_PTR)clientContext))
        {
            closesocket(clientSocket);
            delete clientContext;
            continue;
        }
        ReceiveData(clientContext);
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
