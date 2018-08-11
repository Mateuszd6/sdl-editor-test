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
        size_t gap_start;
        size_t gap_end;

        void initailize();

        void set_current_line(int64 index);

        void move_gap_to_point(size_t point);
        void move_gap_to_buffer_end();

        bool insert_character(size_t line, size_t point, uint8 character);
        bool insert_newline(size_t line);

        bool delete_line(size_t line);

        size_t size() const;
        size_t gap_size() const;
        gap_buffer* get_line(size_t line);

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

    struct buffer_point
    {
        detail::buffer_chunk* buffer_ptr;
    };

#if 0
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
        bool insert_character
        (uint8 character);
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
#endif

    static buffer_point create_buffer_point(buffer* buffer_ptr);
}

#endif // __EDITOR_BUFFER_HPP_INCLUDED__
