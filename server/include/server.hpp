//
// Created by apolline on 01/04/25.
//

#pragma once

#include <map>
#include <mutex>
#include <vector>
#include <csignal>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

class Server {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    static Server* getInstance();
    void run();
    ~Server();

private:
    static Server* instance;
    static mutex instanceMutex;

    mutex mtx;
    int serverFd{};
    vector<int> connectedClients;
    map<int, string> clientPseudo;

    struct sockaddr_in6 address{};
    struct sockaddr_in6 clientAddress{};
    socklen_t clientAddrLen {};

    enum class CMD {
        UNKNOWN,
        DISCONNECT,
        CONNECTED,
        RENAME
    };

    Server();
    void setupServer();
    static void setupSignalHeaders();
    static void signalHandler(int signum);
    void closeServer();

    void handleClient(int clientFd);
    std::string setupPseudo(int clientFd);
    CMD serverCommand(const string& cmd, int client);
    string receive(int clientFd);
    void disconnectClient(int clientFd);
    void broadcastMessage(int sender, const char* message);
};
