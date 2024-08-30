#ifndef JSON_RPC_SERVER_H
#define JSON_RPC_SERVER_H

#include "server.h"
#include <QObject>
#include <QRegularExpression>
#include "http_recv_buffers.h"

namespace RegularExpressions
{
    static const QRegularExpression http_json_reg("GET|POST ?* HTTP/1.1");
}

class JsonRPCServer : public QObject, public WebServer
{
Q_OBJECT
public:
    explicit JsonRPCServer(int port, QObject *parent = nullptr);

protected:
    void processRequest(const SOCKET client) override;

private:
    httpRecvBuffers<MAX_CLIENTS, 1024> http_buffers_;
};

#endif //JSON_RPC_SERVER_H