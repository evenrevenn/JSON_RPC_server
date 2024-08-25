#ifndef SERVER_H
#define SERVER_H

#include "../common.h"
#include <set>
#include <memory>

namespace CleanUtils{
    class autoCloseSocket{
    public:
        void operator()(SOCKET *sock){CLOSE_SOCKET(*sock); delete sock;}
    };
}

class WebServer
{
public:
    explicit WebServer(int port);
    ~WebServer();

private:
    SOCKET server_socket_;

    using socket_ptr = std::unique_ptr<SOCKET, CleanUtils::autoCloseSocket>;
    using clients_set = std::set<socket_ptr>;
    clients_set client_sockets_;
};

#endif //SERVER_H