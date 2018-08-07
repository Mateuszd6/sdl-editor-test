#ifndef __EDITOR_BUFFER_HPP_INCLUDED__
#define __EDITOR_BUFFER_HPP_INCLUDED__

#include "../text/gap_buffer.hpp"
#define NUMBER_OF_LINES_IN_BUFFER (256)

namespace editor::detail
{
    struct buffer_chunk
    {
        gap_buffer lines[NUMBER_OF_LINES_IN_BUFFER];

        // The size of the chunks places before in the chunk array in the chunks
        // array in buffer object.
        int64 prev_chunks_size;
        int gap_start;
        int gap_end;
        int curr_line;

        void initailize();

        void set_current_line(int64 index);
        void move_gap_to_current_line();

        bool insert_character(uint8 character);
        bool insert_newline();

#if 0
        void move_gap_to_buffer_end();
#endif
        int64 get_gap_size() const;

        // Only these should be used.
        bool line_forward();
        bool line_backward();

        // Can exapnd buffer memory.
#if 0
        void insert_at_point(uint8 character);
#endif

        // TODO(Cleaup): Change to size_t.
        int64 get_size() const;
        int64 get_line_in_chunk() const;

        void DEBUG_print_state() const;
    };
}

namespace editor
{
    struct buffer
    {
        detail::buffer_chunk* chunks;
        int64 chunks_size;
        int64 chunks_capacity;
        int64 last_up_to_date_with_size_chunk;

        void initialize();

        void DEBUG_print_state() const;
    };

    // TODO: Move to another file.
    struct buffer_point
    {
        buffer* buffer_ptr;
        detail::buffer_chunk* curr_chunk;
        uint64 line_in_chunk; // Can be smaller integer.
        uint64 index_in_line;

        // Should be called after every operation on the line.
        void update_pos_in_line();

        // Line navigation and editing.
        bool insert_character(uint8 character);
        bool insert_newline();

        bool move_forward_curr_line();
        bool move_backward_curr_line();
        bool delete_char_at_cursor_backward();
        bool delete_char_at_cursor_forward();
        bool jump_start_line();
        bool jump_end_line();

        // Buffer navigation and editing.
        bool move_line_up();
        bool move_line_down();
    };

    static buffer_point create_buffer_point(buffer* buffer_ptr);
}

#endif // __EDITOR_BUFFER_HPP_INCLUDED__
