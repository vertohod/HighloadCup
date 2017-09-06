#include <boost/thread/detail/singleton.hpp>
#include <stdexcept>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <thread>

#include "hl_server.h"
#include "log.h"

using namespace boost::detail::thread;

hl_server::hl_server(const std::string& port) : m_port(port), m_threads_amount(0)
{
//    unsigned int num = std::thread::hardware_concurrency();

    num = 3;

    lo::l(lo::TRASH) << "Threads amount: " << num;

    m_threads_amount = num;
    m_threads.resize(m_threads_amount);
    for (auto& thread_ptr : m_threads) {
        thread_ptr = std::make_shared<processor>();
    }
}

int hl_server::create_and_bind(const std::string& port)
{
    struct addrinfo hints;

    memset (&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    struct addrinfo *result;
    int r = 0;

    if (0 != (r = getaddrinfo(NULL, port.c_str(), &hints, &result))) throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(r)));

    int listen_socket = 0;

    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (-1 == (listen_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))) continue;
        if (0 == bind(listen_socket, rp->ai_addr, rp->ai_addrlen)) break;
        close (listen_socket);
    }

    if (rp == NULL) throw std::runtime_error("Could not bind");

    freeaddrinfo(result);

    return listen_socket;
}

void hl_server::make_socket_non_blocking(int listen_socket)
{
    int flags;

    if (-1 == (flags = fcntl(listen_socket, F_GETFL, 0))) throw std::runtime_error("fcntl(1)");

    flags |= O_NONBLOCK;

    if (-1 == fcntl(listen_socket, F_SETFL, flags)) throw std::runtime_error("fcntl(2)");
}

void hl_server::run()
{
    auto listen_socket = create_and_bind(m_port);
    make_socket_non_blocking(listen_socket);

    if (-1 == listen(listen_socket, 128)) throw std::runtime_error("listen");

    int epoll_fd = 0;
    if (-1 == (epoll_fd = epoll_create1(0))) throw std::runtime_error("epoll_create");

    struct epoll_event event;

    event.data.fd = listen_socket;
    event.events = EPOLLIN;
    if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &event)) throw std::runtime_error("epoll_ctl(1)");

    struct sockaddr in_addr;
    socklen_t in_len = sizeof(in_addr);
    struct epoll_event event_in;

    size_t threads_counter = 0;
    while (true) {

        int res = 0;
        if (-1 == (res = epoll_wait(epoll_fd, &event_in, 1, -1))) throw std::runtime_error("epoll_wait");
        if (res == 0) continue;

        auto evs = event_in.events;
        if ((evs & EPOLLERR) || (evs & EPOLLHUP) || (!(evs & EPOLLIN))) throw std::runtime_error("SOME ERROR");

        while (true) {
            int socket_in = 0;
            if (-1 == (socket_in = accept(listen_socket, &in_addr, &in_len))) break;

            threads_counter = ++threads_counter % m_threads_amount;
            m_threads[threads_counter]->give(socket_in);
        }
    }
    close(listen_socket);
}
