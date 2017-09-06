#ifndef HTTP_PROTO_H
#define HTTP_PROTO_H

#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <memory>

#include "format.h"
#include "types.h"

enum method_t {
	OPTIONS,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	TRACE,
	CONNECT,
	PATCH,
	UNKNOWN
};

enum status_t {
	S200 = 200,
	S204 = 204,
	S302 = 302,
	S400 = 400,
	S403 = 403,
	S404 = 404,
	S405 = 405,
	S500 = 500,
	S501 = 501,
	SUNKNOWN = 1000
};

enum state_parse_t {
	METHOD,
	PATH,
	GETOPTIONS,
	VERSION,
	STATUS,
	HEADER,
	BODY
};

#define v_to_str(REF, SIZE) std::string(reinterpret_cast<char*>(&REF), SIZE)

typedef std::vector<unsigned char> data_t;
typedef std::pair<data_t::const_iterator, data_t::const_iterator> value_t;

class post_field
{
private:
	sptr_cstr							m_name;
	sptr_cstr							m_content_type;
	sptr_cstr							m_file_name;
	size_t								m_size;
	value_t								m_value;
    std::shared_ptr<data_t>				m_value_hidden;

public:
	post_field();
	post_field(const sptr_cstr&, const sptr_cstr&, const sptr_cstr&, const std::shared_ptr<data_t>&);
	post_field(const sptr_cstr&, const sptr_cstr&, const sptr_cstr&, value_t);

	sptr_cstr name() const;
	sptr_cstr ct() const;
	sptr_cstr file() const;
	size_t size() const;

	template <typename T>
	T val() const {
		if (m_value_hidden->size() > 0)
			return format<T>(sptr_str(new v_to_str(m_value_hidden->at(0), m_value_hidden->size())));

		sptr_cstr temp(new std::string(m_value.first, m_value.second));
		return format<T>(temp);
	}

	value_t val_pr() const;
};

struct ver_t
{
    unsigned char byte1;
    unsigned char byte2;

    ver_t();
    ver_t(const std::string&);
    bool operator<(ver_t);

    std::string to_str() const;
};

typedef std::shared_ptr<post_field> sptr_post;
typedef std::unordered_map<sptr_cstr, sptr_post, std::hash<sptr_cstr>, equal_sptr_cstr> post_map_t;

class http_proto
{
private:
	method_t		m_method;
	sptr_cstr		m_path;
	ver_t			m_ver;
	status_t		m_status;

	std::unordered_map<sptr_cstr, sptr_cstr>	m_header;
	std::unordered_map<sptr_cstr, sptr_cstr>	m_get;
	std::unordered_map<sptr_cstr, sptr_cstr>	m_cookie;

	post_map_t		m_post;
	size_t			m_content_length;

    std::shared_ptr<data_t> m_raw;
    std::shared_ptr<data_t> m_raw_backup;

	// Используется только в ответах. Заполняется через data()
	sptr_str		m_data;

	// Используются только для парсинга
	state_parse_t	m_state;
	size_t			m_current;
	size_t			m_cvalue;
	bool			m_flag_2n;

public:
	http_proto();
	bool parse(bool req = true);
	sptr_cstr to_raw(bool req = true, bool include_data = true);

	// Получение данных

	method_t method() const;
	sptr_cstr path() const;
	ver_t ver() const;
	status_t status() const;
	size_t content_length() const;

	// заполнение производится через эти методы
	std::shared_ptr<data_t> raw();
	sptr_str data() const;
    void set_data(sptr_str& data);
	void set_raw(std::shared_ptr<data_t>);

    void print_raw() const;

	template <typename T>
	T header(const std::string& key, const T& val) const
	{
		auto it = m_header.find(key);
		if (it == m_header.end()) return val;
		return format<T>(it->second);
	}

	template <typename T>
	T get(const std::string& key, const T& val) const
	{
		auto it = m_get.find(key);
		if (it == m_get.end()) return val;
		return format<T>(it->second);
	}

    bool is_get(const std::string& key) const;

	sptr_post post(const std::string& key) const;

	template <typename T>
	T cookie(const std::string& key, const T& val) const
	{
		auto it = m_cookie.find(key);
		if (it == m_cookie.end()) return val;
		return format<T>(it->second);
	}

	// Заполнение данных

	void set_method(method_t);
	void set_path(const std::string&);
	void set_ver(ver_t);
	void set_status(status_t);

	void add_header(const sptr_cstr&, const sptr_cstr&);
	void add_get(const sptr_cstr&, const sptr_cstr&);
	void add_get(const sptr_cstr&, const std::shared_ptr<data_t>&);
	void add_post(const sptr_cstr&, const std::shared_ptr<data_t>&);
	void add_post(const sptr_cstr&, const sptr_cstr&, const std::shared_ptr<data_t>&);
	void add_cookie(const sptr_cstr&, const sptr_cstr&);
	void add_cookie(const sptr_cstr&, const std::shared_ptr<data_t>&);

private:
	void parse_body();
	void parse_multipart(const std::string& boundary);
	void parse_urlencoded(size_t, size_t, std::function<void(const sptr_cstr&, const std::shared_ptr<data_t>&)>, char separator = '&', bool unquote_flag = true);

	size_t minBSize(size_t);

public:
	static sptr_cstr method_to_str(method_t);
	static method_t method_from_str(const std::string&);
	static sptr_cstr status_to_str(status_t);
	static status_t status_from_int(size_t);
};

unsigned char recode_char(char ch);
std::shared_ptr<data_t> unquote(unsigned char* data_ptr, size_t size);
sptr_cstr quote(const std::string& str);
sptr_cstr expires();

namespace std {
    template<>
    class hash<method_t>
    {
    public:
        size_t operator()(method_t) const;
    };

    template<>
    class hash<status_t>
    {
    public:
        size_t operator()(status_t) const;
    };
}


#endif
