#ifndef HTTP_RECV_BUFFERS_H
#define HTTP_RECV_BUFFERS_H

#include <atomic>
#include <array>
#include <QString>

/* Instance of buffer */
template <unsigned int buf_size>
class httpRecvBufferSingle
{
public:
    enum {buffer_size = buf_size};
    ~httpRecvBufferSingle(){busy_flag = false;}

    operator void*(){return buffer.toLocal8Bit().data() + buffer.toLocal8Bit().length();}
    operator QString&(){return buffer;}
    operator bool(){return busy_flag;}

template <unsigned int N, unsigned int buf_size_pool>
friend class httpRecvBuffers;

private:
    httpRecvBufferSingle(std::atomic<bool> &flag, QString &buffer):busy_flag(flag),buffer(buffer){}
    std::atomic<bool> &busy_flag;
    QString &buffer;
};

/* QString-based buffers pool, each one gets acquired with object-lifetime-based exclusive token */
template <unsigned int N, unsigned int buf_size>
class httpRecvBuffers
{
public:
    httpRecvBuffers():busy_flags{false}{for(auto &buffer : buffers){buffer.reserve(buf_size);}}

    /* Getting personal buffer from buffers pool */
    [[nodiscard]] httpRecvBufferSingle<buf_size> getBuffer()
    {
        while (true){
            for (int i = 0; i < N; i++)
            {
                if (busy_flags[i] == false){
                    busy_flags[i] = true;
                    buffers[i].clear();
                    return {busy_flags[i], buffers[i]};
                }
            }
        }
    }

private:
    std::array<std::atomic<bool>, N> busy_flags;
    std::array<QString, N> buffers;
};

#endif //HTTP_RECV_BUFFERS_H