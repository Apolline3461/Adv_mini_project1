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

    socket->write(username.toUtf8());

    //left part
    logLabel = new QLabel("Log as: " + username, this);
    lineEdit = new QLineEdit(this);
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    sendButton = new QPushButton("Send", this);

    //right part for available commands
    commandsAvailable = new QTextEdit(this);
    commandsAvailable->setReadOnly(true);
    commandsAvailable->setHtml("<b>Available commands:</b><br>"
                              "SVR:who - List connected users<br>"
                              "SVR:disconnect - Disconnect from server");
    commandsAvailable->setStyleSheet("background-color: #E8E8E8; border: 1px solid #ccc; padding: 5px;");
    commandsAvailable->setStyleSheet("background-color: #E8E8E8; color: #000000; border: 1px solid #ccc; padding: 5px;");
    commandsAvailable->setMaximumWidth(200);

    connect(sendButton, &QPushButton::clicked, this, &Client::sendMessage);
    connect(lineEdit, &QLineEdit::returnPressed, this, &Client::sendMessage);
    connect(socket, &QTcpSocket::readyRead, this, &Client::readMessage);
    connect(socket, &QTcpSocket::disconnected, this, &Client::handleDisconnection);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // chat layout
    QVBoxLayout *chatLayout = new QVBoxLayout();
    chatLayout->addWidget(logLabel);
    chatLayout->addWidget(textEdit);
    chatLayout->addWidget(lineEdit);
    chatLayout->addWidget(sendButton);

    // add chat layout into the main one and Command as a widget
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
    socket = new QTcpSocket(this);
    socket->connectToHost(QHostAddress("127.0.0.1"), 8080);
    if (!socket->waitForConnected(5000)) {
        QMessageBox::critical(this, "Error", "Could not connect to server");
        exit(1);
    }
}

void Client::sendMessage() {
    QString message = lineEdit->text();
    if (message.isEmpty()) return;

    socket->write(message.toUtf8());

    QString time = QDateTime::currentDateTime().toString("HH:mm");

    QString customString = QString("<span style='color:#00008B;'><b>You [%1]:</b></span> %2").arg(time, message);
    textEdit->append(customString);

    lineEdit->clear();
}

void Client::readMessage() {
    QString message = socket->readAll();

    if (message.startsWith("FROM SERVER")) {
        QStringList parts = message.split("\n", Qt::SkipEmptyParts);
        QString customString = QString("<span style='color:#FFD700; font-weight:bold;'>%1</span><br>")
                .arg(parts[1]);
        for (int i = 2; i < parts.size(); i++)
            customString += parts[i] + "<br>";
        textEdit->append(customString);
        return;
    }

    QRegExp regex(R"((.+) \[(\d{2}:\d{2})\]: (.+))");

    if (regex.indexIn(message) != -1) {
        QString sender = regex.cap(1);
        QString time = regex.cap(2);
        QString content = regex.cap(3);

        QString customString = QString("<span style='color:#1E90FF;'><b>%1 [%2]:</b></span> %3")
                .arg(sender, time, content);
        textEdit->append(customString);
    } else {
        textEdit->append(message);
    }
}