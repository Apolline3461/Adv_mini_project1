#pragma once

#include <QWidget>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class Client : public QWidget {
Q_OBJECT

public:
    explicit Client(QWidget *parent = nullptr);
    ~Client() override;

private slots:
    void sendMessage();
    void readMessage();

private:
    QTcpSocket *socket{};
    QLineEdit *lineEdit;
    QTextEdit *textEdit;
    QPushButton *sendButton;

    void setupSocket();
};
