#ifndef SERVERS_WRAPPER_H
#define SERVERS_WRAPPER_H

#include "server.h"
#include <vector>
#include <map>
#include <thread>
#include <iostream>

class ServersWrapper
{
public:
    enum {MAX_SERVERS = 3};
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
    explicit ServersWrapper(){/*servers_.reserve(MAX_SERVERS);*/};

    /* Thread function, polls all servers socket fds for new connections */
    static void serversListeningLoop(std::stop_token stopper);

    typedef std::vector<CleanUtils::autoClosedSocketFd> SocketFdsArray;
    SocketFdsArray servers_;
    std::mutex servers_lock_;
    std::jthread loop_handle_;

    std::map<std::reference_wrapper<WebServer>, SocketFdsArray> clients_map_;
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
    int clients_num = Server_t::MAX_CLIENTS;
    if(listen(server_sock, clients_num)){
        std::printf("Can't listen on socket, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }

    auto new_vec = clients_map_.try_emplace(server);
    if (new_vec.second){
        new_vec.first->second.reserve(Server_t::MAX_CLIENTS);
    }
}