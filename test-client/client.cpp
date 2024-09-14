#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

bool InitializeWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return false;
    }
    return true;
}

SOCKET CreateClientSocket(const char* serverAddress, int port)
{
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create client socket." << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverAddress);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to server." << std::endl;
        closesocket(clientSocket);
        return INVALID_SOCKET;
    }

    std::cout << "Connected to server." << std::endl;
    return clientSocket;
}

bool SendMessageToServer(SOCKET clientSocket, const std::string& message)
{
    int result = send(clientSocket, message.c_str(), message.length(), 0);
    if (result == SOCKET_ERROR)
    {
        std::cerr << "Failed to send message." << std::endl;
        return false;
    }
    std::cout << "Message sent: " << message << std::endl;
    return true;
}

bool ReceiveMessageFromServer(SOCKET clientSocket)
{
    char buffer[1024] = { 0 };
    int result = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (result > 0)
    {
        std::cout << "Received from server: " << buffer << std::endl;
        return true;
    }
    else if (result == 0)
    {
        std::cout << "Connection closed by server." << std::endl;
    }
    else
    {
        std::cerr << "Failed to receive message from server." << std::endl;
    }
    return false;
}

int main()
{
    if (!InitializeWinsock())
    {
        return 1;
    }

    SOCKET clientSocket = CreateClientSocket("127.0.0.1", 8080);
    if (clientSocket == INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }

    std::string message;
    while (true)
    {
        std::cout << "Enter message to send (type 'exit' to quit): ";
        std::getline(std::cin, message);

        if (message == "exit")
        {
            break;
        }

        if (!SendMessageToServer(clientSocket, message))
        {
            break;
        }

        if (!ReceiveMessageFromServer(clientSocket))
        {
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
