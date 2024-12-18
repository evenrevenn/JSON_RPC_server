#ifndef HTTP_RECV_BUFFERS_H
#define HTTP_RECV_BUFFERS_H

#include <atomic>
#include <array>
#include <QString>
#include <string>

/* Instance of buffer */
template <unsigned int buf_size>
class httpRecvBufferSingle
{
public:
    enum {buffer_size = buf_size};
    ~httpRecvBufferSingle(){busy_flag = false;}

    httpRecvBufferSingle(httpRecvBufferSingle &&other):busy_flag(other.busy_flag),buffer(other.buffer){}

    ssize_t receivedContentLength() const
    {
        const char *http_end = "\r\n\r\n";
        const char *body_start = std::strstr(buffer, http_end);
        return body_start ? strlen(body_start + 4) : -1;
    }
    QByteArray requestBody(ssize_t length) const
    {
        const char *http_end = "\r\n\r\n";
        const char *body_start = std::strstr(buffer, http_end);
        return body_start ? QByteArray::fromRawData(body_start + 4, length) : QByteArray();
    }

    /* Cleaning when overloaded, worse than circular buffer, but easier to convert to QString and QByteArray */
    /* Return amount of cleaned memory */
    /* Leaving data in front of last http message untouched */
    size_t cleanBuffer() const
    {
        const char *leftover = buffer;
        const char *temp = std::strstr(buffer, "\r\n\r\n");
        if (!temp){
            return 0;
        }
        
        while(temp){
            leftover = temp + 4;
            temp = std::strstr(temp + 4, "\r\n\r\n");
        }
        auto leftover_len = buf_size - (leftover - buffer);
        if (leftover_len < buf_size / 2){
            memcpy(buffer, leftover, leftover_len);
        }
        else {
            memmove(buffer, leftover, leftover_len);
        }
        
        return buf_size - leftover_len;
    }

    operator void*(){return buffer + strnlen(buffer, buf_size);}
    operator char*(){return buffer + strnlen(buffer, buf_size);}
    operator QString() const{return QString::fromLocal8Bit(buffer, strnlen(buffer, buf_size));} // Deep copy
    operator QByteArray() const{return QByteArray::fromRawData(buffer, buf_size);} // No deep copy here
    operator bool() const{return busy_flag;}

template <unsigned int N, unsigned int buf_size_pool>
friend class httpRecvBuffers;

private:
    httpRecvBufferSingle(std::atomic<bool> &flag, char *buffer):busy_flag(flag),buffer(buffer){}
    std::atomic<bool> &busy_flag;
    char *buffer;
};

/* Buffers pool, acess to each one gets acquired with lifetime-based exclusive token */
template <unsigned int N, unsigned int buf_size>
class httpRecvBuffers
{
public:
    /* Allocating memory for buffers and assigning QString to each one */
    httpRecvBuffers()
    {
        for(int i = 0; i < N; i++){
            busy_flags[i] = false;
            mem_pool[i] = new char[buf_size];
        }
    }

    /* Waiting for every buffer to get released */
    ~httpRecvBuffers()
    {
        while(true)
        {
            for(int i = 0; i < N; i++)
            {
                if (busy_flags[i] == false){
                    busy_flags[i] = true;
                    delete[] mem_pool[i];
                }
            }
        }
    }

    /* Getting personal buffer from buffers pool */
    [[nodiscard]] httpRecvBufferSingle<buf_size> getBuffer()
    {
        while (true){
            for (int i = 0; i < N; i++)
            {
                if (busy_flags[i] == false){
                    busy_flags[i] = true;
                    memset(mem_pool[i], 0, buf_size);
                    return {busy_flags[i], mem_pool[i]};
                }
            }
        }
    }

private:
    std::array<std::atomic<bool>, N> busy_flags;
    char *mem_pool[N];
};

#endif //HTTP_RECV_BUFFERS_H