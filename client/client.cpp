#include <QInputDialog>
#include <QHostAddress>
#include <QMessageBox>
#include <QDateTime>
#include "include/client.hpp"

Client::Client(QWidget *parent) : QWidget(parent) {
    setupSocket();

    bool setPseudo;
    QString pseudo = QInputDialog::getText(this, "Enter your pseudo",
                                           "Log as (enter your pseudo or leave empty to stay anonymous):",
                                           QLineEdit::Normal, "", &setPseudo);
    if (!setPseudo) exit(0);

    if (pseudo.trimmed().isEmpty()) {
        pseudo = "anonymous client";
    }
    username = pseudo;

    // Send the username to the server
    socket->write(username.toUtf8());

    // UI setup
    logLabel = new QLabel("Log as: " + username, this);
    lineEdit = new QLineEdit(this);
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    sendButton = new QPushButton("Send", this);

    // Display available commands
    commandsAvailable = new QTextEdit(this);
    commandsAvailable->setReadOnly(true);
    commandsAvailable->setHtml("<b>Available commands:</b><br>"
                               "SVR:who - List connected users<br>"
                               "SVR:rename $new_pseudo - change your pseudo<br>"
                               "SVR:disconnect - Disconnect from server");
    commandsAvailable->setStyleSheet("background-color: #E8E8E8; color: #000000; border: 1px solid #ccc; padding: 5px;");
    commandsAvailable->setMaximumWidth(400);

    // Connect signals to slots
    connect(sendButton, &QPushButton::clicked, this, &Client::sendMessage);
    connect(lineEdit, &QLineEdit::returnPressed, this, &Client::sendMessage);
    connect(socket.data(), &QTcpSocket::readyRead, this, &Client::readMessage);
    connect(socket.data(), &QTcpSocket::disconnected, this, &Client::handleDisconnection);

    shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(shortcut, &QShortcut::activated, this, &Client::close);

    // Layout setup
    mainLayout = new QHBoxLayout(this);

    chatLayout = new QVBoxLayout();
    chatLayout->addWidget(logLabel);
    chatLayout->addWidget(textEdit);
    chatLayout->addWidget(lineEdit);
    chatLayout->addWidget(sendButton);

    mainLayout->addLayout(chatLayout);
    mainLayout->addWidget(commandsAvailable);

    setLayout(mainLayout);
}

Client::~Client() {
    socket->close();
}

void Client::handleDisconnection() {
    QMessageBox::information(this, "Disconnected", "You have been disconnected from the server.");
    close();
}

void Client::setupSocket() {
    socket.reset(new QTcpSocket(this));
    socket->connectToHost(QHostAddress("127.0.0.1"), 8080);
    if (!socket->waitForConnected(5000)) {
        QMessageBox::critical(this, "Error", "Could not connect to server");
        exit(1);
    }
}

void Client::sendMessage() {
    QString message = lineEdit->text();
    if (message.isEmpty()) return;

    if (socket->write(message.toUtf8()) == -1) {
        QMessageBox::warning(this, "Error", "Message not sent due to network error.");
        return;
    }

    // display our message in our screen
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    QString customString = QString("<span style='color:#00008B;'><b>You [%1]:</b></span> %2").arg(time, message);
    textEdit->append(customString);

    lineEdit->clear();
}

void Client::readMessage() {
    QString message = socket->readAll();

    // Handle username change confirmation
    if (message.startsWith("Your new username is ")) {
        username = message.mid(21).trimmed();
        logLabel->setText("Log as: " + username);
        textEdit->append("<span style='color:green;'>Your name has been updated to: <b>" + username + "</b></span>");
        return;
    }
    if (message.startsWith("Invalid username.\n")) {
        textEdit->append("<span style='color:red;'><b>Rename failed</b> ");
        return;
    }

    // Handle SVR:who command response
    if (message.startsWith("FROM SERVER")) {
        QStringList parts = message.split("\n", Qt::SkipEmptyParts);
        QString customString = QString("<span style='color:#FFD700; font-weight:bold;'>%1</span><br>")
                .arg(parts[1]);
        for (int i = 2; i < parts.size(); i++)
            customString += parts[i] + "<br>";
        textEdit->append(customString);
        return;
    }

    // Handle standard chat messages
    QRegExp regex(R"((.+) \[(\d{2}:\d{2})\]: (.+))");
    if (regex.indexIn(message) != -1) {
        QString sender = regex.cap(1);
        QString time = regex.cap(2);
        QString content = regex.cap(3);
        QString color = sender.startsWith("SERVER") ? "#FFD700" : "#1E90FF";

        QString customString = QString("<span style='color:%1;'><b>%2 [%3]:</b></span> %4")
                .arg(color, sender, time, content);
        textEdit->append(customString);
    } else {
        textEdit->append(message);
    }
}