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

void ServersWrapper::serversListeningLoop(std::stop_token stopper)
{
    auto &wrapper = ServersWrapper::getInstance();

    while(!stopper.stop_requested())
    {
        std::lock_guard lock(wrapper.servers_lock_);
        int ret = POLL(wrapper.servers_.data(), wrapper.servers_.size(), 100);
        if (ret < 0){
            std::cout << "Server poll error" << std::endl;
            exit(EXIT_FAILURE);
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
                        std::cout << "Couldn't resolve server object from server fd" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    POLL_FD client_fd{.fd = client, .events = POLLIN};

                    map_iter->second.emplace_back(client_fd);
                }
            }
            if (poll_fd.revents & POLLNVAL){
                /* Server object have been destroyed */
                std::printf("Server socket %d closed\n", poll_fd.fd);
                poll_fd.fd = -1;
                invalid_server = true;
            }
        }
        if (invalid_server){
            auto iter = wrapper.servers_.begin();
            POLL_FD invalid{.fd = -1};
            while (iter != wrapper.servers_.end()){
                iter = std::find(wrapper.servers_.begin(), wrapper.servers_.end(), invalid);
                wrapper.servers_.erase(iter);
            }
        }
    }
}