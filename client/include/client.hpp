#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QShortcut>
#include <QScopedPointer>

/**
* @class Client
* @brief This class represents a simple TCP chat client using Qt.
*
* The `Client` class manages the connection to a server, sending and receiving messages.
* It provides a GUI for user interaction with a chat system.
*/
class Client : public QWidget {
Q_OBJECT

public:
    /*
     * Constructs the Client widget using QT stuff*/
    explicit Client(QWidget *parent = nullptr);
    ~Client() override;

private slots:
    /*
     * Sends messages to the server.
     * */
    void sendMessage();

    /*
     * Reads messages from the server and displays them in the chat area.
     * */
    void readMessage();

private:

    // Manages the network connection to the server
    QScopedPointer<QTcpSocket> socket;

    // UI elements for user interaction
    QLineEdit *lineEdit;
    QTextEdit *textEdit;
    QPushButton *sendButton;

    QHBoxLayout *mainLayout;
    QVBoxLayout *chatLayout;
    QString username;
    QLabel *logLabel;
    QTextEdit *commandsAvailable;
    QShortcut *shortcut;

    /*
     * Configures and initializes the TCP socket.
     * */
    void setupSocket();

    /*
     * Handles disconnections from the server.
     * */
    void handleDisconnection();
};
