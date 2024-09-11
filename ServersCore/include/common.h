#pragma once

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <sys/socket.h> 
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <poll.h>
    #include <sys/stat.h>
#endif

#ifdef _WIN32
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERRNO() (WSAGetLastError())
    #define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
    #define POLL(fdarray, fds, timeout) WSAPoll(fdarray, fds, timeout)
    #define POLL_FD pollfd
    #define OPTVAL_T const char
    #define SO_REUSEPORT SO_BROADCAST
    #define SLEEP_MS(t) Sleep(t)
    typedef signed long long ssize_t;
#else
    #define SOCKET int
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERRNO() (errno)
    #define IS_VALID_SOCKET(s) ((s) >= 0)
    #define POLL(fds, nfds, timeout) poll(fds, nfds, timeout)
    #define POLL_FD pollfd
    #define INVALID_SOCKET -1
    #define SLEEP_MS(t) sleep(t)
    #define OPTVAL_T int
#endif

#define MAX_DATA_BUFFER_SIZE 2048