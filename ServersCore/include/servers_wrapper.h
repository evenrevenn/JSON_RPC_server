#ifndef SERVERS_WRAPPER_H
#define SERVERS_WRAPPER_H

#include "server.h"
#include <vector>
#include <map>
#include <thread>
#include <iostream>

/* Singletone wrapper that contains all server sockets fds and associated client sockets fds */
class ServersWrapper
{
public:
    static ServersWrapper &getInstance(){static ServersWrapper wrapper; return wrapper;}

    // ~ServersWrapper();
    ServersWrapper(const ServersWrapper &other) = delete;

    void startListenLoop();
    void stopListenLoop(){loop_handle_.request_stop(); loop_handle_.join();}

    template <isWebServer... Server_t>
    void addServer(Server_t &... server){(addOneServer(server),...);}

    template <isWebServer Server_t>
    void addOneServer(Server_t & server);

private:
    explicit ServersWrapper()
    {
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);
    if(WSAStartup(wVersionRequested, &wsaData)!=0){ exit(EXIT_FAILURE); }
#endif
    };

    /* Thread function, polls all servers socket fds for new connections */
    static void serversListeningLoop(std::stop_token stopper);

    CleanUtils::SocketFdsArray servers_;
    std::mutex servers_lock_;
    std::jthread loop_handle_;

    std::map<std::reference_wrapper<WebServer>, CleanUtils::SocketFdsArray> clients_map_;
};
#endif //SERVERS_WRAPPER_H

template <isWebServer Server_t>
inline void ServersWrapper::addOneServer(Server_t &server)
{
    std::lock_guard lock(servers_lock_);

    SOCKET server_sock = server.server_socket_;
    if (std::find_if(servers_.begin(), servers_.end()\
        ,[server_sock](POLL_FD &fd){return fd.fd == server_sock;}\
        ) != servers_.end())
    {
        return;
    }

    servers_.emplace_back(POLL_FD{.fd = server_sock, .events = POLLIN});
    if(listen(server_sock, Server_t::MAX_CLIENTS)){
        std::printf("Can't listen on socket, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }

    auto new_vec = clients_map_.try_emplace(server);
    if (new_vec.second){
        new_vec.first->second.reserve(Server_t::MAX_CLIENTS);
    }
    server.startListeningLoop(new_vec.first->second);
}