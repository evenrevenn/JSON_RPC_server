#include <QLabel>
#include <QCoreApplication>
#include "servers_wrapper.h"
#include "json_rpc_server.h"

int main(int argc, char *argv[])
{
    JsonRPCServer server1(12345);
    JsonRPCServer server2(12346);
    JsonRPCServer server3(12347);
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    wrapper.addServer(server1, server2, server3);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}