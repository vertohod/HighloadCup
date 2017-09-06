#ifndef SP_HTTP_PROTO_H
#define SP_HTTP_PROTO_H

#include "http_proto.h"

class sp_http_proto
{
private:
	std::unordered_map<sptr_cstr, sptr_cstr> m_get;

	method_t        m_method;
    std::string     m_path;
    char*           m_data;

public:
	sp_http_proto();
	bool parse(data_t& raw, size_t data_size);

	// Получение данных
	method_t method() const;
    const std::string& path() const;

	inline const char* data() const
    {
        return m_data;
    }

	template <typename T>
	T get(const std::string& key, const T& val) const
	{
		auto it = m_get.find(key);
		if (it == m_get.end()) return val;
		return format<T>(it->second);
	}
    bool is_get(const std::string& key) const;

	void add_get(const sptr_cstr&, const sptr_cstr&);
	void add_get(const sptr_cstr&, const std::shared_ptr<data_t>&);

private:
	void parse_urlencoded(data_t& m_raw, size_t, size_t, std::function<void(const sptr_cstr&, const std::shared_ptr<data_t>&)>, char separator = '&', bool unquote_flag = true);

public:
	static method_t method_from_str(const std::string&);
};

#endif
