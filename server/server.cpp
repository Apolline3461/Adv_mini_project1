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

void Server::handleClient(int clientFd) {
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
        connectedClients.push_back(clientFd);
    }

    std::string welcomeMessage = "Welcome " + pseudo + "!\n";
    send(clientFd, welcomeMessage.c_str(), welcomeMessage.size(), 0);

    while (true) {
        ssize_t rdNumber = read(clientFd, buffer, 1024);
        if (rdNumber == -1) {
            perror("Error reading from client");
            break;
        }
        if (rdNumber <= 0) {
            std::cout << "Client nb " << clientFd << " disconnected" << std::endl;
            string msgServer = "The client " + clientPseudo[clientFd] + " has disconnected.\n";
            broadcastMessage(-1, msgServer.c_str());
            break;
        }
        buffer[rdNumber] = '\0';
        string message(buffer);
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
        message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

        std::cout << "MSG {"<< message << "}" << std::endl;
        if (message == "SVR:whoIsConnected") {
            std::cout << "Client want to know who is connected {"<< message <<"}" << std::endl;
            string whoIsConnectedList = "Connected clients:\n";
            for (const auto &connected: clientPseudo) {
                if (connected.first == clientFd)
                    whoIsConnectedList += connected.second + " (YOU)\n";
                else
                    whoIsConnectedList += connected.second;
            }
            send(clientFd, whoIsConnectedList.c_str(), whoIsConnectedList.size(), 0);
        } else if (message == "SVR:disconnect") {
            std::cout << "Client " << clientPseudo[clientFd] << " requested to disconnect" << std::endl;
            string msgServer = "The client " + clientPseudo[clientFd] + " has disconnected.\n";
            broadcastMessage(-1, msgServer.c_str());
            break;
        } else {
            std::cout << "Message received from the client "<< clientFd << std::endl;
            broadcastMessage(clientFd, buffer);
        }
        memset(buffer, 0, sizeof(buffer));
    }
    {
        lock_guard<mutex> lock(mtx);
        connectedClients.erase(remove(connectedClients.begin(), connectedClients.end(), clientFd),
                               connectedClients.end());
        clientPseudo.erase(clientFd);
    }
    close(clientFd);
}

void Server::broadcastMessage(int sender, const char *message) {
    {
        lock_guard<mutex> lock(mtx);
        std::string msg(message);
        msg.erase(std::remove_if(msg.begin(), msg.end(), [](unsigned char c) {
            return !std::isprint(c);
        }), msg.end());
        std::cout << "Message to send = {"<< msg << "}" << std::endl;
        for (int client: connectedClients) {
            if (client != sender) {
                if (send(client, message, strlen(message), 0) == -1)
                    perror("Failed to send message to the client");
                std::cout << "Send message to "<< client << std::endl;
            }
        }
        std::cout << "End of the broadcasting" << std::endl;
    }
}