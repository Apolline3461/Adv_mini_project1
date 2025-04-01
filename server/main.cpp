#include <iostream>
#include "include/server.hpp"

int main() {
    Server *svr = Server::getInstance();

    svr->run();
    return 0;
}
