#include "html_server.h"

HtmlServer::HtmlServer(int port, std::shared_ptr<DatabaseObj> database):
WebServer(port),
database_(database)
{
}

void HtmlServer::processRequest(const SOCKET client)
{
    auto buffer = http_buffers_.getBuffer();
    CleanUtils::socket_ptr client_ptr(new POLL_FD{.fd = client, .events = POLLIN});
    std::cout << "Processing html request" << std::endl;

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
                if (method.isEmpty()){
                    throw std::runtime_error("Can't resolve http method\n");
                    return;
                }

                if (method == "POST"){
                    return processRequestPOST(buffer, std::move(client_ptr));
                }
                else if (method == "GET"){
                    reg_match = RegularExpressions::http_html_GET.match(buffer);
                    auto uri = reg_match.captured("http_method");
                    if (uri.isEmpty()){
                        throw std::runtime_error("Can't resolve http method\n");
                        return;
                    }
                    QStringList uri_list = uri.split('/', Qt::SkipEmptyParts);
                    auto connection = reg_match.captured("http_connection");
                    if (connection == "close"){
                        processRequestGET(uri_list, std::move(client_ptr));
                        
                        return;
                    }
                    else{
                        processRequestGET(uri_list, std::forward<CleanUtils::socket_ptr>(client_ptr));
                    }
                }
            }
        }

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

void HtmlServer::processRequestPOST(QByteArray &&req_body, CleanUtils::socket_ptr &&client)
{
    /* TODO:Add POST processing */
}

void HtmlServer::processRequestGET(const QStringList &uri, CleanUtils::socket_ptr &&client)
{
}
