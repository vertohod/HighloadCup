#include <boost/thread/detail/singleton.hpp>
#include <sys/socket.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "http_proto.h"
#include "processor.h"
#include "request.h"
#include "base.h"
#include "util.h"
#include "log.h"

using namespace boost::detail::thread;

processor::processor() : m_stop_flag(false), m_epoll_fd(0)
{
    m_thread = std::make_shared<std::thread>(std::bind(&processor::thread_function, this));
    m_answer200.reserve(8192);
}

processor::~processor()
{
    m_stop_flag = true;
    m_thread->join();
}

void processor::give(int socket)
{
    struct epoll_event event;
    event.data.fd = socket;
    event.events = EPOLLIN;
    if (-1 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, socket, &event)) close(socket);
}

void processor::thread_function()
{
    static const int max_events = 1024;
    static const std::string empty;
    static const size_t buffer_size = 1024;

    std::vector<unsigned char> buffer;
    buffer.resize(buffer_size);

    request req;

    if (-1 == (m_epoll_fd = epoll_create1(0))) return;

    struct epoll_event* events_in = new struct epoll_event [max_events];

    while (!m_stop_flag) {
        int res;
        if (-1 == (res = epoll_wait(m_epoll_fd, events_in, max_events, -1))) break;
        if (res == 0) continue;

        for (int event_num = 0; event_num < res; ++event_num) {

            auto socket = events_in[event_num].data.fd;
            auto evs = events_in[event_num].events;
            if ((evs & EPOLLERR) || (evs & EPOLLHUP) || (!(evs & EPOLLIN))) {
                close(socket);
                continue;
            } 

            auto data_size = read(socket, reinterpret_cast<char*>(&buffer[0]), buffer_size);

            if (data_size == -1) {
                close(socket);
                continue;
            } else if (data_size == 0) {
                close(socket);
                continue;
            }

            bool parse_ok = false;
            try {
                parse_ok = req.parse(buffer, data_size);
            } catch (...) {
                std::string temp(buffer.begin(), buffer.end());
                lo::l(lo::ERROR) << "Can't parse the request (1): " << temp;
            }

            if (parse_ok) this->handle(socket, req);
            else {
                std::string temp(buffer.begin(), buffer.end());
                lo::l(lo::ERROR) << "Can't parse the request (2): " << temp;
                send400(socket);
            }
        }
    }
    delete[] events_in;
}

void processor::handle(int socket, const request& req)
{
    static auto& bs = singleton<base>::instance();

    static const std::string f_fromDate("fromDate");
    static const std::string f_toDate("toDate");
    static const std::string f_country("country");
    static const std::string f_toDistance("toDistance");
    static const std::string f_fromAge("fromAge");
    static const std::string f_toAge("toAge");
    static const std::string f_gender("gender");

    static const std::string empty;
    static const std::string slash("/");

    auto path_ptr = req.path();
    auto pos_1 = path_ptr->find(slash);
    if (pos_1 == std::string::npos) {
        send400(socket);
        return;
    }
    auto pos_2 = path_ptr->find(slash, pos_1 + 1);
    if (pos_2 == std::string::npos) {
        send400(socket);
        return;
    }

    std::string entity;
    std::string id;
    std::string func;

    auto pos_3 = path_ptr->find(slash, pos_2 + 1);
    if (pos_3 != std::string::npos) {
        entity = path_ptr->substr(pos_1 + 1, pos_2 - pos_1 - 1);
        id = path_ptr->substr(pos_2 + 1, pos_3 - pos_2 - 1);
        func = path_ptr->substr(pos_3 + 1, path_ptr->size() - pos_3 - 1);
    } else {
        entity = path_ptr->substr(pos_1 + 1, pos_2 - pos_1 - 1);
        id = path_ptr->substr(pos_2 + 1, path_ptr->size() - pos_2 - 1);
    }

    if (req.method() == GET) {

        if (func.empty()) {

            bool options_ok = true;
            uint32_t id_uint = 0;

            try {
                id_uint = format<uint32_t>(id);
            } catch (const std::exception& e) {
                send404(socket);
                return;
            }

            auto res = bs.get_object(entity, id_uint);

            if (res->empty()) {
                send404(socket);
                return;
            } else {
                send200(socket, *res);
                return;
            }

        } else if (func == "visits" && entity == "users") {

            bool options_ok = true;
            uint32_t id_uint = 0;

            bool is_fromDate = false;
            bool is_toDate = false;
            bool is_country = false;
            bool is_toDistance = false;

            long fromDate = 0;
            long toDate = 0;
            std::string country;
            uint32_t toDistance = 0;

            try {
                id_uint = format<uint32_t>(id);

                if (is_fromDate = req.is_get(f_fromDate)) {
                    fromDate = req.get(f_fromDate, long(0));
                }

                if (is_toDate = req.is_get(f_toDate)) {
                    toDate = req.get(f_toDate, long(0));
                }

                if (is_country = req.is_get(f_country)) {
                    country = req.get(f_country, std::string(empty));
                }

                if (is_toDistance = req.is_get(f_toDistance)) {
                    toDistance = req.get(f_toDistance, uint32_t(0));
                }

            } catch (...) {
                options_ok = false;
            }

            if (options_ok) {

                auto res = bs.get_visits(id_uint, is_fromDate, fromDate, is_toDate, toDate, is_country, country, is_toDistance, toDistance);
                if (res->empty()) {
                    send404(socket);
                    return;
                } else {
                    send200(socket, *res);
                    return;
                }

            } else {
                send400(socket);
                return;
            }

        } else if (func == "avg" && entity == "locations") {

            bool options_ok = true;
            uint32_t id_uint = 0;

            bool is_fromDate = false;
            bool is_toDate = false;
            bool is_fromAge = false;
            bool is_toAge = false;
            bool is_gender = false;

            long fromDate = 0;
            long toDate = 0;
            uint32_t fromAge = 0;
            uint32_t toAge = 0;
            char gender;

            try {
                id_uint = format<uint32_t>(id);

                if (is_fromDate = req.is_get(f_fromDate)) {
                    fromDate = req.get(f_fromDate, long(0));
                }

                if (is_toDate = req.is_get(f_toDate)) {
                    toDate = req.get(f_toDate, long(0));
                }
                
                if (is_fromAge = req.is_get(f_fromAge)) {
                    fromAge = req.get(f_fromAge, uint32_t(0));
                }

                if (is_toAge = req.is_get(f_toAge)) {
                    toAge = req.get(f_toAge, uint32_t(0));
                }

                if (is_gender = req.is_get(f_gender)) {
                    auto temp = req.get(f_gender, empty);
                    if (temp != "m" && temp != "f") options_ok = false;
                    else gender = temp[0];
                }

            } catch (...) {
                options_ok = false;
            }

            if (options_ok) {

                auto res = bs.get_mark(id_uint, is_fromDate, fromDate, is_toDate, toDate, is_fromAge, fromAge, is_toAge, toAge, is_gender, gender);
                if (res.empty()) {
                    send404(socket);
                    return;
                } else {
                    send200(socket, res);
                    return;
                }

            } else {
                send400(socket);
                return;
            }

        } else {
            send400(socket);
            return;
        }

    } else if (req.method() == POST) {

        auto data = req.data();

        if (data[0] == 0 || !func.empty()) {
            send400(socket);
            return;
        }

        if (id == "new") {

            if (bs.insert(entity, data)) {
                send_brackets(socket);
                return;
            } else {
                send400(socket);
                return;
            }

        } else {

            bool options_ok = true;
            uint32_t id_uint = 0;

            try {
                id_uint = format<uint32_t>(id);
            } catch (...) {
                options_ok = false;
            }

            int res_update = 400;
            if (options_ok) res_update = bs.update(id_uint, entity, data);

            if (res_update == 200) {
                send_brackets(socket);
                return;
            } else if (res_update == 400) {
                send400(socket);
                return;
            } else if (res_update == 404) {
                send404(socket);
                return;
            }

        }

    }

    send400(socket);
    return;
}
