#ifndef __UNDO_BUFFER_HPP_INCLUDED__
#define __UNDO_BUFFER_HPP_INCLUDED__

#include "../debug_goodies.h"
#include "../config.h"

// NOTE: Layout in the undo buffer:
//       ||2B            |8B       |8B      |8B       |FIXED        ||
//       ||operation_mask|curr_line|curr_idx|data_size|raw_char_data|| ...

//       ||1b     |1b      |14b      ||
//       ||is_last|is_first|enum_data|| ...

const uint64 UNDO_BUFFER_SIZE = 4000000; // in bytes

enum operation_enum : uint16
{
    NONE = 0,

    INSERT_CHARACTERS = 1,
    REMOVE_CHARACTERS = 2,

    IS_LAST = 1 << 14,
    IS_FIRST = 1 << 14,
};

struct undo_buffer
{
    uint8* buffer;

    /// Points to the beginning of the one element
    /// (to the operation_enum byte in the layout decribed above).
    uint8* last_operation;

    void DEBUG_print_state() const;
};

struct operation_info
{
    uint16 operation_mask;
    uint64 curr_line;
    uint64 curr_idx;
    uint64 data_size;
};

static void undo_buffer_initialize(undo_buffer* ub);

#endif // __UNDO_BUFFER_HPP_INCLUDED__
