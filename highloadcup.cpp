#include <boost/thread/detail/singleton.hpp>
#include <fstream>
#include <string>
#include <execinfo.h>
#include <signal.h>

#include "hl_server.h"
#include "base.h"
#include "util.h"
#include "log.h"

using namespace boost::detail::thread;

void print_backtrace(int sig)
{
    constexpr int ARRAYSIZE = 50;

    void *array[ARRAYSIZE];
    char **strings;

    int size = backtrace(array, ARRAYSIZE);
    strings = backtrace_symbols(array, size);

    lo::l(lo::ERROR) << "Signal: " << sig;
    if (strings != nullptr) {
        for (int count = 0; count < size; ++count) {
            lo::l(lo::ERROR) << strings[count];
        }
    }
    free(strings);
}

void load(const std::string& path_data)
{
    static const std::string path_temp("./data");
    static const std::string path_zip("/tmp/data/data.zip");

    lo::l(lo::TRASH) << "Wait data...";

    while (true) {
        if (util::file_exists(path_zip)) {
            util::sleep(1000);
            lo::l(lo::TRASH) << "Start unzip";
            auto pid = util::exec_command("unzip -uo " + path_zip + " -d " + path_temp);
            lo::l(lo::TRASH) << "Waiting process finish";
            util::wait_command(pid);
            lo::l(lo::TRASH) << "Process is finished";
            break;
        } else {
            util::sleep(100);
        }
    }

    lo::l(lo::TRASH) << "Load data...";

    static auto& bs = singleton<base>::instance();

    std::ifstream   ifs;
    std::string     str_in;

    auto files_list = util::get_files_list(path_data);

    for (auto file_ptr : *files_list) {

        auto pos = file_ptr->rfind("_");
        if (pos == std::string::npos) continue;

        auto entity = file_ptr->substr(0, pos);

        std::string path_file = path_data + "/" + *file_ptr;
        lo::l(lo::TRASH) << path_file;

        // FILE read
        ifs.open(path_file.c_str(), std::ifstream::in);
        if (ifs.fail()) continue; 

        size_t count = 0;

        bool first = true;
        bool brace_open = false;
        std::string buffer;
        for (char ch = ifs.get(); ifs.good(); ch = ifs.get()) {
            if (ch == '{' && first) {
                first = false;
                continue;
            }
            if (ch == '{') {
                brace_open = true;
                buffer.clear();
            }
            buffer += ch;
            if (ch == '}' && brace_open) {
                brace_open = false;
                bs.insert(entity, buffer.c_str());
            }
        }
        ifs.close();
    }

    bs.print_size();
}

void heating()
{
    static auto& bs = singleton<base>::instance();
    
    lo::l(lo::TRASH) << "Heating...";

    auto size = bs.visits_size();
    for (uint32_t count = 1; count < size; ++count) {
        sptr_str res1 = bs.get_object("visits", count);
        sptr_str res2;
        sptr_str res3;
        
        if (count < bs.users_size()) {
            res2 = bs.get_object("users", count);
        }
        if (count < bs.locations_size()) {
            res3 = bs.get_object("locations", count);
        }

        if (1 == count % 100000) {
            lo::l(lo::TRASH) << count;
            lo::l(lo::TRASH) << *res1;
            lo::l(lo::TRASH) << *res2;
            lo::l(lo::TRASH) << *res3;
        }
    }
}

int main(int argc, char* argv[])
{
    signal(SIGSEGV, print_backtrace);
    signal(SIGABRT, print_backtrace);

    //-------------------------

    lo::set_log_level(lo::TRASH);

    //-------------------------
    
    if (argc != 3) {
        lo::l(lo::ERROR) << "Useing: highloadcup <port> <path_data>";
        return 1;
    }

    load(argv[2]);

    hl_server server(argv[1]);

    lo::l(lo::TRASH) << "Start...";
    try {
        server.run();
    } catch (const std::exception& e) {
        lo::l(lo::FATAL) << e.what();
    }

    return 0;
}
