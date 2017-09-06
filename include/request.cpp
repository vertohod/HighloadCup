#include <string>

#include "request.h"
#include "types.h"

request::request()
{
}

bool request::parse(data_t& raw, size_t data_size)
{
	return sp_http_proto::parse(raw, data_size);
}

method_t request::method() const
{
	return sp_http_proto::method();
}

sptr_cstr request::path() const
{
	return sp_http_proto::path();
}

bool request::is_get(const std::string& key) const
{
    return sp_http_proto::is_get(key);
}
