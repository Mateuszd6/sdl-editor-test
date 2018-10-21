#ifndef UNDO_BUFFER_HPP_INCLUDED
#define UNDO_BUFFER_HPP_INCLUDED

#include "../debug_goodies.h"
#include "../config.h"

#include "./misc/length_bufer.hpp"

// NOTE: Layout in the undo buffer:
//       ||FIXED        |2B            |8B       |8B      |8B       ||8B            ||
//       ||raw_char_data|operation_mask|curr_line|curr_idx|data_size||next_data_size|| ...
//                       ^ Here last_operation points.

static constexpr char word_separators[] = " ./\\()\"'-:,.;<>~!@#$%^&*|+=[]{}`~?";
static constexpr int64 word_separators_len = lengthof(word_separators);

// TODO: Please, not in the header ;(
static constexpr bool is_word_separator(uint8 ch)
{
    for(auto i = 0; i < word_separators_len; ++i)
        if(word_separators[i] == ch)
            return true;

    return false;
}

static const uint64 UNDO_BUFFER_MAX_SIZE = 4000000; // in bytes
static const uint64 UNDO_BUFFER_SIZE = 64 * 1000;

enum operation_enum
{
    NONE = 0,

    INSERT_CHARACTERS = 1,
    REMOVE_CHARACTERS = 2,
};

struct operation_info
{
    uint64 curr_line;
    uint64 curr_idx;
    uint64 data_size;
    uint8* data_ptr;
    operation_enum operation;
};

struct undo_buffer
{
    uint8* buffer;
    uint32 buffer_size;
    uint32 buffer_capacity;

    operation_info* operations;

    int32 operation_capacity;
    int32 operation_size;
    int32 operation_index;
    operation_enum current_operation;

    operation_info const* undo();
    operation_info const* redo();

    void add_undo_info(uint64 curr_line,
                       uint64 curr_idx,
                       uint64 data_size,
                       operation_enum operation,
                       misc::length_buffer text_buffer_weak_ptr);

    // When inserting a sequence of characters we keep the info about it in the
    // current_operaiton field. When e.g. caret is moved, we must indicate that
    // the action is stopped.
    void stop_current_operation();

    void DEBUG_print_state() const;
};


static void undo_buffer_initialize(undo_buffer* ub);

#endif // __UNDO_BUFFER_HPP_INCLUDED__
