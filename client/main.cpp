//
// Created by apolline on 01/04/25.
//

#include <QApplication>
#include "include/client.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Client client;
    client.show();
    return app.exec();
}