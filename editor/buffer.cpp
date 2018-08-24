// TODO(Cleanup): Try to use as less globals as possible...
namespace editor::global
{
    static int number_of_buffers = 0;
    static editor::buffer buffers[256];
}

namespace editor::detail
{
    /// Line 0 is valid.
    void buffer_chunk::initailize()
    {
        prev_chunks_size = 0;
        gap_start = 1;
        gap_end = NUMBER_OF_LINES_IN_BUFFER;

        lines[0].initialize();
    }

    /// After this move point'th line is not valid and must be initialized before use.
    void buffer_chunk::move_gap_to_point(size_t point)
    {
        if (point < gap_start)
        {
            auto diff = gap_start - point;
            auto dest_start = gap_end - diff;
            auto source_start = point;
            for (auto i = 0_u64; i < diff; ++i)
                move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

            gap_start -= diff;
            gap_end -= diff;
        }
        else if (point > gap_start)
        {
            auto diff = point - gap_start;
            auto dest_start = gap_start;
            auto source_start = gap_end;
            for (auto i = 0_u64; i < diff; ++i)
                move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

            gap_start += diff;
            gap_end += diff;
        }
        else if (point == gap_start) { }
        else
            UNREACHABLE();
    }

    void buffer_chunk::move_gap_to_buffer_end()
    {
        if (gap_end == NUMBER_OF_LINES_IN_BUFFER)
            LOG_WARN("Gap already at the end.");
        else
        {
            auto diff = NUMBER_OF_LINES_IN_BUFFER - gap_end;
            auto dest_start = gap_start;
            auto source_start = gap_end;
            for (auto i = 0_u64; i < diff; ++i)
                move_gap_bufffer(&lines[source_start + i], &lines[dest_start + i]);

            gap_start += diff;
            gap_end += diff;
        }
    }

    // TODO: Dont use it. Use: get_line()->insert....
    bool buffer_chunk::insert_character(size_t line, size_t point, uint8 character)
    {
        get_line(line)->insert_at_point(point, character);

        if (get_line(line)->gap_start == nullptr)
            PANIC("Nooooooo");

        return true;
    }

    /// line - number of line after we insert newline.
    bool buffer_chunk::insert_newline(size_t line)
    {
        move_gap_to_point(line + 1);
        gap_start++;
        lines[line + 1].initialize();

        // TODO: Case when there is not enought memory.
        return true;
    }

    bool buffer_chunk::insert_newline_correct(size_t line, size_t point)
    {
        ASSERT(line < size());

        insert_newline(line);
        auto edited_line = get_line(line);
        auto created_line = get_line(line + 1);
        ASSERT(point <= edited_line->size());


        for(auto i = point; i < edited_line->size(); ++i)
        {
            printf("Moving: %c\n", edited_line->get(i));
            created_line->insert_at_point(i - point, edited_line->get(i));
        }

        // TODO: Move gap to the end in the gap_buffer class.
        edited_line->delete_to_the_end_of_line(point);
        return true;
    }

    bool buffer_chunk::delete_line(size_t line)
    {
        ASSERT(line > 0);
        ASSERT(line < size());

        move_gap_to_point(line + 1); // TODO(NEXT): What about it!
        auto prev_line = get_line(line - 1);
        auto removed_line = get_line(line);
        auto prev_line_size = prev_line->size();
        auto removed_line_size = removed_line->size();
        auto prev_line_idx = prev_line_size;

        // TODO: Don't use the constant. Think about optimal value here!
        prev_line->reserve_gap(removed_line_size + 16);
        for(auto i = 0_u64; i < removed_line_size; ++i)
            prev_line->insert_at_point(prev_line_idx++, removed_line->get(i));

        // TODO: Free the memory allocted it the line, just removed.
        gap_start--;
        return true;
    }

    size_t buffer_chunk::size() const
    {
        return NUMBER_OF_LINES_IN_BUFFER - gap_size();
    }

    size_t buffer_chunk::gap_size() const
    {
        return gap_end - gap_start;
    }

    gap_buffer* buffer_chunk::get_line(size_t line)
    {
        ASSERT(line < size());

        if (line < gap_start)
            return &lines[line];
        else
            return &lines[gap_end + (line - gap_start)];
    }

    void buffer_chunk::DEBUG_print_state() const
    {
        auto in_gap = false;
        printf("Gap: %ld-%ld\n", gap_start, gap_end);
        for (auto i = 0_u64; i < NUMBER_OF_LINES_IN_BUFFER; ++i)
        {
            if (i == gap_start)
                in_gap = true;

            if (i == gap_end)
                in_gap = false;

            if (!in_gap)
            {
                printf("%3ld:[%3ld]  ", i, lines[i].size());
                auto line = lines[i].to_c_str();
                printf("%s", line);
                free(static_cast<void*>(line));
                printf("\n");
            }
            else if (in_gap && i - gap_start < 2)
                printf("%3ld:  ???\n", i);
            else if (in_gap && i == gap_end - 1)
                printf("      ...\n%3ld:  ???\n", i);
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
        result.buffer_ptr = &buffer_ptr->chunks[0];
	result.first_line = 0;
	result.curr_line = 0;
	result.curr_idx = 0;
	result.starting_from_top = true;

        return result;
    }

#if 0

    void buffer_point::update_pos_in_line()
    {
        PANIC("Not implemented yet.");
    }

    // Line navigation and editing.
    bool buffer_point::insert_character(size_t line, uint8 character)
    {
        curr_chunk->insert_character(line, character);
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
        auto removed = curr_chunk->lines[line_in_chunk].delete_char_backward();

        if (!removed)
        {
            // Check if caret is at the end of the line...
            if (curr_chunk->lines[line_in_chunk].get_idx() == 0)
            {
                // We are at the end of a buffer.
                if (curr_chunk->curr_line == 0)
                {
                    // TODO: Remove a line from the previous chunk.
                    // PANIC("Not implemented yet.");
                }
                else
                {
                    curr_chunk->move_gap_to_current_line();
                    curr_chunk->gap_start--;
                    curr_chunk->curr_line--;
                    // line_in_chunk--; // TODO: This one is triky!

                    removed = true;
                }
            }
            else
                PANIC("No idea why removing the letter failed.");
        }

        return removed;
    }

    bool buffer_point::delete_char_at_cursor_forward()
    {
        auto removed = curr_chunk->lines[line_in_chunk].delete_char_forward();

        // TODO: Handle the case when above fails.
        return removed;
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
#endif

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
