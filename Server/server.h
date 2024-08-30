#ifndef SERVER_H
#define SERVER_H

#include "../common.h"
#include <set>
#include <memory>
#include <iostream>
#include <string.h>

namespace CleanUtils{
    class autoCloseSocket{
    public:
        void operator()(POLL_FD *sock){CLOSE_SOCKET(sock->fd); delete sock;}
    };
    // using socket_ptr = std::unique_ptr<POLL_FD, CleanUtils::autoCloseSocket>;
    typedef std::unique_ptr<POLL_FD, CleanUtils::autoCloseSocket> socket_ptr;

    struct autoClosedSocketFd : public POLL_FD{
        autoClosedSocketFd(const POLL_FD &other) noexcept
        {fd = other.fd; events = other.events;}
        
        autoClosedSocketFd(autoClosedSocketFd &&other) noexcept
        {fd = std::move(other.fd); events = std::move(other.events); other.fd = -1;}

        autoClosedSocketFd & operator= (autoClosedSocketFd &&other) noexcept
        {fd = std::move(other.fd); events = std::move(other.events);
        other.fd = -1; return *this;}

        autoClosedSocketFd(POLL_FD &&other) noexcept
        {fd = std::move(other.fd); events = std::move(other.events);}
        
        ~autoClosedSocketFd(){std::printf("Destructor %d addr: 0x%p\n", fd, this); CLOSE_SOCKET(fd);}
        friend bool operator== (const autoClosedSocketFd &rhs, const autoClosedSocketFd &lhs)
        {return rhs.fd == lhs.fd;}
    };
    static_assert(std::is_standard_layout_v<autoClosedSocketFd> == true);
}

class WebServer;
template <typename T>
concept isWebServer = std::is_base_of<WebServer, T>::value;


class WebServer
{
public:
    explicit WebServer(int port);
    ~WebServer();
    enum {MAX_CLIENTS = 5};

    friend bool operator< (const WebServer &lhs, const WebServer &rhs){
        return lhs.server_socket_ < rhs.server_socket_;
    }

friend class ServersWrapper;
// template <isWebServer Server_t>
// friend void ServersWrapper::addOneServer(Server_t &server);

private:
    SOCKET server_socket_;
};

#endif //SERVER_H