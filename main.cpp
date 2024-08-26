#include <QLabel>
#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    WebServer server1(12345);
    WebServer server2(12346);
    WebServer::startListenForClients(server1, server2);

    QCoreApplication a(argc, argv);

    return a.exec();
}