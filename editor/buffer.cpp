// TODO(Cleanup): Try to use as less globals as possible...
namespace editor::global
{
    static int number_of_buffers = 0;
    static editor::buffer buffers[256];
}

namespace editor::detail
{
    void buffer_chunk::initailize()
    {
        prev_chunks_size = 0;
        gap_start = 1;
        gap_end = NUMBER_OF_LINES_IN_BUFFER;
        curr_line = 1;

        lines[0].initialize();
    }

    void buffer_chunk::set_current_line(int64 index)
    {
        if (gap_start <= index)
            curr_line = gap_start;
        else
            curr_line = gap_end + (index - gap_start);
    }

    void buffer_chunk::move_gap_to_current_line()
    {
        if (curr_line < gap_start)
        {
            auto diff = gap_start - curr_line;
#if 1
            auto dest_start = gap_end - diff;
            auto source_start = curr_line;
            for (auto i = 0; i < diff; ++i)
                move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);
#else
            memcpy(gap_end - diff, curr_line, sizeof(uint8) * diff);
#endif
            gap_start -= diff;
            gap_end -= diff;
            curr_line = gap_start;
        }
        else if (gap_start < curr_line)
        {
            auto diff = curr_line - gap_end;
#if 1
            auto dest_start = gap_start;
            auto source_start = gap_end;
            for (auto i = 0; i < diff; ++i)
                move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);
#else
            memcpy(gap_start, gap_end, sizeof(uint8) * diff);
#endif
            gap_start += diff;
            gap_end += diff;
            curr_line = gap_start;
        }
        else if (curr_line == gap_start)
        {
            // TODO(Cleanup): Don't do anything?
        }
        else
            UNREACHABLE();
    }

    bool buffer_chunk::insert_newline()
    {
        lines[curr_line].initialize();
        curr_line++;
        gap_start++;

        // TODO: Case when there is not enought memory.
        return true;
    }

    int64 buffer_chunk::get_gap_size() const
    {
        return gap_end - gap_start;
    }

    bool buffer_chunk::line_forward()
    {
        // If we are not at the end of buffer
        if (curr_line < NUMBER_OF_LINES_IN_BUFFER)
        {
            // If we are at the last character before gap.
            if (curr_line == gap_start) // TODO: This +1 might be wrong!
            {
                // If the gap is at the end of allocated memory we cannot move further.
                if (gap_end == NUMBER_OF_LINES_IN_BUFFER)
                    return false;
                // Otherwise we just throught the gap.
                else
                    curr_line = gap_end + 1; // TODO(Cleanup): Is this legal -> gap_end + 1.
            }
            else
                curr_line++;
        }
        else
            return false;

        return true;
    }

    bool buffer_chunk::line_backward()
    {
        if (curr_line != 0)
        {
            if (curr_line == gap_end + 1) // TODO(Cleanup): Is this legal -> gap_end + 1.
                curr_line = gap_start;
            else
                curr_line--;

            return true;
        }
        else
            return false;
    }

    int64 buffer_chunk::get_size() const
    {
        PANIC("Not implemented yet.");
        return 0;
    }

    int64 buffer_chunk::get_line_in_chunk() const
    {
        return NUMBER_OF_LINES_IN_BUFFER - get_gap_size();
    }

    void buffer_chunk::DEBUG_print_state() const
    {
        auto in_gap = false;
        for (int i = 0; i < NUMBER_OF_LINES_IN_BUFFER; ++i)
        {
            if (i == gap_start)
                in_gap = true;

            if (i == gap_end)
                in_gap = false;

            if (!in_gap)
            {
                printf("%3d:  ", i);
                auto line = const_cast<unsigned char*>(lines[i].to_c_str());
                printf("%s", line);
                free(static_cast<void*>(line));
                printf("\n");
            }
            else if (in_gap && i - gap_start < 2)
                printf("%3d:  ???\n", i);
            else if (in_gap && i == gap_end - 1)
                printf("      ...\n%3d:  ???\n", i);
        }
    }
}

namespace editor
{
    // TODO(Docs): Summary!
    static buffer* CreateNewBuffer()
    {
        auto result = global::buffers + global::number_of_buffers;
        global::number_of_buffers++;
        result->initialize();

        return result;
    }

    void buffer::initialize()
    {
        // TODO: Realloc this array if needed.
        chunks = static_cast<detail::buffer_chunk*>(malloc(sizeof(detail::buffer_chunk) * 16));
        chunks_size = 1;
        chunks_capacity = 16;

        chunks[0].initailize();
        last_up_to_date_with_size_chunk = 1;
    }

    void buffer::DEBUG_print_state() const
    {
        for (int i = 0; i < chunks_size; ++i)
        {
            printf("--> chunk %d\n", i);
            chunks[i].DEBUG_print_state();
        }
    }

    static buffer_point create_buffer_point(buffer* buffer_ptr)
    {
        auto result = buffer_point {};
        result.curr_chunk = buffer_ptr->chunks + 0;
        result.line_in_chunk = 0; // Can be smaller integer.
        result.index_in_line = 0;
        result.buffer_ptr = buffer_ptr;

        return result;
    }

    void buffer_point::update_pos_in_line()
    {
        PANIC("Not implemented yet.");
    }

    // Line navigation and editing.
    bool buffer_point::insert_character(uint8 character)
    {
        curr_chunk->lines[line_in_chunk].insert_at_point(character);
        return true; // TODO: For sure?
    }

    bool buffer_point::move_forward_curr_line()
    {
        return curr_chunk->lines[line_in_chunk].cursor_forward();
    }

    bool buffer_point::move_backward_curr_line()
    {
        return curr_chunk->lines[line_in_chunk].cursor_backward();
    }

    bool buffer_point::delete_char_at_cursor_backward()
    {
        PANIC("Not implemented yet.");
        return false;
    }
    bool buffer_point::delete_char_at_cursor_forward()
    {
        PANIC("Not implemented yet.");
        return false;
    }
    bool buffer_point::jump_start_line()
    {
        PANIC("Not implemented yet.");
        return false;
    }
    bool buffer_point::jump_end_line()
    {
        PANIC("Not implemented yet.");
        return false;
    }

    // Buffer navigation.
    bool buffer_point::move_line_up()
    {
        // TODO: Handle the case if it fails.
        if (curr_chunk->line_backward())
        {
            line_in_chunk--;
            return true;
        }
        else
            return false;
    }

    // Buffer navigation.
    bool buffer_point::move_line_down()
    {
        // TODO: Handle the case if it fails.
        if (curr_chunk->line_forward())
        {
            line_in_chunk++;
            return true;
        }
        else
            return false;
    }

    bool buffer_point::insert_newline()
    {
        if(curr_chunk->insert_newline())
        {
            line_in_chunk++;
            return true;
        }
        else
            return false;
    }

#if 0
    void buffer::update_pos_in_line()
    {
        curr_index = lines[curr_line].get_idx();
        LOG_TRACE("Position has beed updated.");
    }

    bool buffer::insert_character(uint8 character)
    {
        auto succeeded = false;
        if (character != '\0')
        {
            lines[curr_line].insert_at_point(character);
            succeeded = true; // Inserting character cannot fail.
            update_pos_in_line();
        }

        return succeeded;
    }

    bool buffer::move_forward_curr_line()
    {
        auto succeeded = lines[curr_line].cursor_forward();
        if (succeeded)
            update_pos_in_line();

        return succeeded;
    }

    bool buffer::move_backward_curr_line()
    {
        auto succeeded = lines[curr_line].cursor_backward();
        if (succeeded)
            update_pos_in_line();

        return succeeded;
    }

    bool buffer::delete_char_at_cursor_backward()
    {
        auto succeeded = lines[curr_line].delete_char_backward();
        if (succeeded)
            update_pos_in_line();

        return succeeded;
    }

    bool buffer::delete_char_at_cursor_forward()
    {
        auto succeeded = lines[curr_line].delete_char_forward();
        return succeeded;
    }

    bool buffer::move_line_up()
    {
        auto succeeded = false;
        if (curr_line > 0)
            curr_line--;
        succeeded = true;

        return succeeded;
    }

    bool buffer::move_line_down()
    {
        auto succeeded = false;
        curr_line++;
        succeeded = true;

        return succeeded;
    }

    bool buffer::jump_start_line()
    {
        auto result = lines[curr_line].jump_start_dumb();
        if (result)
            update_pos_in_line();

        return result;
    }

    bool buffer::jump_end_line()
    {
        auto result = lines[curr_line].jump_end_dumb();
        if (result)
            update_pos_in_line();

        return result;
    }
#endif
}
