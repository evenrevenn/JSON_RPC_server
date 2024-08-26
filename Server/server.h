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

class WebServer;
template <typename T>
concept isWebServer = std::is_base_of<WebServer, T>::value;

class WebServer
{
public:
    explicit WebServer(int port);
    ~WebServer();

    /* Collect all server sockets fds in fd_set and poll 
     * them in another thread for incoming connections */
    template <isWebServer ...Server>
    static void startListenForClients(Server & ...servers);

private:
    SOCKET server_socket_;

    using socket_ptr = std::unique_ptr<SOCKET, CleanUtils::autoCloseSocket>;
    using clients_set = std::set<socket_ptr>;
    clients_set client_sockets_;
};

#endif //SERVER_H