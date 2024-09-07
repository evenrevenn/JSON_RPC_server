#ifndef DATABASE_OBJ_H
#define DATABASE_OBJ_H

#include <QObject>
#include "json_rpc_server.h"

class DatabaseObj : public QObject
{
Q_OBJECT
public:
    explicit DatabaseObj(QObject *parent = nullptr):QObject(parent)
    { if(parent == nullptr){setObjectName("DatabaseRoot");} }

    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t addProperty(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t deleteProperty(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t listProperties(JsonRPCServer::JsonRPCParams_t params);

    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t addChild(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t deleteChild(JsonRPCServer::JsonRPCParams_t params);
    Q_INVOKABLE JsonRPCServer::JsonRPCRet_t listChildren(JsonRPCServer::JsonRPCParams_t params);

private:
    /* Using rvalue for forwarding reference in case of using lvalue references as Params_t */
    bool extractParamStr(const JsonRPCServer::JsonRPCParams_t &&params, const QString &key, std::string &str) const;
};

#endif //DATABASE_OBJ_H