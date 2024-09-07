#include <QLabel>
#include <QCoreApplication>
#include "servers_wrapper.h"
#include "json_rpc_server.h"
#include "database_obj.h"

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>

int main(int argc, char *argv[])
{
    QObject *obj = new QObject();
    obj->setObjectName("Test obj");
    qDebug() << "set property:" << obj->setProperty("Test prop", "test str");
    qDebug() << "property names:" << obj->dynamicPropertyNames();
    qDebug() << "property read:" << obj->property("Test prop");


    auto meta_obj = obj->metaObject();
    QMetaProperty meta_prop = meta_obj->property(meta_obj->indexOfProperty("Test prop"));
    qDebug() << "meta_property writeable:" << meta_prop.isWritable();
    qDebug() << "meta_property write:" << meta_prop.write(obj, 42);
    qDebug() << "meta_property readable:" << meta_prop.isReadable();
    qDebug() << "meta_property read:" << meta_prop.read(obj);
    qDebug() << "property read:" << obj->property("Test prop");

    auto *database = new DatabaseObj();
    JsonRPCServer server1(12345, database);
    // JsonRPCServer server2(12346);
    // JsonRPCServer server3(12347);
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    wrapper.addServer(server1);
    // wrapper.addServer(server1, server2, server3);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}