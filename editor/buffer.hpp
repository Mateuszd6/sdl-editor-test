#ifndef EDITOR_BUFFER_HPP_INCLUDED
#define EDITOR_BUFFER_HPP_INCLUDED

#include "../text/gap_buffer.hpp"
#include "../text/undo_buffer.hpp"

#define NUMBER_OF_LINES_IN_BUFFER (256)

namespace editor
{
    struct buffer
    {
        gap_buffer* lines;
        size_t capacity;
        size_t gap_start;
        size_t gap_end;
        undo_buffer undo;

        void initialize();

        void move_gap_to_point(size_t point);
        void move_gap_to_buffer_end();

        bool insert_character(size_t line, size_t point, uint8 character);
        bool insert_newline(size_t line);
        bool insert_newline_correct(size_t line, size_t point);

        bool delete_line(size_t line);

        size_t size() const;
        size_t gap_size() const;
        gap_buffer* get_line(size_t line) const;

        void DEBUG_print_state() const;
    };

    struct buffer_point
    {
        buffer* buffer_ptr;
        uint64 first_line;
        uint64 curr_line;
        uint64 curr_idx;

        /// If the line is switched to the much shorter one, and the current
        /// index is truncated to the end, ths value holds the previous index
        /// until the cursor is moved left / right, something is inserted etc.
        /// After moving to the larger one, this index is tried to be restored.
        /// This value is ignored when it is equal to -1.
        int64 last_line_idx;
        bool starting_from_top;


        bool insert_character_at_point(uint8 character);
        bool insert_newline_at_point();

        bool remove_character_backward();
        bool remove_character_forward();

        bool character_right();
        bool character_left();
        bool line_up();
        bool line_down();
        bool jump_up(uint64 number_of_lines);
        bool jump_down(uint64 number_of_lines);
        bool line_start();
        bool line_end();
        bool buffer_start();
        bool buffer_end();

        bool point_is_valid();
    };

    static buffer_point create_buffer_point(buffer* buffer_ptr);
}

#endif // EDITOR_BUFFER_HPP_INCLUDED
