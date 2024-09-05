#include "json_rpc_server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

JsonRPCServer::JsonRPCServer(int port, QObject *parent):
QObject(parent),
WebServer(port)
{
}

/* QOL utility function */
namespace{
template <typename ...Args>
[[nodiscard]] inline bool checkWithMsg(bool statement, const char *format, Args ...args)
{
    if (statement){
        if constexpr (sizeof...(args)){
            std::printf(format, args...);
        }
        else{
            std::cout << format << std::endl;
        }
    }
    return statement;
}
}

void JsonRPCServer::processRequest(const SOCKET client)
{
    auto buffer = http_buffers_.getBuffer();
    CleanUtils::socket_ptr client_ptr(new POLL_FD{.fd = client, .events = POLLIN});
    std::cout << "Processing request" << std::endl;

    ssize_t received = 0;
    ssize_t ret = 0;

    while(true)
    {
        ret = recv(client_ptr->fd, buffer, decltype(buffer)::buffer_size - received, 0);
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
            received += ret;
            QRegularExpressionMatch reg_match = RegularExpressions::http_general_req.match(buffer);
            if (reg_match.hasMatch()){
                auto method = reg_match.captured("http_method");
                if (checkWithMsg(method.isEmpty(),"Can't resolve http method\n")){
                    return;
                }

                if (method == "POST"){
                    reg_match = RegularExpressions::http_json_POST.match(buffer);
                    QString length;
                    if (reg_match.hasMatch() && !(length = reg_match.captured("http_length")).isEmpty()){
                        return receiveMessageBodyPOST(std::move(buffer), length.toInt(), std::move(client_ptr));
                    }
                }
                else if (method == "GET"){
                    return processRequestGET(buffer, std::move(client_ptr));
                }
            }
        }

        ret = poll(client_ptr.get(), 1, 10000);
        if (ret == 1){
            if (client_ptr->revents & POLLHUP){
                std::printf("Client closed connection\n");
                return;
            }
            if (client_ptr->revents & POLLIN){
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
}


void JsonRPCServer::processRequestPOST(const QByteArray &req_body, CleanUtils::socket_ptr &&client)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(req_body, &error);
    if (checkWithMsg(doc.isNull(), "Json parse error: %s\n", error.errorString().toLocal8Bit().data())){
        return sendJsonResponseError(0, ParseError, std::move(client));
    }
    
    QJsonObject root_obj = doc.object();
    QJsonArray root_arr = doc.array();
    if (checkWithMsg(root_obj.isEmpty() && root_arr.isEmpty(), "Json parse error, can't find root object or array\n")){
        return sendJsonResponseError(0, ParseError, std::move(client));
    }

    QJsonValue val;
    val = root_obj["jsonrpc"];
    // if ((root_obj, "Json parse error, can't find root object\n")){
    //     return;
    // }
}

void JsonRPCServer::processRequestGET(QByteArray &&req_body, CleanUtils::socket_ptr &&client)
{
}

void JsonRPCServer::sendJsonResponseValid(int id, const std::string &result, CleanUtils::socket_ptr &&client)
{
}

namespace{
constexpr const char* errCodeToStr(JsonRPCServer::JsonRPCErrorCodes code)
{
    using enum JsonRPCServer::JsonRPCErrorCodes;

    switch(code){
    case NoError:
        return "OK";
    case ParseError:
        return "Parse error";
    case InvalidRequest:
        return "Invalid Request";
    case MethodNotFound:
        return "Method not found";
    case InvalidParams:
        return "Invalid params";
    case InternalError:
        return "Internal error";
    }

    return "";
}

constexpr int errCodeToHTTPCode(JsonRPCServer::JsonRPCErrorCodes code)
{
    using enum JsonRPCServer::JsonRPCErrorCodes;

    switch(code){
    case NoError:
        return 200;
    case InvalidRequest:
        return 400;
    case MethodNotFound:
        return 404;
    case ParseError:
    case InvalidParams:
    case InternalError:
        return 500;
    }
    return 500;
}
}

void JsonRPCServer::sendJsonResponseError(int id, JsonRPCErrorCodes error_code, CleanUtils::socket_ptr &&client)
{
    static constexpr char format[] =\
    R"({"jsonrpc": "2.0", "error": {"code": %d, "message": "%s"}, "id": "%s"})";

    std::string id_s = "";
    id_s = id ? std::to_string(id) : "null";

    enum{SEND_BUF_SIZE = 2048};
    char *send_buffer = new char[SEND_BUF_SIZE];
    memset(send_buffer, 0, SEND_BUF_SIZE);

    /* Ugly workaround for continious memory footprint */
    int http_head_offset = sizeof(HTTPTemplates::json_post_response_fmt) + 50;
    
    /* Populating buffer with json content first, leave 50 bytes extra space for formatting characters */
    size_t json_len = std::snprintf(send_buffer + http_head_offset, SEND_BUF_SIZE - http_head_offset, format, error_code, errCodeToStr(error_code), id_s.data());
    if (checkWithMsg(json_len < 0 && json_len < SEND_BUF_SIZE - http_head_offset, "Insufficient send buffer\n")){
        return;
    }

    /* Populating buffer with http_head at the beginning */
    size_t http_len = std::snprintf(send_buffer, http_head_offset, HTTPTemplates::json_post_response_fmt\
    ,errCodeToHTTPCode(error_code)\
    ,errCodeToStr(error_code)\
    ,json_len);
    /* Moving http_head to the json body */
    memmove(send_buffer + http_head_offset - http_len, send_buffer, http_len);

    client->events = POLLOUT;
    ssize_t sent = 0;
    ssize_t ret = 0;
    
    while (sent < json_len + http_len)
    {
        ret = poll(client.get(), 1, 10000);
        if (ret == 1){
            if (checkWithMsg(client->revents & POLLHUP, "Client closed connection\n")){
                return;
            }
            if (client->revents & POLLOUT){
                ret = send(client->fd, send_buffer + http_head_offset - http_len + sent, json_len + http_len - sent, 0);
                if (int err = GET_SOCKET_ERRNO(); checkWithMsg(ret < 0 && err != EAGAIN, "Send failed, errno %d", err)){
                    return;
                }
                else if (checkWithMsg(ret == 0, "Send timed out, closing connection\n")){
                    return;
                }
                else{
                    sent += ret;
                }
            }
        }
        else if (checkWithMsg(ret == 0, "Client send timeout\n")){
            return;
        }
        else if (checkWithMsg(ret < 0, "Client send poll error, errno %d\n", GET_SOCKET_ERRNO())){
            return;
        }
    }
}
