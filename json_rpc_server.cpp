#include "json_rpc_server.h"

JsonRPCServer::JsonRPCServer(int port, QObject *parent):
QObject(parent),
WebServer(port)
{
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
            if (!reg_match.hasMatch()){
                if (reg_match.captured("http_method") == "POST"){
                    reg_match = RegularExpressions::http_json_POST.match(buffer);
                    QString length;
                    if (reg_match.hasMatch() && !(length = reg_match.captured("http_length")).isEmpty()){
                        return receiveMessageBodyPOST(std::move(buffer), length.toInt(), std::move(client_ptr));
                    }
                }
                else if (reg_match.captured("http_method") == "GET"){
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
}

void JsonRPCServer::processRequestGET(QByteArray &&req_body, CleanUtils::socket_ptr &&client)
{
}
