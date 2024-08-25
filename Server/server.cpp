#include "server.h"
#include "testhtml.h"
#include <string.h>
#include <memory>
#include <iostream>
#include <fcntl.h>

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

    listen(server_socket_, 5);

    socket_ptr client(new SOCKET);
    *client = accept(server_socket_, NULL, NULL);

    std::printf("Accepted\n");

    send(*client, test_htmls::test_html, sizeof(test_htmls::test_html), 0);

    while(1);
}

WebServer::~WebServer()
{
    client_sockets_.clear();
    CLOSE_SOCKET(server_socket_);
}
