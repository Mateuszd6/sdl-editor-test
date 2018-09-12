#ifndef __UNDO_BUFFER_HPP_INCLUDED__
#define __UNDO_BUFFER_HPP_INCLUDED__

#include "../debug_goodies.h"
#include "../config.h"

#include "./misc/length_bufer.hpp"

// NOTE: Layout in the undo buffer:
//       ||FIXED        |2B            |8B       |8B      |8B       ||8B            ||
//       ||raw_char_data|operation_mask|curr_line|curr_idx|data_size||next_data_size|| ...
//                       ^ Here last_operation points.


const uint64 UNDO_BUFFER_SIZE = 4000000; // in bytes

enum operation_enum : uint16
{
    NONE = 0,

    INSERT_CHARACTERS = 1,
    REMOVE_CHARACTERS = 2,

    IS_LAST = (1 << 15) - 1,
    IS_FIRST = 1 << 15,
};

struct undo_buffer
{
    uint8* buffer;

    /// Points to the beginning of the one element
    /// (to the operation_enum byte in the layout decribed above).
    uint8* current_point;

    /// current_point must point to operation_info with operation_mask IS_FIRST
    /// bit on and it has already been undone.
    bool no_more_undo;

    // NOTE: This is a weak reference!
    misc::length_buffer get_data() const;

    operation_enum get_operation_time() const;

    void DEBUG_print_state() const;
};

struct operation_info
{
    operation_enum operation;
    uint64 curr_line;
    uint64 curr_idx;
    uint64 data_size;
    uint64 data_size_next;
};

static void undo_buffer_initialize(undo_buffer* ub);

#endif // __UNDO_BUFFER_HPP_INCLUDED__
