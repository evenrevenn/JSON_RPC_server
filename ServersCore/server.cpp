#include "server.h"
#include "testhtml.h"
#include <string.h>
#include <memory>
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <atomic>

WebServer::WebServer(int port)
{
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_ == -1){
        std::cout << "Can't create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    unsigned long set = 1;
    if (ioctlsocket(server_socket_, FIONBIO, &set)){
        std::printf("Can't set socket non-blocking, errno %ld\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
#else
    int flags = fcntl(server_socket_, F_GETFL, 0);
    if (flags == -1){
        std::printf("Can't get socket flags, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;
    if (fcntl(server_socket_, F_SETFL, flags)){
        std::printf("Can't set socket non-blocking, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }
#endif

    int opt_val = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int)) < 0){
        std::printf("Can't set socket reuse addr, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEPORT, &opt_val, sizeof(int)) < 0){
        std::printf("Can't set socket reuse port, errno %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (bind(server_socket_, (sockaddr *)&serv_addr, sizeof(serv_addr)) == 0){
        sockaddr_in sock_ad;
        socklen_t len = 1;
        getsockname(server_socket_, (sockaddr *)&sock_ad, &len);
        std::printf("Bound to address: %s:%d\n" \
            , inet_ntoa(sock_ad.sin_addr) \
            , ntohs(sock_ad.sin_port));
    }
    else{
        std::printf("Can't bind to address, errno = %d\n", GET_SOCKET_ERRNO());
        exit(EXIT_FAILURE);
    }
}

WebServer::~WebServer()
{
    CLOSE_SOCKET(server_socket_);
}

void WebServer::startListeningLoop(CleanUtils::SocketFdsArray &clients_fds)
{
    if (!loop_handle_.joinable()){
    loop_handle_ = std::jthread(&WebServer::clientsListeningLoop, this,
                                loop_stopper_,
                                std::ref(clients_fds));
    } else {
        std::printf("Clients loop already started\n");
    }
}

void WebServer::clientsListeningLoop(std::stop_token stopper, CleanUtils::SocketFdsArray &clients_fds)
{
    while(!stopper.stop_requested())
    {
        std::lock_guard lock(clients_lock_);
        int ret = POLL(clients_fds.data(), clients_fds.size(), 100);
        if (ret < 0){
            std::cout << "Clients poll error" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if(ret == 0){
            continue;
        }

        bool client_client = false;
        for (auto &poll_fd : clients_fds)
        {
            if (poll_fd.revents & POLLIN){
                std::thread req_thread(&WebServer::processRequest, this, poll_fd.fd);
                req_thread.detach();

                /* Dropping descriptor, responce and connection close is on processRequest */
                poll_fd.fd = -1;
                client_client = true;
            }
            else if (poll_fd.revents & POLLHUP){
                /* Client closed connection */
                poll_fd.fd = -1;
                client_client = true;
            }
        }
        /* Cleaning hang or processing clients */
        if (client_client){
            auto iter = clients_fds.begin();
            POLL_FD invalid{.fd = -1};
            while (iter != clients_fds.end()){
                iter = std::find(clients_fds.begin(), clients_fds.end(), invalid);
                clients_fds.erase(iter);
                iter = clients_fds.begin();
            }
        }
    }
}
