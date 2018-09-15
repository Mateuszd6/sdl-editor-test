#ifndef __MISC_LENGTH_BUFFER_HPP_INCLUDED__
#define __MISC_LENGTH_BUFFER_HPP_INCLUDED__

#include "../debug_goodies.h"
#include "../config.h"

namespace misc
{
    struct length_buffer
    {
        uint8 const* data;
        uint64 length;

        length_buffer() {}

        length_buffer(const char* dat, uint64 len)
        {
            data = reinterpret_cast<const uint8*>(dat);
            length = len;
        }
    };
}

#endif // __MISC_LENGTH_BUFFER_HPP_INCLUDED__
