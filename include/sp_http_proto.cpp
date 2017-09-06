#include <algorithm>

#include "sp_http_proto.h"
#include "stream.h"
#include "util.h"
#include "log.h"

sp_http_proto::sp_http_proto() {}

method_t sp_http_proto::method() const
{
	return m_method;
}

const std::string& sp_http_proto::path() const
{
	return m_path;
}

bool sp_http_proto::is_get(const std::string& key) const
{
	auto it = m_get.find(sptr_cstr(key));
    return it != m_get.end();
}

method_t sp_http_proto::method_from_str(const std::string& method)
{
	if (method == "GET") return GET;
	else if (method == "POST") return POST;
	else return UNKNOWN;
}

bool sp_http_proto::parse(data_t& m_raw, size_t raw_size)
{
    state_parse_t m_state(METHOD);
    size_t m_current(0);
    size_t m_cvalue(0);
    bool m_flag_2n(false);

    m_get.clear();
    m_path.clear();

    m_raw[raw_size] = 0;
    m_data = reinterpret_cast<char*>(&m_raw[raw_size]);

	for (size_t count = m_current; count < raw_size; ++count) {

		char b = static_cast<char>(m_raw[count]);

		switch (m_state) {
			case METHOD: {
				if (b == ' ') {
					if (count - m_current > 0) {
						m_method = method_from_str(v_to_str(m_raw[m_current], count - m_current));
						m_state = PATH;
						m_current = count + 1;
					} else return false;
				}
				break;
			}
			case PATH: {
				if (b == ' ' || b == '?') {
					if (count - m_current > 0) {
						m_path = v_to_str(m_raw[m_current], count - m_current);

						if (b == '?') m_state = GETOPTIONS;
						else m_state = VERSION;

						m_current = count + 1;
					} else return false;
				}
				break;
			}
			case GETOPTIONS: {
				if (b == ' ') {
					if (count - m_current > 0) {
						parse_urlencoded(m_raw, m_current, count, [this](const sptr_cstr& name, const std::shared_ptr<data_t>& data){add_get(name, data);});
                        if (m_method == GET) return true;
					}
					m_current = count + 1;
				}
			}
			case VERSION: {
				if (b == '\n' || b == '\r') {
					if (count - m_current > 0) {
						m_state = HEADER;
						m_current = count + 1;
					} else return false;
				}
				break;
			}
			case STATUS: {
				if (b == '\n') {

					m_state = HEADER;
					m_current = count + 1;

				}
				break;
			}
			case HEADER: {
				if (b == '\n' || b == '\r') {

					if (b == '\n' && m_flag_2n) {

						if (m_method == GET) return true;

						m_state = BODY;
						m_current = count + 1;

						continue;
					}
					if (b == '\n') m_flag_2n = true;

					m_current = count + 1;

				} else if (b == ':' && m_cvalue < m_current) {
					m_cvalue = count + 2;
				} else {
					m_flag_2n = false;
					continue;
				}
				break;
			}
			case BODY: {
				if (b == '\n' || b == '\r') {
					m_current = count + 1;
					continue;
				}

				if (m_method == POST) {
                    m_data = reinterpret_cast<char*>(&m_raw[m_current]);
                    m_data[raw_size - m_current] = 0;
                }

				return true;
			}
		}
	}
	return true;
}

void sp_http_proto::parse_urlencoded(data_t& m_raw, size_t off_begin, size_t off_end, std::function<void(const sptr_cstr&, const std::shared_ptr<data_t>&)> func, char separator, bool unquote_flag)
{
    size_t name_start = off_begin;
    size_t name_size = 0;
    size_t value_start = 0;
    size_t value_size = 0;

    for (size_t count = off_begin; count < off_end; ++count) {

		if (m_raw[count] == ' ') {

			name_start = count + 1;

		} else if (m_raw[count] == '=' && value_start == 0) {

            name_size = count - name_start;
            value_start = count + 1;

        } else if (value_start != 0) {

            if (m_raw[count] == separator) {

                value_size = count - value_start;

            } else if (count + 1 == off_end) {

                value_size = count - value_start + 1;

            } else continue;

			if (unquote_flag) {
                auto name = unquote(&m_raw[name_start], name_size);
				func(sptr_cstr(new v_to_str(name->at(0), name->size())), unquote(&m_raw[value_start], value_size));
			} else {
                auto value = std::make_shared<data_t>();
                value->resize(value_size);
                for (size_t count = 0; count < value_size; ++count) (*value)[count] = m_raw[value_start + count];
				func(sptr_cstr(new v_to_str(m_raw[name_start], name_size)), value);
			}

            name_start = count + 1;
            value_start = 0;
        }
    }
}

void sp_http_proto::add_get(const sptr_cstr& key, const sptr_cstr& val)
{
	m_get.insert(std::make_pair(key, val));
}

void sp_http_proto::add_get(const sptr_cstr& key, const std::shared_ptr<data_t>& val)
{
    sptr_cstr temp(new std::string(val->begin(), val->end()));
    add_get(key, temp);
}

