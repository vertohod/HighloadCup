#ifndef BASE_H
#define BASE_H

#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <vector>
#include <time.h>
#include <string>
#include <memory>
#include <mutex>
#include <queue>

#include <google/dense_hash_map>

#include "types.h"
#include "json.h"

using google::dense_hash_map;

class user;
class location;
class visit;

class base
{
private:
    dense_hash_map<uint32_t, std::shared_ptr<user>> m_users;            // id, user
    dense_hash_map<uint32_t, std::shared_ptr<visit>> m_visits;          // id, visit
    dense_hash_map<uint32_t, std::shared_ptr<location>> m_locations;    // id, location

    std::mutex m_user_mutex;
    std::mutex m_visit_mutex;
    std::mutex m_location_mutex;

    // user id, visit id
    dense_hash_map<uint32_t, std::unordered_set<uint32_t>> m_user_visit;
    // location id, visit id
    dense_hash_map<uint32_t, std::unordered_set<uint32_t>> m_location_visit;

    struct tm m_tm;
    std::vector<std::pair<long, long>> m_ages;

    struct task
    {
        task();
        task(std::shared_ptr<visit> new_ptr, std::shared_ptr<visit> old_ptr, bool update);

        std::shared_ptr<visit>  m_new_ptr;
        std::shared_ptr<visit>  m_old_ptr;
        bool                    m_update;
    };

    std::queue<task> m_tasks;
    std::mutex m_tasks_mutex;
    std::atomic<bool> m_data_visit_flag;

    std::queue<std::shared_ptr<user>> m_tasks_user;
    std::mutex m_tasks_user_mutex;
    std::atomic<bool> m_data_user_flag;

    std::queue<std::shared_ptr<location>> m_tasks_location;
    std::mutex m_tasks_location_mutex;
    std::atomic<bool> m_data_location_flag;

    std::atomic<bool> m_stop_flag;
    std::condition_variable m_cv;

    std::shared_ptr<std::thread> m_thread_ptr;
    void thread_function();

    char m_buffer_for_avg[32];

public:
    base();
    ~base();
    sptr_str get_object(const std::string& name, uint32_t id);
    sptr_str get_visits(uint32_t id, bool is_fromDate, long fromDate, bool is_toDate, long toDate, bool is_country, const std::string& country, bool is_toDistance, uint32_t toDistance);
    const std::string get_mark(uint32_t id, bool is_fromDate, long fromDate, bool is_toDate, long toDate, bool is_fromAge, long fromAge, bool is_toAge, long toAge, bool is_gender, char gender);

    bool insert(const std::string& name, const char* json);
    int update(uint32_t id, const std::string& name, const char* json);

    void print_size();
    size_t users_size();
    size_t visits_size();
    size_t locations_size();

private:
    inline void initialize_ages()
    {
        time_t rawtime;
        time(&rawtime);

        auto tm = gmtime(&rawtime);
        memcpy(&m_tm, tm, sizeof(struct tm));

        m_ages.resize(100);
        for (long age = 0; age < 100; ++age) {
            m_ages[age] = generate_timestamp_from_age(age);
        }
    }

    inline std::pair<long, long> generate_timestamp_from_age(long age)
    {
        struct tm tm_local1(m_tm);
        struct tm tm_local2(m_tm);

        tm_local1.tm_year -= age;
        tm_local1.tm_sec = 59;
        tm_local1.tm_min = 59;
        tm_local1.tm_hour = 23;

        tm_local2.tm_year -= age;
        tm_local2.tm_sec = 0;
        tm_local2.tm_min = 0;
        tm_local2.tm_hour = 0;

        return std::make_pair(mktime(&tm_local1) + 1, mktime(&tm_local2) - 1);
    }

    inline long get_timestamp_fromAge(long age)
    {
        if (age > 99) return generate_timestamp_from_age(age).first;
        return m_ages[age].first;
    }

    inline long get_timestamp_toAge(long age)
    {
        if (age > 99) return generate_timestamp_from_age(age).second;
        return m_ages[age].second;
    }
};

class user
{
public:
    uint32_t        m_id;
    std::unique_ptr<std::string>    m_email;        // 100
    std::unique_ptr<std::string>    m_first_name;   // 50
    std::unique_ptr<std::string>    m_last_name;    // 50
    char            m_gender;                       // m/f
    long            m_birth_date;                   // 01.01.1930 - 01.01.1999    

    sptr_str        m_serialization;

    user();
    user(user&);
    void move(user&);

    bool deserializate(const char*, bool update = false);

    sptr_str serializate(bool force = false);
    void serializate(std::string&);
};

class location
{
public:
    uint32_t        m_id;
    std::unique_ptr<std::string>    m_place;        // unlimited
    std::unique_ptr<std::string>    m_country;      // 50
    std::unique_ptr<std::string>    m_city;         // 50
    uint32_t        m_distance;

    sptr_str        m_serialization;

    location();
    location(location&);
    void move(location&);

    bool deserializate(const char*, bool update = false);

    sptr_str serializate(bool force = false);
    void serializate(std::string&);
};

class visit
{
public:
    uint32_t        m_id;
    uint32_t        m_location;
    uint32_t        m_user;
    long            m_visited_at;   // 01.01.2000 - 01.01.2015
    unsigned int    m_mark;         // 0-5

    visit();
    visit(visit&);
    void move(visit&);

    bool deserializate(const char*, bool update = false);

    sptr_str serializate();
    void serializate(std::string&);
};

#endif
