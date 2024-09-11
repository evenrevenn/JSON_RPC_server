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
    UserMetaTypes::registerTypes();
    auto database = std::make_shared<QObject>(new DatabaseObj("DatabaseRoot"));

    QObject *obj = database.get();

    QMetaType meta_t = QMetaType::fromName("HyperLink");
    qDebug() << "name:" << meta_t.name();
    QVariant link_v = QVariant(meta_t, nullptr);
    qDebug() << "variant:" << link_v;
    obj->setProperty("Link", link_v);

    qDebug() << "property names:" << obj->dynamicPropertyNames();

    link_v = obj->property("Link");
    QMetaType link_t = link_v.metaType();
    QObject *link_copy = static_cast<QObject *>(link_t.create(link_v.data()));

    QVariant link_copy_v = link_copy->property("url");
    qDebug() << "copy variant before:" << link_copy_v;
    link_copy_v = "www.some.ru";
    link_copy->setProperty("url", link_copy_v);
    qDebug() << "copy variant after:" << link_copy_v;
    
    obj->setProperty("Link", QVariant(link_t, link_copy));
    QDebug & debug = qDebug() << "link after:";

    link_v = obj->property("Link");
    link_copy = static_cast<QObject *>(link_t.create(link_v.data()));
    QByteArray arr;
    QMetaObject::invokeMethod(link_copy, "toHtml", Qt::DirectConnection, Q_RETURN_ARG(QByteArray, arr));
    link_t.debugStream(qDebug() << "from type:", link_v.constData());

    QFile f(QString("test.txt"));
    qDebug() << f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text);
    QDataStream f_stream(&f);
    // f_stream << "arr";
    link_t.save(f_stream, link_v.constData());
    qDebug() << f_stream.status();
    f.close();
    
    ServersWrapper &wrapper = ServersWrapper::getInstance();
    
    JsonRPCServer server1(12345, database.get());
    // JsonRPCServer server2(12346);
    // JsonRPCServer server3(12347);
    
    wrapper.addServer(server1);
    // wrapper.addServer(server1, server2, server3);

    wrapper.startListenLoop();

    QCoreApplication a(argc, argv);

    return a.exec();
}