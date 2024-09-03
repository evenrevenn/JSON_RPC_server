#ifndef JSON_RPC_SERVER_H
#define JSON_RPC_SERVER_H

#include "server.h"
#include <QObject>
#include <QRegularExpression>
#include "http_recv_buffers.h"

namespace RegularExpressions
{
    static const QRegularExpression http_general_req\
    (R"((?<http_method>/S+) /\S* HTTP/1.1\r\n(?:.*)(?=\r\n\r\n))");

    static const QRegularExpression http_json_POST\
    /* Ugly one-line, but better than escaping characters */
    (R"((?<=POST )(?<http_URI>/\S*) HTTP/1.1\r\n(?:.*)Content-Type: application/json-rpc 2.0\r\n(?:.*)Content-Length: (?<http_length>\d+)(?:.*)Connection: (?<http_connection>\S+)\r\n(?=\r\n\r\n))",
    QRegularExpression::DotMatchesEverythingOption);
    
    static const QRegularExpression http_json_GET\
    (R"((?<=GET )(?<http_URI>/\S*) HTTP/1.1\r\n(?:.*)Connection: (?<http_connection>\S+)\r\n(?=\r\n\r\n))",
    QRegularExpression::DotMatchesEverythingOption);
}

class JsonRPCServer : public QObject, public WebServer
{
Q_OBJECT
public:
    explicit JsonRPCServer(int port, QObject *parent = nullptr);

protected:
    void processRequest(const SOCKET client) override;

private:
    template<unsigned int buf_size>
    void receiveMessageBodyPOST(httpRecvBufferSingle<buf_size> &&buffer, const int length, CleanUtils::socket_ptr &&client);

    void processRequestPOST(const QByteArray &req_body, CleanUtils::socket_ptr &&client);
    void processRequestGET(QByteArray &&req_body, CleanUtils::socket_ptr &&client);

    httpRecvBuffers<MAX_CLIENTS, 2048> http_buffers_;
};

template<unsigned int buf_size>
inline void JsonRPCServer::receiveMessageBodyPOST(httpRecvBufferSingle<buf_size>&& buffer, const int length, CleanUtils::socket_ptr && client)
{
    ssize_t recv_length = buffer.requestBodyLength();
    if (recv_length < 0){
        std::printf("Error finding body length\n");
        return;
    }

    while(recv_length < length)
    {
        ssize_t ret = recv(client->fd, buffer, length - recv_length, 0);
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
        ret = poll(client.get(), 1, 10000);
        if (ret == 1){
            if (client->revents & POLLHUP){
                std::printf("Client closed connection\n");
                return;
            }
            if (client->revents & POLLIN){
                continue;
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
    }

    processRequestPOST(buffer.requestBody(), std::move(client));
}

#endif //JSON_RPC_SERVER_H