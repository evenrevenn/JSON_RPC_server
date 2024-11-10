#include "servers_wrapper.h"
#include <iostream>

void ServersWrapper::startListenLoop()
{
    if (!loop_handle_.joinable()){
        loop_handle_ = std::jthread(&ServersWrapper::serversListeningLoop);
    } else {
        std::printf("Servers loop already started\n");
    }
}

/**
 * @details Listening loop, that perform polling on prior added servers socket fds.
 * When POLLIN flag gets detected on any fd, new client connection gets accepted and
 * loop resolves the server object reference, using clients_map_. Then client fd gets
 * passed to resolved server's client fds array, so it could be processed by it's own
 * listening loop
 * @throws std::runtime_error if poll returns error
 * @throws std::runtime_error if can't resolve server object from fd
 */
void ServersWrapper::serversListeningLoop(std::stop_token stopper)
{
    std::printf("Servers loop started\n");
    auto &wrapper = ServersWrapper::getInstance();

    while(!stopper.stop_requested())
    {
        std::lock_guard lock(wrapper.servers_lock_);
        int ret = POLL(wrapper.servers_.data(), wrapper.servers_.size(), 100);
        if (ret < 0){
            throw std::runtime_error("Server poll error, errno %d\n", GET_SOCKET_ERRNO());
        }
        else if(ret == 0){
            continue;
        }

        bool invalid_server = false;
        for (auto &poll_fd : wrapper.servers_)
        {
            if (poll_fd.revents & POLLIN){
                SOCKET client = accept(poll_fd.fd, NULL, NULL);
                if (client > 0){
                    SOCKET server_sock = poll_fd.fd;

                    /* Resolving WebServer & from socket fd */
                    auto map_iter = std::find_if(wrapper.clients_map_.begin(),wrapper.clients_map_.end()
                    , [server_sock](decltype(clients_map_)::value_type &elem){return server_sock == elem.first.get().server_socket_;});
                    
                    if (map_iter == wrapper.clients_map_.end()){
                        throw std::runtime_error("Couldn't resolve server object from server fd");
                    }
                    POLL_FD client_fd{.fd = client, .events = POLLIN};

                    map_iter->second.emplace_back(client_fd);
                }
            }
            if (poll_fd.revents & POLLNVAL){
                /* Server object have been destroyed */
                std::printf("Server socket %d closed\n", poll_fd.fd);
                poll_fd.fd = INVALID_SOCKET;
                invalid_server = true;
            }
        }
        if (invalid_server){
            auto iter = wrapper.servers_.begin();
            POLL_FD invalid{.fd = INVALID_SOCKET};
            while (iter != wrapper.servers_.end()){
                iter = std::find(wrapper.servers_.begin(), wrapper.servers_.end(), invalid);
                wrapper.servers_.erase(iter);
            }
        }
    }
    std::printf("Servers loop stop requested\n");
}