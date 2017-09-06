#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <queue>

#include "request.h"

class processor
{
private:
    std::atomic<bool> m_stop_flag;
    int m_epoll_fd;

    std::shared_ptr<std::thread> m_thread;

    std::string m_answer200;

public:
    processor();
    ~processor();

    void give(int socket);

private:
    void thread_function();
    void handle(int socket, const request& req);

    inline void send200(int socket, const std::string& data)
    {
        static const std::string answer200 = "HTTP/1.1 200 OK\r\n"
            "Status: 200 OK\r\n"
            "Server: SP (0.1)\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: ";

        if (m_answer200.empty()) m_answer200.assign(answer200);
        else (m_answer200.resize(answer200.size()));
        m_answer200 += std::to_string(data.length()) + "\r\n\r\n" + data;
        write(socket, &m_answer200[0], m_answer200.size());
    }

    inline void send404(int socket)
    {
        static const std::string answer404 = "HTTP/1.1 404 Not Found\r\n"
            "Status: 404 Not Found\r\n"
            "Server: SP (0.1)\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: 0\r\n\r\n";

        write(socket, &answer404[0], answer404.size());
    }

    inline void send400(int socket)
    {
        static const std::string answer400 = "HTTP/1.1 400 Bad Request\r\n"
            "Status: 400 Bad Request\r\n"
            "Server: SP (0.1)\r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: 0\r\n\r\n";

        write(socket, &answer400[0], answer400.size());
    }

    inline void send_brackets(int socket)
    {
        static const std::string answer200 = "HTTP/1.1 200 OK\r\n"
            "Status: 200 OK\r\n"
            "Server: SP (0.1)\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: 2\r\n\r\n{}";

        write(socket, &answer200[0], answer200.size());
    }
};

#endif
