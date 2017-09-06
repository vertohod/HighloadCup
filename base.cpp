#include <stdio.h>
#include <string>

#include "types.h"
#include "util.h"
#include "base.h"
#include "log.h"

const std::string f_id("id");
const std::string f_email("email");
const std::string f_first_name("first_name");
const std::string f_last_name("last_name");
const std::string f_gender("gender");
const std::string f_birth_date("birth_date");

const std::string f_place("place");
const std::string f_country("country");
const std::string f_city("city");
const std::string f_distance("distance");

const std::string f_location("location");
const std::string f_user("user");
const std::string f_visited_at("visited_at");
const std::string f_mark("mark");

base::base() : m_stop_flag(false),
    m_data_visit_flag(false),
    m_data_user_flag(false),
    m_data_location_flag(false)
{
    m_users.set_empty_key(0);
    m_visits.set_empty_key(0);
    m_locations.set_empty_key(0);

    m_user_visit.set_empty_key(0);
    m_location_visit.set_empty_key(0);

    initialize_ages();

    m_thread_ptr = std::make_shared<std::thread>(std::bind(&base::thread_function, this)); 
}

base::~base()                                                                                                                                                                                                                       
{                                                                                                                                                                                                                                             
    {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);                                                                                                                                                                                       
        m_stop_flag = true;                                                                                                                                                                                                                   
    }
    m_cv.notify_all();                                                                                                                                                                                                                    
    m_thread_ptr->join();                                                                                                                                                                                            
} 

void base::thread_function()                                                                                                                                                                                                             
{                                                                                                                                                                                                                                             
    while (!m_stop_flag) {                                                                                                                                                                                                                    
        {                                                                                                                                                                                                                                     
            std::unique_lock<std::mutex> lock(m_tasks_mutex);                                                                                                                                                                                       
            m_cv.wait(lock, [this]{ return m_data_user_flag || m_data_visit_flag || m_data_location_flag || m_stop_flag; });                                                                                                                                                                    
        }                                                                                                                                                                                                                                     

        if (m_data_visit_flag) {
            std::queue<task> visits_temp;
            {
                std::lock_guard<std::mutex> lock(m_tasks_mutex);
                visits_temp.swap(m_tasks);
                m_data_visit_flag = false;
            }
            while (visits_temp.size() > 0) {
                auto& ts = visits_temp.front();
                if (ts.m_update) {

                    if (ts.m_old_ptr->m_user != ts.m_new_ptr->m_user) {
                        auto temp_it = m_user_visit.find(ts.m_old_ptr->m_user);
                        if (temp_it != m_user_visit.end()) {
                            temp_it->second.erase(ts.m_old_ptr->m_id);
                            if (temp_it->second.size() == 0) {
                                m_user_visit.erase(temp_it);
                            }
                        }
                        m_user_visit[ts.m_new_ptr->m_user].insert(ts.m_old_ptr->m_id);
                    }

                    if (ts.m_old_ptr->m_location != ts.m_new_ptr->m_location) {
                        auto temp_it = m_location_visit.find(ts.m_old_ptr->m_location);
                        if (temp_it != m_location_visit.end()) {
                            temp_it->second.erase(ts.m_old_ptr->m_id);
                            if (temp_it->second.size() == 0) {
                                m_location_visit.erase(temp_it);
                            }
                        }
                        m_location_visit[ts.m_new_ptr->m_location].insert(ts.m_old_ptr->m_id);
                    }

                    ts.m_old_ptr->move(*ts.m_new_ptr);

                } else {
                    m_user_visit[ts.m_new_ptr->m_user].insert(ts.m_new_ptr->m_id);
                    m_location_visit[ts.m_new_ptr->m_location].insert(ts.m_new_ptr->m_id);
                }
                visits_temp.pop();
            }
        }

        if (m_data_user_flag) {
            std::queue<std::shared_ptr<user>> users_temp;
            {
                std::lock_guard<std::mutex> lock(m_tasks_user_mutex);
                users_temp.swap(m_tasks_user);
                m_data_user_flag = false;
            }
            while (users_temp.size() > 0) {
                auto user_ptr = users_temp.front();
                user_ptr->serializate(true);
                users_temp.pop();
            }
        }

        if (m_data_location_flag) {
            std::queue<std::shared_ptr<location>> locations_temp;
            {
                std::lock_guard<std::mutex> lock(m_tasks_location_mutex);
                locations_temp.swap(m_tasks_location);
                m_data_location_flag = false;
            }
            while (locations_temp.size() > 0) {
                auto location_ptr = locations_temp.front();
                location_ptr->serializate(true);
                locations_temp.pop();
            }
        }
    }                                                                                                                                                                                                                                         
}  

sptr_str base::get_object(const std::string& name, uint32_t id)
{
    if (name == "users") {

        auto it = m_users.find(id);
        if (it != m_users.end()) {
            return it->second->serializate();
        }

    } else if (name == "visits") {

        auto it = m_visits.find(id);
        if (it != m_visits.end()) {
            return it->second->serializate();
        }

    } else if (name == "locations") {

        auto it = m_locations.find(id);
        if (it != m_locations.end()) {
            return it->second->serializate();
        }
    }

    return sptr_str();
}

sptr_str base::get_visits(uint32_t id, bool is_fromDate, long fromDate, bool is_toDate, long toDate, bool is_country, const std::string& country, bool is_toDistance, uint32_t toDistance)
{
    auto user_it = m_users.find(id);
    if (user_it == m_users.end()) return sptr_str();

    sptr_str res;
    res->reserve(200);
    res->append("{\"visits\":[");

    auto list_it = m_user_visit.find(id);
    if (list_it == m_user_visit.end()) {
        res->append("]}");
        return res;
    }

    std::string object;
    object.reserve(200);

    std::map<long, sptr_cstr> ordered_buffer;

    for (auto visit_id : list_it->second) {

        auto visit_it = m_visits.find(visit_id);
        if (visit_it == m_visits.end()) continue;

        auto location_it = m_locations.find(visit_it->second->m_location);
        if (location_it == m_locations.end()) continue;

        auto& visit_temp = *visit_it->second;
        auto& location_temp = *location_it->second;

        if (is_fromDate && fromDate >= visit_temp.m_visited_at) continue;
        if (is_toDate && toDate <= visit_temp.m_visited_at) continue;
        if (is_toDistance && toDistance <= location_temp.m_distance) continue;
        if (is_country && country != *(location_temp.m_country)) continue;

        object.clear();
        object += "{\"" + f_mark + "\":" + std::to_string(visit_temp.m_mark) + ",";
        object += "\"" + f_visited_at + "\":" + std::to_string(visit_temp.m_visited_at) + ",";
        object += "\"" + f_place + "\":\"" + *location_temp.m_place + "\"}";

        ordered_buffer.insert(std::make_pair(visit_temp.m_visited_at, sptr_cstr(new std::string(object))));
    }

    bool first = true;
    for (auto& pr : ordered_buffer) {
        if (!first) res->append(","); else first = false;
        res->append(*pr.second);
    }
    res->append("]}");

    return res;
}

const std::string base::get_mark(uint32_t id, bool is_fromDate, long fromDate, bool is_toDate, long toDate, bool is_fromAge, long fromAge, bool is_toAge, long toAge, bool is_gender, char gender)
{
    auto location_it = m_locations.find(id);
    if (location_it == m_locations.end()) return std::string();  

    std::string res("{\"avg\": ");

    auto list_it = m_location_visit.find(id);
    if (list_it == m_location_visit.end()) {
        res.append("0}");
        return res;
    }

    auto fromAge_timestamp = get_timestamp_fromAge(fromAge);
    auto toAge_timestamp = get_timestamp_toAge(toAge);

    size_t counter = 0;
    double mark = 0.0;

    auto& visits_list = list_it->second;
    for (auto visit_id : visits_list) {

        auto visit_it = m_visits.find(visit_id);
        if (visit_it == m_visits.end()) continue;

        auto user_it = m_users.find(visit_it->second->m_user);
        if (user_it == m_users.end()) continue;

        auto& visit_temp = *visit_it->second;
        auto& user_temp = *user_it->second;

        if (is_fromDate && fromDate >= visit_temp.m_visited_at) continue;
        if (is_toDate && toDate <= visit_temp.m_visited_at) continue;
        if (is_fromAge && fromAge_timestamp <= user_temp.m_birth_date) continue;
        if (is_toAge && toAge_timestamp >= user_temp.m_birth_date) continue;
        if (is_gender && gender != user_temp.m_gender) continue;

        ++counter;
        mark += visit_temp.m_mark;
    }

    if (counter > 0) {
        sprintf(m_buffer_for_avg, "%.5f}", mark / counter + 1E-10);
        res.append(m_buffer_for_avg);
    } else res.append("0}");

    return res;
}

bool base::insert(const std::string& name, const char* json)
{
    if (name == "users") {

        auto user_ptr = std::make_shared<user>();
        if (!user_ptr->deserializate(json, false)) return false;
        // lock
        {
            std::lock_guard<std::mutex> lock(m_user_mutex);
            auto pr = m_users.insert(std::make_pair(user_ptr->m_id, user_ptr));
            if (!pr.second) return false;
        }
        // lock
        {
            std::lock_guard<std::mutex> lock(m_tasks_user_mutex);
            m_tasks_user.push(user_ptr);
            m_data_user_flag = true;
            m_cv.notify_one();
        }
        return true;

    } else if (name == "visits") {

        auto visit_ptr = std::make_shared<visit>();

        if (!visit_ptr->deserializate(json, false)) return false;
        
        // lock
        std::unique_lock<std::mutex> lock(m_visit_mutex);
        auto pr = m_visits.insert(std::make_pair(visit_ptr->m_id, visit_ptr));
        lock.unlock();

        if (pr.second) {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            m_tasks.push(task(visit_ptr, std::make_shared<visit>(), false));
            m_data_visit_flag = true;
            m_cv.notify_one();
        }

        return pr.second;

    } else if (name == "locations") {

        auto location_ptr = std::make_shared<location>();

        if (!location_ptr->deserializate(json, false)) return false;

        // lock
        {
            std::lock_guard<std::mutex> lock(m_location_mutex);
            auto pr = m_locations.insert(std::make_pair(location_ptr->m_id, location_ptr));
            if (!pr.second) return false;
        }
        // lock
        {
            std::lock_guard<std::mutex> lock(m_tasks_location_mutex);
            m_tasks_location.push(location_ptr);
            m_data_location_flag = true;
            m_cv.notify_one();
        }
        return true;
    }
    return false;
}

void base::print_size()
{
    lo::l(lo::INFO) << "Users: " << m_users.size();
    lo::l(lo::INFO) << "Visits: " << m_visits.size();
    lo::l(lo::INFO) << "Locations: " << m_locations.size();
}

size_t base::users_size()
{
    return m_users.size();
}

size_t base::visits_size()
{
    return m_visits.size();
}

size_t base::locations_size()
{
    return m_locations.size();
}

int base::update(uint32_t id, const std::string& name, const char* json)
{
    if (name == "users") {

        // lock ----
        std::unique_lock<std::mutex> lock(m_user_mutex);
        auto it = m_users.find(id);

        if (it == m_users.end()) return 404;

        auto user_ptr = it->second;
        lock.unlock();
        // ---------

        auto user_temp_ptr = std::make_shared<user>(*user_ptr);
        if (!user_temp_ptr->deserializate(json, true)) return 400;
        user_ptr->move(*user_temp_ptr);

        // lock
        {
            std::lock_guard<std::mutex> lock(m_tasks_user_mutex);
            m_tasks_user.push(user_ptr);
            m_data_user_flag = true;
            m_cv.notify_one();
        }

        return 200;

    } else if (name == "visits") {

        std::shared_ptr<visit> visit_ptr;

        // lock ----
        {
            std::lock_guard<std::mutex> lock(m_visit_mutex);
            auto it = m_visits.find(id);

            if (it == m_visits.end()) return 404;

            visit_ptr = it->second;
        }

        auto visit_temp_ptr = std::make_shared<visit>(*visit_ptr);
        if (!visit_temp_ptr->deserializate(json, true)) return 400;

        // lock
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            m_tasks.push(task(visit_temp_ptr, visit_ptr, true));
            m_data_visit_flag = true;
            m_cv.notify_one();
        }

        return 200;

    } else if (name == "locations") {

        // lock ----
        std::unique_lock<std::mutex> lock(m_location_mutex);
        auto it = m_locations.find(id);

        if (it == m_locations.end()) return 404;

        auto location_ptr = it->second;
        lock.unlock();
        // ---------

        auto location_temp_ptr = std::make_shared<location>(*location_ptr);

        if (!location_temp_ptr->deserializate(json, true)) return 400;

        location_ptr->move(*location_temp_ptr);

        // lock
        {
            std::lock_guard<std::mutex> lock(m_tasks_location_mutex);
            m_tasks_location.push(location_ptr);
            m_data_location_flag = true;
            m_cv.notify_one();
        }

        return 200;

    }

    return 400;
}

user::user() {}

user::user(user& obj)
{
    m_id = obj.m_id;
    m_email = std::make_unique<std::string>(*obj.m_email);
    m_first_name = std::make_unique<std::string>(*obj.m_first_name);
    m_last_name = std::make_unique<std::string>(*obj.m_last_name);
    m_gender = obj.m_gender;
    m_birth_date = obj.m_birth_date;
}

void user::move(user& obj)
{
    m_id = obj.m_id;
    m_email = std::move(obj.m_email);
    m_first_name = std::move(obj.m_first_name);
    m_last_name = std::move(obj.m_last_name);
    m_gender = std::move(obj.m_gender);
    m_birth_date = obj.m_birth_date;

    m_serialization->clear();
}

bool user::deserializate(const char* json, bool update)
{
    rapidjson::Document document;
    document.SetObject();

    try {
        document.Parse(json);
    } catch (const std::exception& e) {
        return false;
    }

    if (update) {

        if (document.HasMember(f_email.c_str())) {
            if (!document[f_email.c_str()].IsString() || document[f_email.c_str()].IsNull()) return false; 
            auto temp = std::make_unique<std::string>();
            *temp = document[f_email.c_str()].GetString();
            if (100 <= util::length_utf8(*temp)) return false; 
            m_email = std::move(temp);
        }

        if (document.HasMember(f_first_name.c_str())) {
            if (!document[f_first_name.c_str()].IsString() || document[f_first_name.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_first_name.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_first_name = std::move(temp);
        }

        if (document.HasMember(f_last_name.c_str())) {
            if (!document[f_last_name.c_str()].IsString() || document[f_last_name.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_last_name.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_last_name = std::move(temp);
        }

        if (document.HasMember(f_gender.c_str())) {
            if (!document[f_gender.c_str()].IsString() || document[f_gender.c_str()].IsNull()) return false;
            std::string temp = document[f_gender.c_str()].GetString();
            if (temp != "m" && temp != "f") return false;
            m_gender = temp[0];
        }

        if (document.HasMember(f_birth_date.c_str())) {
            if (!document[f_birth_date.c_str()].IsInt() || document[f_birth_date.c_str()].IsNull()) return false;
            m_birth_date = document[f_birth_date.c_str()].GetInt();
        }

        m_serialization->clear();

    } else {

        if (document.HasMember(f_id.c_str())) {
            if (!document[f_id.c_str()].IsUint() || document[f_id.c_str()].IsNull()) return false;
            m_id = document[f_id.c_str()].GetUint();    
        } else return false;

        if (document.HasMember(f_email.c_str())) {
            if (!document[f_email.c_str()].IsString() || document[f_email.c_str()].IsNull()) return false; 
            auto temp = std::make_unique<std::string>();
            *temp = document[f_email.c_str()].GetString();
            if (100 <= util::length_utf8(*temp)) return false; 
            m_email = std::move(temp);
        } else return false;

        if (document.HasMember(f_first_name.c_str())) {
            if (!document[f_first_name.c_str()].IsString() || document[f_first_name.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_first_name.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_first_name = std::move(temp);
        } else return false;

        if (document.HasMember(f_last_name.c_str())) {
            if (!document[f_last_name.c_str()].IsString() || document[f_last_name.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_last_name.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_last_name = std::move(temp);
        } else return false;

        if (document.HasMember(f_gender.c_str())) {
            if (!document[f_gender.c_str()].IsString() || document[f_gender.c_str()].IsNull()) return false;
            std::string temp = document[f_gender.c_str()].GetString();
            if (temp != "m" && temp != "f") return false;
            m_gender = temp[0];
        } else return false;

        if (document.HasMember(f_birth_date.c_str())) {
            if (!document[f_birth_date.c_str()].IsInt() || document[f_birth_date.c_str()].IsNull()) return false;
            m_birth_date = document[f_birth_date.c_str()].GetInt();
        } else return false;

        m_serialization->clear();
    }

    return true;
}

sptr_str user::serializate(bool force)
{
    if (force || m_serialization->empty()) {
        std::string buffer;
        serializate(buffer);
        m_serialization->assign(buffer);
    }
    return m_serialization;
}

void user::serializate(std::string& buffer)
{
    buffer.reserve(300);

    buffer += "{\"" + f_id + "\":" + std::to_string(m_id) + ",";
    buffer += "\"" + f_email + "\":\"" + *m_email + "\",";
    buffer += "\"" + f_first_name + "\":\"" + *m_first_name + "\",";
    buffer += "\"" + f_last_name + "\":\"" + *m_last_name + "\",";
    buffer += "\"" + f_gender + "\":\"" + m_gender + "\",";
    buffer += "\"" + f_birth_date + "\":" + std::to_string(m_birth_date) + "}";
}

location::location() {}

location::location(location& obj)
{
    m_id = obj.m_id;
    m_place = std::make_unique<std::string>(*obj.m_place);
    m_country = std::make_unique<std::string>(*obj.m_country);
    m_city = std::make_unique<std::string>(*obj.m_city);
    m_distance = obj.m_distance;
}

void location::move(location& obj)
{
    m_id = obj.m_id;
    m_place = std::move(obj.m_place);
    m_country = std::move(obj.m_country);
    m_city = std::move(obj.m_city);
    m_distance = obj.m_distance;

    m_serialization->clear();
}

bool location::deserializate(const char* json, bool update)
{
    rapidjson::Document document;
    document.SetObject();

    try {
        document.Parse(json);
    } catch (const std::exception& e) {
        return false;
    }

    if (update) {

        if (document.HasMember(f_place.c_str())) {
            if (!document[f_place.c_str()].IsString() || document[f_place.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_place.c_str()].GetString();
            m_place = std::move(temp);
        }

        if (document.HasMember(f_country.c_str())) {
            if (!document[f_country.c_str()].IsString() || document[f_country.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_country.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_country = std::move(temp);
        }

        if (document.HasMember(f_city.c_str())) {
            if (!document[f_city.c_str()].IsString() || document[f_city.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_city.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_city = std::move(temp);
        }

        if (document.HasMember(f_distance.c_str())) {
            if (!document[f_distance.c_str()].IsUint() || document[f_distance.c_str()].IsNull()) return false;
            m_distance = document[f_distance.c_str()].GetUint();    
        }

        m_serialization->clear();

    } else {

        if (document.HasMember(f_id.c_str())) {
            if (!document[f_id.c_str()].IsUint() || document[f_id.c_str()].IsNull()) return false;
            m_id = document[f_id.c_str()].GetUint();    
        } else return false;

        if (document.HasMember(f_place.c_str())) {
            if (!document[f_place.c_str()].IsString() || document[f_place.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_place.c_str()].GetString();
            m_place = std::move(temp);
        } else return false;

        if (document.HasMember(f_country.c_str())) {
            if (!document[f_country.c_str()].IsString() || document[f_country.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_country.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_country = std::move(temp);
        } else return false;

        if (document.HasMember(f_city.c_str())) {
            if (!document[f_city.c_str()].IsString() || document[f_city.c_str()].IsNull()) return false;
            auto temp = std::make_unique<std::string>();
            *temp = document[f_city.c_str()].GetString();
            if (50 <= util::length_utf8(*temp)) return false; 
            m_city = std::move(temp);
        } else return false;

        if (document.HasMember(f_distance.c_str())) {
            if (!document[f_distance.c_str()].IsUint() || document[f_distance.c_str()].IsNull()) return false;
            m_distance = document[f_distance.c_str()].GetUint();    
        } else return false;

        m_serialization->clear();
    }

    return true;
}

sptr_str location::serializate(bool force)
{
    if (force || m_serialization->empty()) {
        std::string buffer;
        serializate(buffer);
        m_serialization->assign(buffer);
    }
    return m_serialization;
}

void location::serializate(std::string& buffer)
{
    buffer.reserve(300);

    buffer += "{\"" + f_id + "\":" + std::to_string(m_id) + ",";
    buffer += "\"" + f_place + "\":\"" + *m_place + "\",";
    buffer += "\"" + f_country + "\":\"" + *m_country + "\",";
    buffer += "\"" + f_city + "\":\"" + *m_city + "\",";
    buffer += "\"" + f_distance + "\":" + std::to_string(m_distance) + "}";
}

visit::visit() {}

visit::visit(visit& obj)
{
    m_id = obj.m_id;
    m_location = obj.m_location;
    m_user = obj.m_user;
    m_visited_at = obj.m_visited_at;
    m_mark = obj.m_mark;
}

void visit::move(visit& obj)
{
    m_id = obj.m_id;
    m_location = obj.m_location;
    m_user = obj.m_user;
    m_visited_at = obj.m_visited_at;
    m_mark = obj.m_mark;
}

bool visit::deserializate(const char* json, bool update)
{
    rapidjson::Document document;
    document.SetObject();

    try {
        document.Parse(json);
    } catch (const std::exception& e) {
        return false;
    }

    if (update) {

        if (document.HasMember(f_location.c_str())) {
            if (!document[f_location.c_str()].IsUint() || document[f_location.c_str()].IsNull()) return false;
            m_location = document[f_location.c_str()].GetUint();    
        }

        if (document.HasMember(f_user.c_str())) {
            if (!document[f_user.c_str()].IsUint() || document[f_user.c_str()].IsNull()) return false;
            m_user = document[f_user.c_str()].GetUint();    
        }

        if (document.HasMember(f_visited_at.c_str())) {
            if (!document[f_visited_at.c_str()].IsInt() || document[f_visited_at.c_str()].IsNull()) return false;
            m_visited_at = document[f_visited_at.c_str()].GetInt();    
        }

        if (document.HasMember(f_mark.c_str())) {
            if (!document[f_mark.c_str()].IsUint() || document[f_mark.c_str()].IsNull()) return false;
            m_mark = document[f_mark.c_str()].GetUint();    
        }

    } else {

        if (document.HasMember(f_id.c_str())) {
            if (!document[f_id.c_str()].IsUint() || document[f_id.c_str()].IsNull()) return false;
            m_id = document[f_id.c_str()].GetUint();    
        } else return false;

        if (document.HasMember(f_location.c_str())) {
            if (!document[f_location.c_str()].IsUint() || document[f_location.c_str()].IsNull()) return false;
            m_location = document[f_location.c_str()].GetUint();    
        } else return false;

        if (document.HasMember(f_user.c_str())) {
            if (!document[f_user.c_str()].IsUint() || document[f_user.c_str()].IsNull()) return false;
            m_user = document[f_user.c_str()].GetUint();    
        } else return false;

        if (document.HasMember(f_visited_at.c_str())) {
            if (!document[f_visited_at.c_str()].IsInt() || document[f_visited_at.c_str()].IsNull()) return false;
            m_visited_at = document[f_visited_at.c_str()].GetInt();    
        } else return false;

        if (document.HasMember(f_mark.c_str())) {
            if (!document[f_mark.c_str()].IsUint() || document[f_mark.c_str()].IsNull()) return false;
            m_mark = document[f_mark.c_str()].GetUint();    
        } else return false;

    }

    return true;
}

sptr_str visit::serializate()
{
    sptr_str buffer;
    serializate(*buffer);
    return buffer;
}

void visit::serializate(std::string& buffer)
{
    buffer.reserve(300);

    buffer += "{\"" + f_id + "\":" + std::to_string(m_id) + ",";
    buffer +=  "\"" + f_location + "\":" + std::to_string(m_location) + ",";
    buffer +=  "\"" + f_user + "\":" + std::to_string(m_user) + ",";
    buffer += "\"" + f_visited_at + "\":" + std::to_string(m_visited_at) + ",";
    buffer +=  "\"" + f_mark + "\":" + std::to_string(m_mark) + "}";
}

base::task::task() {}
base::task::task(std::shared_ptr<visit> new_ptr, std::shared_ptr<visit> old_ptr, bool update) : m_new_ptr(new_ptr), m_old_ptr(old_ptr), m_update(update) {}
