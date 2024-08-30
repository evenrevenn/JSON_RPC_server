#include <QLabel>
#include <QCoreApplication>
#include "server.h"
#include "servers_wrapper.h"

int main(int argc, char *argv[])
{
    WebServer server1(12345);
    WebServer server2(12346);
    WebServer server3(12347);
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    wrapper.addServer(server1, server2, server3);
    wrapper.addServer(server1);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}