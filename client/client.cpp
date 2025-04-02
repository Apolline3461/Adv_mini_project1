#include <QInputDialog>
#include <QHostAddress>
#include <QMessageBox>
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

    logLabel = new QLabel("Log as: " + username, this);
    lineEdit = new QLineEdit(this);
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    sendButton = new QPushButton("Send", this);

    connect(sendButton, &QPushButton::clicked, this, &Client::sendMessage);
    connect(socket, &QTcpSocket::readyRead, this, &Client::readMessage);

    layout = new QVBoxLayout(this);
    layout->addWidget(logLabel);
    layout->addWidget(textEdit);
    layout->addWidget(lineEdit);
    layout->addWidget(sendButton);
    setLayout(layout);
}

Client::~Client() {
    delete layout;
    socket->close();
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
    socket->write(message.toUtf8());
    lineEdit->clear();
}

void Client::readMessage() {
    QString message = socket->readAll();
    textEdit->append(message);
}