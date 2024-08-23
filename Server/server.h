#include "../common.h"
#include <vector>

class WebServer
{
public:
    explicit WebServer(int port);
    ~WebServer();

private:
    SOCKET server_socket_;
    std::vector<SOCKET> client_socket_;
};