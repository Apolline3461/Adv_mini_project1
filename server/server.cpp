#include "include/server.hpp"

Server* Server::instance = nullptr;

Server::Server() {
    instance = this;
    setupSignalHeaders();
    setupServer();
}

Server::~Server() {
    closeServer();
}

void Server::setupSignalHeaders() {
    struct sigaction sa{};
    sa.sa_handler = Server::signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, nullptr);  // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr); // kill
}

void Server::setupServer() {
    clientAddrLen = sizeof(clientAddress);

    if ((serverFd = socket(AF_INET6, SOCK_STREAM, 0)) == 0) {
        perror("Server socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin6_family = AF_INET6;
    address.sin6_addr = IN6ADDR_ANY_INIT;
    address.sin6_port = htons(8080);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Server bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0) {
        perror("Server listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Waiting for connections on port 8080..." << std::endl;
}

void Server::run() {
    while (true) {
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddress, (socklen_t*)&clientAddrLen);

        if(clientFd < 0) {
            if (errno == EINTR)
                continue;
            perror("Failed to accept a new client");
            continue;
        }
        char clientIP[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &clientAddress.sin6_addr, clientIP, sizeof(clientIP));
        cout << "New client connected :" << clientFd << endl;

        {
            lock_guard<mutex> lock(mtx);
            connectedClients.push_back(clientFd);
        }
        thread(&Server::handleClient, this, clientFd).detach();
    }
}

void Server::signalHandler(int signum) {
    std::cout << "\nStopping server..." << std::endl;
    if (instance)
        instance->closeServer();
    exit(signum);
}

void Server::closeServer() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int client : connectedClients) {
        close(client);
    }
    close(serverFd);
    std::cout << "Server closed properly." << std::endl;
}

void Server::broadcastMessage(int sender, const char *message) {
    lock_guard<mutex> lock(mtx);

    for (int client: connectedClients) {
        if (client != sender) {
            if (send(client, message, strlen(message), 0) == -1)
                perror("Failed to send message to the client");
            std::cout << "Send message to "<< client << std::endl;
        }
    }
    std::cout << "End of the broadcasting" << std::endl;
}

void Server::handleClient(int clientFd) {
    string message;
    char buffer[1024] = {0};
    std::string welcomeMessage = "Welcome " + setupPseudo(clientFd) + "!\n";
    send(clientFd, welcomeMessage.c_str(), welcomeMessage.size(), 0);

    while (true) {
        message = receive(clientFd, buffer);
        if (message.empty()) continue;

        if (serverCommand(message, clientFd) == 1) continue;

        std::cout << "Message {" << message << "} received from the client fd "<< clientFd << std::endl;
        message += '\n';
        broadcastMessage(clientFd, message.c_str());
        memset(buffer, 0, sizeof(buffer));
        message.clear();
    }
//    disconnectClient(clientFd);
}

std::string Server::setupPseudo(int clientFd) {
    char buffer[1024] = {0};

    std::string pseudo = "anonymousClient";
    ssize_t rdNumber = read(clientFd, buffer, sizeof(buffer));
    if (rdNumber > 0) {
        buffer[rdNumber] = '\0';
        if (strlen(buffer) > 0) {
            pseudo = buffer;
            pseudo.erase(std::remove(pseudo.begin(), pseudo.end(), '\n'), pseudo.end());
            pseudo.erase(std::remove(pseudo.begin(), pseudo.end(), '\r'), pseudo.end());
        }
        if (pseudo.empty())
            pseudo = "anonymousClient";
    }
    {
        lock_guard<mutex> lock(mtx);
        clientPseudo[clientFd] = pseudo;
    }
    return pseudo;
}

int Server::serverCommand(const string& cmd, int client) {
    if (cmd == "SVR:whoIsConnected") {
        std::cout << "Client want to know who is connected" << std::endl;

        string whoIsConnectedList = "Connected clients:\n";
        for (const auto &connected: clientPseudo) {
            if (connected.first == client)
                whoIsConnectedList += connected.second + " (YOU)\n";
            else
                whoIsConnectedList += connected.second + "\n";
        }
        send(client, whoIsConnectedList.c_str(), whoIsConnectedList.size(), 0);
        return 1;
    }
    if (cmd == "SVR:disconnect") {
        std::cout << "Client " << clientPseudo[client] << " requested to disconnect" << std::endl;
        disconnectClient(client);
        return 1;
    }
    return 0;
}

string Server::receive(int clientFd, char *buffer) {
    ssize_t rdNumber = read(clientFd, buffer, 1023);

    if (rdNumber == -1) {
        perror("Error reading from client");
        memset(buffer, 0, rdNumber);
        return "";
    }
    if (rdNumber <= 0) {
        std::cout << "Client nb " << clientFd << " disconnected" << std::endl;
        string msgServer = "The client " + clientPseudo[clientFd] + " has disconnected.\n";
        broadcastMessage(-1, msgServer.c_str());
        memset(buffer, 0, rdNumber);
        return "";
    }
    buffer[rdNumber] = '\0';
    string message(buffer);
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

    return message;
}

void Server::disconnectClient(int clientFd) {
    {
        lock_guard<mutex> lock(mtx);

        connectedClients.erase(remove(connectedClients.begin(), connectedClients.end(), clientFd),
                               connectedClients.end());
        string msgServer = "The client " + clientPseudo[clientFd] + " has disconnected.\n";
        broadcastMessage(-1, msgServer.c_str());
        clientPseudo.erase(clientFd);
        close(clientFd);
    }
}
