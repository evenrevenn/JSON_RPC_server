#include "json_rpc_server.h"

JsonRPCServer::JsonRPCServer(int port, QObject *parent):
QObject(parent),
WebServer(port)
{
}

void JsonRPCServer::processRequest(const SOCKET client)
{
    auto buffer = http_buffers_.getBuffer();
    CleanUtils::socket_ptr client_ptr(new POLL_FD{.fd = client});
    std::cout << "Processing request" << std::endl;

    ssize_t received = 0;
    ssize_t ret = 0;

    ret = recv(client_ptr->fd, buffer, decltype(buffer)::buffer_size - received, 0);
    if (ret < 0){
        std::printf("Failed receive data, errno %d", GET_SOCKET_ERRNO());
        return;
    }
}
