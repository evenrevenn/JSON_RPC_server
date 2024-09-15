#ifndef HTML_SERVER_H
#define HTML_SERVER_H

#include "server.h"
#include "database_obj.h"
#include <QRegularExpression>

namespace RegularExpressions
{
    static const QRegularExpression http_html_GET\
        (R"((?<=GET )(?<http_URI>/\S*) HTTP/1\.1\r\n(?:.*)Connection: (?<http_connection>\S+)(?:\r\n[^\r\n]*)*(?=\r\n\r\n))",
        QRegularExpression::DotMatchesEverythingOption);
}

namespace HTTPTemplates
{
static constexpr char html_get_response_fmt[] =
"HTTP/1.1 %d %s\r\n\
Version: HTTP/1.1\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: %lu\r\n\
Connection: %s\
\r\n\r\n";

static constexpr char html_error_response_fmt[] =
"HTTP/1.1 %d %s\r\n\
Version: HTTP/1.1\r\n\
Connection: %s\
\r\n\r\n";

}

namespace HTTPConstants{

enum StatusCodes{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500
};

typedef char const * ReasonPhrase;
constexpr ReasonPhrase getReasonPhrase(StatusCodes code);

typedef char const * Connection;
constexpr Connection getConnectionText(const bool close);
}

class HtmlServer : public WebServer
{
public:
    explicit HtmlServer(int port, std::shared_ptr<DatabaseObj> database);

    void processRequest(const SOCKET client) override;

private:
    void processRequestPOST(QByteArray &&req_body, CleanUtils::socket_ptr &&client);
    template <bool close>
    void processRequestGET(const QStringList &uri, CleanUtils::socket_ptr &&client);

    template <HTTPConstants::StatusCodes code, bool close = true>
    void sendResponse(CleanUtils::socket_ptr &&client, FILE *f = nullptr, size_t content_len = 0);

    std::shared_ptr<DatabaseObj> database_;

    httpRecvBuffers<MAX_CLIENTS, 2048> http_buffers_;
};


template <bool close>
inline void HtmlServer::processRequestGET(const QStringList &uri, CleanUtils::socket_ptr &&client)
{
    DatabaseObj *resource = database_.get();
    for (const QString &step : uri){
        resource = resource->findChild<DatabaseObj *>(step, Qt::FindDirectChildrenOnly);
        if (!resource){
            sendResponse<HTTPConstants::NOT_FOUND, close>(std::forward<CleanUtils::socket_ptr>(client));
            throw(std::runtime_error("Can't find corresponding object"));
        }
    }

    auto html = resource->getHTMLFile();
    if (!html){
        sendResponse<HTTPConstants::NOT_FOUND, close>(std::forward<CleanUtils::socket_ptr>(client));
        throw(std::runtime_error("Can't open corresponding html page"));
    }

    sendResponse<HTTPConstants::OK, close>(std::forward<CleanUtils::socket_ptr>(client), html, html.length());
}

template <HTTPConstants::StatusCodes code, bool close>
inline void HtmlServer::sendResponse(CleanUtils::socket_ptr &&client, FILE *f, size_t content_len)
{
    char buf[512] = "";
    ssize_t http_len = 0;
    if (f == nullptr){
        http_len = std::snprintf(buf, 512, HTTPTemplates::html_error_response_fmt, code, getReasonPhrase(code), HTTPConstants::getConnectionText(close));
    }
    else {
        http_len = std::snprintf(buf, 512, HTTPTemplates::html_get_response_fmt, code, getReasonPhrase(code), content_len, HTTPConstants::getConnectionText(close));
    }

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

    sent = 0;
    while (sent < content_len)
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
                ret = sendfile(client->fd, fileno(f), nullptr, content_len - sent);
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
        }
        else if (ret == 0){
            throw std::runtime_error("Client send timeout\n");
        }
        else if (ret < 0){
            throw std::runtime_error(std::string("Client send poll error, errno ") + std::to_string(GET_SOCKET_ERRNO()));
        }
    }
}

#endif //HTML_SERVER_H
