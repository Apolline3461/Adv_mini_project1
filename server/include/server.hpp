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

/**
* @class Server
* @brief This class represents a server.
*
* The `Sever` class handles the connection of several client.
*/
class Server {
public:
    /* To deny the possibility to create several instances of server, we disable copy constructor and assigment */
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    static Server* getInstance(); // Returns the unique instance of Server
    static void deleteInstance(); // Delete the unique instance of Server

    void run(); // Starts the main server loop

    ~Server();

private:
    static std::unique_ptr<Server> instance;
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

    Server(); // Private to prevent direct instantiation and multiple definition

    /*
     * Creates an IPv6 TCP socket.
     * Binds it to port 8080.
     * Listening for incoming connections*/
    void setupServer();

    /*
     * Registers signal handlers for SIGINT (Ctrl+C) and SIGTERM
     * */
    static void setupSignalHeaders();

    /*
     * Handles termination signals.
     * Ensures the server is closed gracefully.*/
    static void signalHandler(int signum);

    /*
     * Closes all client sockets and the server socket.
     */
    void closeServer();

    /*
     * Handles a single client session in a dedicated thread.
     *
     * Receives and processes commands.*/
    void handleClient(int clientFd);

    /*
     * Reads the client’s username (pseudo).
     * Store it in a List*/
    std::string setupPseudo(int clientFd);

    /*
     * Parses commands sent by the client:
     * "SVR:who" → Lists connected clients.
     * "SVR:disconnect" → Disconnects the client.
     * "SVR:rename newName" → Changes the client’s username.
     * */
    CMD serverCommand(const string& cmd, int client);

    /*
     * Reads incoming messages from the client.
     * */
    string receive(int clientFd);


    /*
     * Removes the client from the active list.
     * Broadcasts a "client has disconnected" message.
     */
    void disconnectClient(int clientFd);

    /*
     * Sends a message to all clients, except the sender.
     * */
    void broadcastMessage(int sender, const char* message);
};
