#include "server.h"
#include "testhtml.h"
#include <string.h>
#include <memory>
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <atomic>

enum {MAX_CLIENTS = 5};

WebServer::WebServer(int port)
{
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_ == -1){
        std::cout << "Can't create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

// #ifdef _WIN32
//     unsigned long set = 1;
//     if (ioctlsocket(server_socket_, FIONBIO, &set)){
//         std::printf("Can't set socket non-blocking, errno %ld\n", WSAGetLastError());
//         exit(EXIT_FAILURE);
//     }
// #else
//     int flags = fcntl(server_socket_, F_GETFL, 0);
//     if (flags == -1){
//         std::printf("Can't get socket flags, errno %d\n", GET_SOCKET_ERRNO());
//         exit(EXIT_FAILURE);
//     }
//     flags |= O_NONBLOCK;
//     if (fcntl(server_socket_, F_SETFL, flags)){
//         std::printf("Can't set socket non-blocking, errno %d\n", GET_SOCKET_ERRNO());
//         exit(EXIT_FAILURE);
//     }
// #endif

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
    }

    // listen(server_socket_, MAX_CLIENTS);

    // socket_ptr client(new SOCKET);
    // *client = accept(server_socket_, NULL, NULL);

    // std::printf("Accepted\n");

    // send(*client, test_htmls::test_html, sizeof(test_htmls::test_html), 0);

    // while(1);
}

WebServer::~WebServer()
{
    client_sockets_.clear();
    CLOSE_SOCKET(server_socket_);
}

namespace{

/** Static variables moved to translation unit scope instead of WebServer::startListenForClients
 * because of multiple possible template realizations */

/* All listening server sockets */
static std::vector<POLL_FD> s_listening_sockets;

/* Listeners thread stopper, transition is performed upwards */
enum class ListenState{
    Stopped,
    Going,
    StopRequested,
};
static std::atomic<ListenState> s_listen_state = ListenState::Stopped;

void addServerToListeners(SOCKET server_sock)
{    
    /* For possible multithread listeners addition */
    static std::mutex listen_lock;
    std::lock_guard(listen_lock);

    if (std::find_if(s_listening_sockets.begin()\
        ,s_listening_sockets.end()\
        , [server_sock](POLL_FD &fd){return fd.fd == server_sock;})\
        == s_listening_sockets.end())
    {
        return;
    }

    s_listening_sockets.push_back(POLL_FD{.fd = server_sock, .events = POLLIN});
    listen(server_sock, MAX_CLIENTS);
}

/* Thread function, polls all servers socket fds for new connections */
void serversListeningLoop()
{
    while(s_listen_state == ListenState::Going)
    {
        int ret = POLL(s_listening_sockets.data(), s_listening_sockets.size(), 100);
        if (ret < 0){
            std::cout << "Server poll error" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if(ret > 0){
            
        }
    }
    s_listen_state = ListenState::Stopped;
}

}

template <isWebServer ...Server>
void WebServer::startListenForClients(Server &... server)
{
    addServerToListeners(server.server_sock...);
    if (s_listen_state == ListenState::Going){
        s_listen_state
    }
}
