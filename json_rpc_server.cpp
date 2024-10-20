#include "json_rpc_server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "html_server.h"
#include "config.h"

JsonRPCServer::JsonRPCServer(int port, QObject *target_root_obj):
WebServer(port),
root_object(target_root_obj)
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
                    if (reg_match.hasMatch() && !(length = reg_match.captured("http_length")).isEmpty())
                    {
                        QStringList target_path = reg_match.captured("http_URI").split('/', Qt::SkipEmptyParts);
                        QObject *target_obj = root_object;
                        for (auto &step : target_path){
                            target_obj = target_obj->findChild<QObject *>(step, Qt::FindDirectChildrenOnly);
                            if (!target_obj){
                                return sendJsonResponseError(0, ObjectNotFound, std::move(client_ptr));
                            }
                        }

                        return receiveMessageBodyPOST(target_obj, std::move(buffer), length.toInt(), std::move(client_ptr));
                    }
                }
                else if (method == "GET"){
                    auto reg_match_iter = RegularExpressions::http_json_GET.globalMatch(buffer);
                    while (reg_match_iter.hasNext()){
                        reg_match = reg_match_iter.next();
                        return processRequestGET(buffer, std::move(client_ptr));
                    }
                }
            }
        }

        client_ptr->events = POLLIN;
        ret = POLL(client_ptr.get(), 1, WAIT_TIMEOUT_MS);
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


void JsonRPCServer::processRequestPOST(QObject *target, const QByteArray &req_body, CleanUtils::socket_ptr &&client)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(req_body, &error);
    if (checkWithMsg(doc.isNull(), "Json parse error: %s\n", error.errorString().toLocal8Bit().data())){
        qDebug() << req_body;
        return sendJsonResponseError(0, ParseError, std::move(client));
    }
    
    QJsonObject root_obj;
    QJsonArray root_arr;

    if (checkWithMsg(doc.isObject() && doc.isArray(), "Json parse error, can't find root object or array\n")){
        return sendJsonResponseError(0, ParseError, std::move(client));
    }

    if (doc.isObject()){
        root_obj = doc.object();
    }

    QJsonValue val;
    val = root_obj["jsonrpc"];
    if (val.isUndefined() || val.toString() != "2.0"){
        std::printf("JsonRPC error, invalid \"jsonrpc\" field\n");
        return sendJsonResponseError(0, InternalError, std::move(client));
    }

    int id = 0;
    val = root_obj["id"];
    if (!val.isNull()){
        if (!val.isDouble()){
            std::printf("JsonRPC error, invalid \"id\" field\n");
            return sendJsonResponseError(0, InternalError, std::move(client));
        }
        else if (val.toInteger() == 0){
            std::printf("JsonRPC error, \"id\" field equals zero\n");
            return sendJsonResponseError(0, NullIDError, std::move(client));
        }
        else{
            id = val.toInteger();
        }
    }

    val = root_obj["method"];
    if (checkWithMsg(!val.isString(), "JsonRPC error, invalid \"method\" field\n")){
        return sendJsonResponseError(0, InvalidRequest, std::move(client));
    }
    QByteArray method = val.toString().toLocal8Bit();
    JsonRPCParams_t params = root_obj["params"].toObject().toVariantMap();

    JsonRPCRet_t return_val{"", NoError};

    bool result = QMetaObject::invokeMethod(target, method.data(), Qt::BlockingQueuedConnection
                                            ,Q_RETURN_ARG(JsonRPCServer::JsonRPCRet_t, return_val)
                                            ,Q_ARG(JsonRPCServer::JsonRPCParams_t, params));
    if (!result){
        return sendJsonResponseError(id, MethodNotFound, std::move(client));
    }
    if (return_val.second != NoError){
        return sendJsonResponseError(id, return_val.second, std::move(client));
    }

    return sendJsonResponseValid(id, return_val.first, std::move(client));
}

void JsonRPCServer::processRequestGET(QByteArray &&req_body, CleanUtils::socket_ptr &&client)
{
    char buf[512] = "";
    ssize_t http_len = 0;
    http_len = std::snprintf(buf, 512, HTTPTemplates::html_get_response_fmt
    ,200
    ,HTTPConstants::getReasonPhrase(HTTPConstants::OK)
    ,JSON_HTML_SIZE
    ,HTTPConstants::getConnectionText(true));

    client->events = POLLOUT;
    ssize_t sent = 0;
    ssize_t ret = 0;
    
    while (sent < http_len)
    {
        ret = POLL(client.get(), 1, WAIT_TIMEOUT_MS);
        if (ret == 1){
            if (client->revents & POLLHUP){
                throw std::runtime_error("Client closed connection\n");
            }
            if (client->revents & POLLOUT){
                ret = send(client->fd, buf + sent, http_len - sent, 0);
                if (auto err = GET_SOCKET_ERRNO(); ret < 0 && err != EAGAIN){
                    throw std::runtime_error(std::string("Send failed, errno ") + std::to_string(err));
                }
                else if (ret == 0){
                    throw std::runtime_error("Send timed out, closing connection\n");
                }
                else{
                    sent += ret;
                }
            }
        }
        else if (ret == 0){
            throw std::runtime_error("Client send timeout\n");
        }
        else if (ret < 0){
            throw std::runtime_error(std::string("Client send poll error, errno ") + std::to_string(GET_SOCKET_ERRNO()));
        }
    }

    FILE *form = fopen("jsonrpc.html", "rb");
    if (!form){
        std::printf("Can't open jsonrpc.html\n");
        return;
    }

    sent = 0;
    while (sent < JSON_HTML_SIZE)
    {
        ret = POLL(client.get(), 1, WAIT_TIMEOUT_MS);

        if (ret == 1){
            if (client->revents & POLLHUP){
                throw std::runtime_error("Client closed connection\n");
            }
            if (client->revents & POLLOUT)
            {
            #ifdef _WIN32
            /* TODO: add windows sendfile alternative */
            #else
                ret = sendfile(client->fd, fileno(form), nullptr, JSON_HTML_SIZE - sent);
            #endif
                if (auto err = GET_SOCKET_ERRNO(); ret < 0 && err != EAGAIN){
                    throw std::runtime_error(std::string("Send failed, errno ") + std::to_string(err));
                }
                else if (ret == 0){
                    throw std::runtime_error("Send timed out, closing connection\n");
                }
                else{
                    sent += ret;
                }
            }
            else if (ret == 0){
                throw std::runtime_error("Client send timeout\n");
            }
            else if (ret < 0){
                throw std::runtime_error(std::string("Client send poll error, errno ") + std::to_string(GET_SOCKET_ERRNO()));
            }
        }
    }
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
    case NullIDError:
        return "ID is 0";
    case ObjectNotFound:
        return "Object not found";
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
    case ObjectNotFound:
        return 404;
    case ParseError:
    case InvalidParams:
    case InternalError:
    case NullIDError:
        return 500;
    }
    return 500;
}
}

void JsonRPCServer::sendJsonResponseValid(int id, const QString &result, CleanUtils::socket_ptr &&client)
{
    /* Notification */
    if (id == 0){
        return;
    }
    
    static constexpr char format[] =\
    R"({"jsonrpc": "2.0", "result": "%s", "id": "%s"})";

    std::string id_s = std::to_string(id);

    enum{SEND_BUF_SIZE = 4096};
    std::unique_ptr<char []> send_buffer_uniq(new char[SEND_BUF_SIZE]);
    char *send_buffer = send_buffer_uniq.get();
    memset(send_buffer, 0, SEND_BUF_SIZE);

    QByteArray result_b = result.toLocal8Bit();
    /* Calculate json body formatted length */
    ssize_t json_len = std::snprintf(nullptr, 0, format, result_b.constData(), id_s.c_str());
    if (checkWithMsg(json_len < 0 || json_len == SEND_BUF_SIZE, "Insufficient send buffer\n")){
        return;
    }
    /* Populating buffer with http_head at the beginning */
    ssize_t http_len = std::snprintf(send_buffer, SEND_BUF_SIZE - json_len, HTTPTemplates::json_post_response_fmt\
    ,errCodeToHTTPCode(NoError)\
    ,errCodeToStr(NoError)\
    ,json_len);

    if (checkWithMsg(http_len < 0 || http_len == SEND_BUF_SIZE - json_len, "Insufficient send buffer\n")){
        return;
    }
    json_len = std::snprintf(send_buffer + http_len, SEND_BUF_SIZE, format, result_b.constData(), id_s.data());

    client->events = POLLOUT;
    ssize_t sent = 0;
    ssize_t ret = 0;
    
    while (sent < json_len + http_len)
    {
        ret = POLL(client.get(), 1, WAIT_TIMEOUT_MS);
        if (ret == 1){
            if (checkWithMsg(client->revents & POLLHUP, "Client closed connection\n")){
                return;
            }
            if (client->revents & POLLOUT){
                ret = send(client->fd, send_buffer + sent, json_len + http_len - sent, 0);
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


void JsonRPCServer::sendJsonResponseError(int id, JsonRPCErrorCodes error_code, CleanUtils::socket_ptr &&client)
{
    static constexpr char format[] =\
    R"({"jsonrpc": "2.0", "error": {"code": %d, "message": "%s"}, "id": "%s"})";

    std::string id_s = "";
    id_s = id ? std::to_string(id) : "null";

    enum{SEND_BUF_SIZE = 2048};
    std::unique_ptr<char []> send_buffer_uniq(new char[SEND_BUF_SIZE]);
    char *send_buffer = send_buffer_uniq.get();
    memset(send_buffer, 0, SEND_BUF_SIZE);

    /* Calculate json body formatted length */
    ssize_t json_len = std::snprintf(nullptr, 0, format, error_code, errCodeToStr(error_code), id_s.data());
    if (checkWithMsg(json_len < 0 || json_len == SEND_BUF_SIZE, "Insufficient send buffer\n")){
        return;
    }
    /* Populating buffer with http_head at the beginning */
    ssize_t http_len = std::snprintf(send_buffer, SEND_BUF_SIZE - json_len, HTTPTemplates::json_post_response_fmt\
    ,errCodeToHTTPCode(error_code)\
    ,errCodeToStr(error_code)\
    ,json_len);

    if (checkWithMsg(http_len < 0 || http_len == SEND_BUF_SIZE- json_len, "Insufficient send buffer\n")){
        return;
    }
    json_len = std::snprintf(send_buffer + http_len, SEND_BUF_SIZE, format, error_code, errCodeToStr(error_code), id_s.data());

    client->events = POLLOUT;
    ssize_t sent = 0;
    ssize_t ret = 0;
    
    while (sent < json_len + http_len)
    {
        ret = POLL(client.get(), 1, WAIT_TIMEOUT_MS);
        if (ret == 1){
            if (checkWithMsg(client->revents & POLLHUP, "Client closed connection\n")){
                return;
            }
            if (client->revents & POLLOUT){
                ret = send(client->fd, send_buffer + sent, json_len + http_len - sent, 0);
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
