#ifndef SERVERS_WRAPPER_H
#define SERVERS_WRAPPER_H

#include "server.h"
#include <vector>
#include <map>
#include <thread>
#include <iostream>

class ServersWrapper;

/**
 * @brief 
 * Singletone wrapper that contains all server sockets fds and associated client sockets fds
 * 
 */
class ServersWrapper
{
public:
    /// @brief Singletone accessor
    /// @return Wrapper reference
    static ServersWrapper &getInstance(){static ServersWrapper wrapper; return wrapper;}

    // ~ServersWrapper();
    ServersWrapper(const ServersWrapper &other) = delete;

    /// @brief Start polling incoming connections on servers wrapped in new thread loop
    void startListenLoop();
    /// @brief Stop servers polling
    void stopListenLoop(){loop_handle_.request_stop(); loop_handle_.join();}

    /** @brief Add multiple servers to wrapper to allow polling its incoming connections
     * Calling this method invokes WebServer::startListeningLoop() for each server
     * @tparam ...Server_t WebServer-based class types
     * @param ...server List of server references to add
    */
    template <isWebServer... Server_t>
    void addServers(Server_t &... server){(addOneServer(server),...);}

    /// @brief Same as addServers() method, but for one server
    /// @tparam Server_t WebServer-based class type
    /// @param server Server reference to add
    template <isWebServer Server_t>
    void addOneServer(Server_t & server);

private:
    /// @brief Private c-tor, on Windows invokes WSAStartup()
    explicit ServersWrapper()
    {
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);
    if(WSAStartup(wVersionRequested, &wsaData)!=0){ exit(EXIT_FAILURE); }
#endif
    };

    ///@brief Thread function, polls all servers socket fds for new connections
    static void serversListeningLoop(std::stop_token stopper);

    /// @brief Array of auto-closed servers fds, used in servers loop to poll incoming connections
    CleanUtils::SocketFdsArray servers_;
    /// @brief Mutex, used to syncronize new servers addition
    std::mutex servers_lock_;
    /// @brief Listening loop thread handle
    std::jthread loop_handle_;

    /** @brief Map with corresponding client socket fds array and server references
     * @details This map contains pairs of WebServer reference and its client fds array.
     * It's used to pass new clients fd to corresponding server,
     * so it could be then processed by server's own listening loop.
     * @todo change reference to shared_ptr
    */
    std::map<std::reference_wrapper<WebServer>, CleanUtils::SocketFdsArray> clients_map_;
};
#endif //SERVERS_WRAPPER_H

/**
 * @details Tries to add < em>server< !em> to wrapper, so new incoming connections could be passed to
 * < em>server< !em> listening loop. If < em>server< !em> fd is already presented, function returns without
 * addition. If listen() call on fd returns error, function throws std::runtime_error
 * @throws std::runtime_error with listen() errno code
 */
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
        throw std::runtime_error("Can't listen on socket, errno %d\n", GET_SOCKET_ERRNO());
    }

    auto new_vec = clients_map_.try_emplace(server);
    if (new_vec.second){
        new_vec.first->second.reserve(Server_t::MAX_CLIENTS);
    }
    server.startListeningLoop(new_vec.first->second);
}