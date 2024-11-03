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

    // database->addProperty(QVariantMap{{"type", "HyperLink"}, {"name", "Link"}});
    // database->setPropertyAttr(QVariantMap{{"property", "Link"}, {"attribute", "url"}, {"data", "http://www.some.ru"}});
    // database->setPropertyAttr(QVariantMap{{"property", "Link"}, {"attribute", "text"}, {"data", "hyper"}});
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    
    JsonRPCServer json_rpc_server(12345, database.get());
    HtmlServer html_server(12346, database);
    
    wrapper.addServer(json_rpc_server, html_server);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}