#include <QLabel>
#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    WebServer server(12345);

    QCoreApplication a(argc, argv);

    return a.exec();
}