#include <QLabel>
#include <QCoreApplication>
#include "servers_wrapper.h"
#include "json_rpc_server.h"
#include "html_server.h"
#include "database_obj.h"

#include <QObject>
#include <QMetaObject>
#include <QMetaProperty>

int main(int argc, char *argv[])
{
    UserMetaTypes::registerTypes();
    auto database = std::make_shared<DatabaseObj>("DatabaseRoot");

    QObject *obj = database.get();

    QMetaType meta_t = QMetaType::fromName("HyperLink");
    QVariant link_v = QVariant(meta_t, nullptr);
    obj->setProperty("Link", link_v);

    link_v = obj->property("Link");
    QMetaType link_t = link_v.metaType();
    QObject *link_copy = static_cast<QObject *>(link_t.create(link_v.data()));

    QVariant link_copy_v = link_copy->property("url");
    link_copy_v = "http://www.some.ru";
    link_copy->setProperty("url", link_copy_v);
    link_copy_v = link_copy->property("text");
    link_copy_v = "some.ru";
    link_copy->setProperty("text", link_copy_v);
    
    obj->setProperty("Link", QVariant(link_t, link_copy));
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    
    JsonRPCServer json_rpc_server(12345, database.get());
    HtmlServer html_server(12346, database);
    
    wrapper.addServer(json_rpc_server, html_server);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}