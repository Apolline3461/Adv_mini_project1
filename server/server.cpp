#include <iomanip>
#include "include/server.hpp"

Server* Server::instance = nullptr;
mutex Server::instanceMutex;

Server* Server::getInstance() {
    lock_guard<mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = new Server();
    }
    return instance;
}

Server::Server() {
    setupSignalHeaders();
    setupServer();
}

Server::~Server() {
    closeServer();
    instance = nullptr;
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
    CMD cmd;
    string message;
    std::string welcomeMessage = "Welcome " + setupPseudo(clientFd) + "!\n";
    send(clientFd, welcomeMessage.c_str(), welcomeMessage.size(), 0);

    while (true) {
        message = receive(clientFd);
        if (message.empty()) continue;
        if (message == "SVR:DISCONNECTED") break;
        cmd = serverCommand(message, clientFd);
        if (cmd == CMD::DISCONNECT) break;

        if (cmd != CMD::UNKNOWN) continue;

        std::cout << "Message {" << message << "} received from the client fd "<< clientFd << std::endl;
        broadcastMessage(clientFd, message.c_str());
        message.clear();
    }
    disconnectClient(clientFd);
}

std::string Server::setupPseudo(int clientFd) {
    char buffer[32] = {0};

    ssize_t rdNumber = read(clientFd, buffer, sizeof(buffer) - 1);

    if (rdNumber <= 0) {
        {
            lock_guard<mutex> lock(mtx);
            clientPseudo[clientFd] = "anonymous client";
        }
        return "anonymous client";
    }
    buffer[rdNumber] = '\0';
    string pseudo(buffer);

    pseudo.erase(std::remove_if(pseudo.begin(), pseudo.end(), [](char c) {
        return c == '\n' || c == '\r';
    }), pseudo.end());

    if (pseudo.empty()) pseudo = "anonymous client";

    {
        lock_guard<mutex> lock(mtx);
        clientPseudo[clientFd] = pseudo;
    }
    return pseudo;
}

Server::CMD Server::serverCommand(const string& cmd, int client) {
    if (cmd == "SVR:who") {
        std::cout << "Client want to know who is connected" << std::endl;

        string whoIsConnectedList = "FROM SERVER\nConnected clients:\n";
        for (const auto &connected: clientPseudo) {
            if (connected.first == client)
                whoIsConnectedList += connected.second + " (YOU)\n";
            else
                whoIsConnectedList += connected.second + "\n";
        }
        send(client, whoIsConnectedList.c_str(), whoIsConnectedList.size(), 0);
        return CMD::CONNECTED;
    }
    if (cmd == "SVR:disconnect") {
        std::cout << "Client "<< client << "want to disconnect" << std::endl;
        return CMD::DISCONNECT;
    }
    if (cmd.rfind("SVR:rename ", 0) == 0) {
        std::string newPseudo = cmd.substr(11);
        newPseudo.erase(std::remove(newPseudo.begin(), newPseudo.end(), '\n'), newPseudo.end());
        newPseudo.erase(std::remove(newPseudo.begin(), newPseudo.end(), '\r'), newPseudo.end());

        if (newPseudo.empty()) {
            send(client, "Invalid username.\n", 18, 0);
            return CMD::CONNECTED;
        }

        lock_guard<mutex> lock(mtx);
        clientPseudo[client] = newPseudo;
        string confirmMsg = "Your new username is " + newPseudo + "\n";
        send(client, confirmMsg.c_str(), confirmMsg.size(), 0);
        return CMD::RENAME;
    }
    return CMD::UNKNOWN;
}

string Server::receive(int clientFd) {
    char buffer[1024] = {0};
    ssize_t rdNumber = read(clientFd, buffer, 1023);

    if (rdNumber == -1) {
        perror("Error reading from client");
        return "";
    }
    if (rdNumber == 0)
        return "SVR:DISCONNECTED";
    buffer[rdNumber] = '\0';
    string message(buffer);
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
    memset(buffer, 0, sizeof(buffer));
    return message;
}

void Server::disconnectClient(int clientFd) {
    std::string msgServer = "The client ";
    {
        lock_guard<mutex> lock(mtx);
        std::cout << "Disconnection of client " << clientFd << " in progress..." << std::endl;

        std::cout << "Connected client size avant = " << connectedClients.size() << std::endl;

        auto it = std::find(connectedClients.begin(), connectedClients.end(), clientFd);
        if (it != connectedClients.end()) {
            connectedClients.erase(it);
        }
        if (clientPseudo.find(clientFd) != clientPseudo.end()) {
            msgServer += clientPseudo[clientFd] + " has disconnected.\n";
            clientPseudo.erase(clientFd);
            std::cout << "Client pseudo erased." << std::endl;

        }
        shutdown(clientFd, SHUT_RDWR);
        close(clientFd);
    }
    if (!connectedClients.empty()) {
        broadcastMessage(-1, msgServer.c_str());
    }
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M");
    return oss.str();
}

void Server::broadcastMessage(int sender, const char *message) {
    lock_guard<mutex> lock(mtx);

    string msgToBdr = clientPseudo[sender] + " [" + getCurrentTime() + "]: " + message + "\n";
    for (int client: connectedClients) {
        if (client != sender) {
            if (send(client, msgToBdr.c_str(), msgToBdr.size(), 0) == -1)
                perror("Failed to send message to the client");
            std::cout << "Send message to "<< client << std::endl;
        }
    }
    std::cout << "End of the broadcasting" << std::endl;
}