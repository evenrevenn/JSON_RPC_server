#ifndef JSON_RPC_SERVER_H
#define JSON_RPC_SERVER_H

#include "server.h"
#include <QObject>
#include <QRegularExpression>
#include "http_recv_buffers.h"

namespace RegularExpressions
{
    static const QRegularExpression http_general_req\
    (R"((?<http_method>\S+) /\S* HTTP/1.1\r\n(?:.*)(?=\r\n\r\n))",\
    QRegularExpression::DotMatchesEverythingOption);

    static const QRegularExpression http_json_POST\
    /* Ugly one-line, but better than escaping characters */
    (R"((?<=POST )(?<http_URI>/\S*) HTTP/1\.1\r\n(?:.*)Content-Type: application/json-rpc 2\.0\r\n(?:.*)Content-Length: (?<http_length>\d+)(?:.*)Accept: application/json-rpc 2\.0\r\n(?:.*)Connection: (?<http_connection>\S+)(?:\r\n[^\r\n]*)?(?=\r\n\r\n))",
    QRegularExpression::DotMatchesEverythingOption);
    
    static const QRegularExpression http_json_GET\
    (R"((?<=GET )(?<http_URI>/\S*) HTTP/1.1\r\n(?:.*)Connection: (?<http_connection>\S+)(?:\r\n[^\r\n]*)?(?=\r\n\r\n))",
    QRegularExpression::DotMatchesEverythingOption);
}

namespace HTTPTemplates
{
    static constexpr char json_post_response_fmt[] =
"HTTP/1.1 %d %s\r\n\
Content-Type: application/json-rpc 2.0\r\n\
Content-Length: %lld\r\n\
Connection: close\r\n\r\n";
}

class JsonRPCServer : public WebServer
{
public:
    enum JsonRPCErrorCodes{
        NoError = 0,
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603,
        NullIDError = -31000,
        ObjectNotFound = -31001
    };

    /** MetaProgramming helper types, that need to be used in target QObject class */
    /** When adding jsonrpc fucntionality use signature : Q_INVOKABLE JsonRPCRet_t <method>(JsonRPCParams_t) */
    /* Type of return value */
    typedef std::pair<QString,JsonRPCServer::JsonRPCErrorCodes> JsonRPCRet_t;
    /* Type of method parameters holder structure */
    typedef const QVariantMap & JsonRPCParams_t;

    explicit JsonRPCServer(int port, QObject *target_root_obj);

protected:
    void processRequest(const SOCKET client) override;

private:
    template<unsigned int buf_size>
    void receiveMessageBodyPOST(QObject *target, httpRecvBufferSingle<buf_size> &&buffer, const int length, CleanUtils::socket_ptr &&client);

    void processRequestPOST(QObject *target, const QByteArray &req_body, CleanUtils::socket_ptr &&client);
    void processRequestGET(QByteArray &&req_body, CleanUtils::socket_ptr &&client);

    void sendJsonResponseValid(int id, const QString &result, CleanUtils::socket_ptr &&client);
    void sendJsonResponseError(int id, JsonRPCErrorCodes error_code, CleanUtils::socket_ptr &&client);
    void sendResponseValidArray();

    httpRecvBuffers<MAX_CLIENTS, 2048> http_buffers_;

    QObject *root_object;
};

template<unsigned int buf_size>
inline void JsonRPCServer::receiveMessageBodyPOST(QObject *target, httpRecvBufferSingle<buf_size>&& buffer, const int length, CleanUtils::socket_ptr && client)
{
    ssize_t recv_length = buffer.receivedContentLength();
    if (recv_length < 0){
        std::printf("Error finding body length\n");
        return;
    }

    while(recv_length < length)
    {
        ssize_t ret = POLL(client.get(), 1, WAIT_TIMEOUT_MS);
        if (ret == 1){
            if (client->revents & POLLHUP){
                std::printf("Client closed connection\n");
                return;
            }
        }
        else if (ret == 0){
            std::printf("Client receive timeout\n");
            return;
        }
        else if (ret < 0){
            std::printf("Client receive poll error, errno %d\n", GET_SOCKET_ERRNO());
            return;
        }

        ret = recv(client->fd, buffer, length - recv_length, 0);
        if (ret < 0){
            auto err = GET_SOCKET_ERRNO();
            if (err != EWOULDBLOCK || err != EAGAIN){
                std::printf("Failed receive data, errno %d\n", err);
                return;
            }
        }
        else if (ret == 0){
            std::printf("Client closed connection\n");
            return;
        }
        else{
            recv_length += ret;
        }
    }

    processRequestPOST(target, buffer.requestBody(length), std::move(client));
}

#endif //JSON_RPC_SERVER_H