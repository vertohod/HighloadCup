#ifndef REQUEST_H
#define REQUEST_H

#include <vector>
#include <string>

#include "sp_http_proto.h"

class request : protected sp_http_proto
{
public:
	request();

	bool parse(data_t& raw, size_t data_size);

    method_t method() const;
    sptr_cstr path() const;

    inline const char* data() const
    {
        return sp_http_proto::data();
    }

    template <typename T>
    T get(const std::string& key, const T& val) const
	{
		return sp_http_proto::get<T>(key, val);
	}

    bool is_get(const std::string& key) const;
};

#endif
