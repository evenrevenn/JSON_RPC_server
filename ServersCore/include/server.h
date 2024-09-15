#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <vector>
#include <memory>
#include <iostream>
#include <string.h>
#include <thread>
#include <mutex>
#include <type_traits>

namespace CleanUtils{
    class autoCloseSocket{
    public:
        void operator()(POLL_FD *sock){CLOSE_SOCKET(sock->fd); delete sock;}
    };
    // using socket_ptr = std::unique_ptr<POLL_FD, CleanUtils::autoCloseSocket>;
    typedef std::unique_ptr<POLL_FD, CleanUtils::autoCloseSocket> socket_ptr;

    /* RAII poll fd wrapper, calls close() on socket fd on destruction */
    struct autoClosedSocketFd : public POLL_FD{
        autoClosedSocketFd(const POLL_FD &other) noexcept
        :POLL_FD(other){}
        
        autoClosedSocketFd(autoClosedSocketFd &&other) noexcept
        {fd = std::move(other.fd); events = std::move(other.events); other.fd = INVALID_SOCKET;}

        autoClosedSocketFd & operator= (autoClosedSocketFd &&other) noexcept
        {fd = std::move(other.fd); events = std::move(other.events);
        other.fd = INVALID_SOCKET; return *this;}

        autoClosedSocketFd(POLL_FD &&other) noexcept
        :POLL_FD(other){}
        
        ~autoClosedSocketFd(){/*std::printf("Destructor %d addr: 0x%p\n", fd, this);*/ CLOSE_SOCKET(fd);}
        friend bool operator== (const autoClosedSocketFd &rhs, const autoClosedSocketFd &lhs)
        {return rhs.fd == lhs.fd;}
    };
    static_assert(std::is_standard_layout_v<autoClosedSocketFd> == true);
    typedef std::vector<CleanUtils::autoClosedSocketFd> SocketFdsArray;
}

class WebServer;
template <typename T>
concept isWebServer = std::is_base_of_v<WebServer, T>;

class WebServer
{
public:
    explicit WebServer(int port);
    ~WebServer();
    enum {MAX_CLIENTS = 5, WAIT_TIMEOUT_MS = 5000};

    friend bool operator< (const WebServer &lhs, const WebServer &rhs){
        return lhs.server_socket_ < rhs.server_socket_;
    }

friend class ServersWrapper;
// template <isWebServer Server_t>
// friend void ServersWrapper::addOneServer(Server_t &server);

protected:
    virtual void processRequest(const SOCKET client) = 0;

private:
    void startListeningLoop(CleanUtils::SocketFdsArray &clients_fds);
    void clientsListeningLoop(std::stop_token stopper, CleanUtils::SocketFdsArray &clients_fds);
    void processReqThreadF(const SOCKET client)
    {
        int id = req_id_++;
        std::printf("Processing request %d\n", id);
        try{ processRequest(client); }
        catch(std::runtime_error &e){ std::cerr << "Error while processing "<< id << ": " << e.what() << std::endl; return;}
        std::printf("Processed successfully %d\n", id);
    }

    /* Inner id for easier request handling track */
    static inline std::atomic<int> req_id_ = 0;

    SOCKET server_socket_;

    std::jthread loop_handle_;
    std::stop_token loop_stopper_;
    std::mutex clients_lock_;
};

#endif //SERVER_H