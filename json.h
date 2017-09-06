#ifndef JSON_H
#define JSON_H

#include <stdexcept>

#define RAPIDJSON_ASSERT(x) { if (!(x)) throw std::runtime_error("Error into rapidjson: " #x); } 

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/allocators.h"

#endif
