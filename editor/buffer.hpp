#ifndef __EDITOR_BUFFER_HPP_INCLUDED__
#define __EDITOR_BUFFER_HPP_INCLUDED__

#include "../text/gap_buffer.hpp"
#define NUMBER_OF_LINES_IN_BUFFER (256)

namespace editor
{
    namespace detail
    {
        struct buffer_chunk
        {

        };
    }

    struct buffer
    {
        uint64 curr_line;
        uint64 curr_index;
        gap_buffer lines[NUMBER_OF_LINES_IN_BUFFER];

        // Should be called after every operation on the line.
        void update_pos_in_line();

        // Line editing API.
        bool insert_character(uint8 character);
        bool move_forward_curr_line();
        bool move_backward_curr_line();
        bool delete_char_at_cursor_backward();
        bool delete_char_at_cursor_forward();
        bool move_line_up();
        bool move_line_down();
        bool jump_start_line();
        bool jump_end_line();
    };
}

#endif // __EDITOR_BUFFER_HPP_INCLUDED__
