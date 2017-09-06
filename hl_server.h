#ifndef HL_SERVER_H
#define HL_SERVER_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <list>

#include "processor.h"

class hl_server
{
private:
    std::string m_port;

    size_t m_threads_amount;
    std::vector<std::shared_ptr<processor>> m_threads;

public:
    hl_server(const std::string& port);
    void run();

private:
    static int create_and_bind(const std::string& port);
    static void make_socket_non_blocking(int sfd);
    void close_connection(size_t counter);
};

#endif
