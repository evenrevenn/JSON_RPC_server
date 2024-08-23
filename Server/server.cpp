#include "server.h"
#include <string.h>
#include <memory>
#include <iostream>

WebServer::WebServer(int port)
{
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_ == -1){
        std::cout << "Can't create socket" << std::endl;
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
    }

    listen(server_socket_, 5);
    accept(server_socket_, NULL, NULL);

    std::printf("Accepted\n");
}

WebServer::~WebServer()
{
}
