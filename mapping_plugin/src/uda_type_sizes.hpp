#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <clientserver/udaStructs.h>
#include <clientserver/udaTypes.h>

namespace json_plugin
{
inline size_t size_of_uda_type(int type_enum)
{
    switch (type_enum) {
        case UDA_TYPE_CHAR:
            return sizeof(char);
        case UDA_TYPE_SHORT:
            return sizeof(short);
        case UDA_TYPE_INT:
            return sizeof(int);
        case UDA_TYPE_LONG:
            return sizeof(long);
        case UDA_TYPE_LONG64:
            return sizeof(int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return sizeof(unsigned char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return sizeof(unsigned short);
        case UDA_TYPE_UNSIGNED_INT:
            return sizeof(unsigned int);
        case UDA_TYPE_UNSIGNED_LONG:
            return sizeof(unsigned long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return sizeof(uint64_t);
        case UDA_TYPE_FLOAT:
            return sizeof(float);
        case UDA_TYPE_DOUBLE:
            return sizeof(double);
        case UDA_TYPE_COMPLEX:
            return sizeof(COMPLEX);
        case UDA_TYPE_DCOMPLEX:
            return sizeof(DCOMPLEX);
        default:
            throw std::runtime_error(std::string("uda type ") + std::to_string(type_enum) +
                                     " not implemented for json_imas_mapping cache");
    }
}
} // namespace json_plugin
