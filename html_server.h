#ifndef HTML_SERVER_H
#define HTML_SERVER_H

#include "server.h"
#include "database_obj.h"
#include <QRegularExpression>

namespace RegularExpressions
{
    static const QRegularExpression http_html_GET\
        (R"((?<=GET )(?<http_URI>/\S*) HTTP/1.1\r\n(?:.*)Connection: (?<http_connection>\S+)(?:\r\n[^\r\n]*)?(?=\r\n\r\n))",
        QRegularExpression::DotMatchesEverythingOption);
}

class HtmlServer : public WebServer
{
public:
    explicit HtmlServer(int port, std::shared_ptr<DatabaseObj> database);

    void processRequest(const SOCKET client) override;

private:
    void processRequestPOST(QByteArray &&req_body, CleanUtils::socket_ptr &&client);
    void processRequestGET(const QStringList &uri, CleanUtils::socket_ptr &&client);

    std::shared_ptr<DatabaseObj> database_;

    httpRecvBuffers<MAX_CLIENTS, 2048> http_buffers_;
};

#endif //HTML_SERVER_H