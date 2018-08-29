// TODO(Cleanup): Try to use as less globals as possible...
namespace editor::global
{
    static int number_of_buffers = 0;
    static editor::buffer buffers[256];
}

namespace editor
{
    // Line 0 is valid.
    void buffer::initialize()
    {
        prev_chunks_size = 0;
        lines = static_cast<gap_buffer*>(malloc(sizeof(gap_buffer) * NUMBER_OF_LINES_IN_BUFFER));
        gap_start = 1;
        gap_end = NUMBER_OF_LINES_IN_BUFFER;

        lines[0].initialize();
    }

    /// After this move point'th line is not valid and must be initialized before use.
    void buffer::move_gap_to_point(size_t point)
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

    void buffer::move_gap_to_buffer_end()
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
    bool buffer::insert_character(size_t line, size_t point, uint8 character)
    {
        get_line(line)->insert_at_point(point, character);

        if (get_line(line)->gap_start == nullptr)
            PANIC("Nooooooo");

        return true;
    }

    /// line - number of line after we insert newline.
    bool buffer::insert_newline(size_t line)
    {
        move_gap_to_point(line + 1);
        gap_start++;
        lines[line + 1].initialize();

        // TODO: Case when there is not enought memory.
        return true;
    }

    bool buffer::insert_newline_correct(size_t line, size_t point)
    {
        ASSERT(line < size());

        insert_newline(line);
        auto edited_line = get_line(line);
        auto created_line = get_line(line + 1);
        ASSERT(point <= edited_line->size());


        for(auto i = point; i < edited_line->size(); ++i)
            created_line->insert_at_point(i - point, edited_line->get(i));

        // TODO: Move gap to the end in the gap_buffer class.
        edited_line->delete_to_the_end_of_line(point);
        return true;
    }

    bool buffer::delete_line(size_t line)
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

    size_t buffer::size() const
    {
        return NUMBER_OF_LINES_IN_BUFFER - gap_size();
    }

    size_t buffer::gap_size() const
    {
        return gap_end - gap_start;
    }

    gap_buffer* buffer::get_line(size_t line)
    {
        ASSERT(line < size());

        if (line < gap_start)
            return &lines[line];
        else
            return &lines[gap_end + (line - gap_start)];
    }

    void buffer::DEBUG_print_state() const
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

#include "cstdio"

namespace editor
{
    // TODO(Docs): Summary!
    // TODO(Cleaup): Move to buffer file.
    static buffer* CreateNewBuffer()
    {
        auto result = global::buffers + global::number_of_buffers;
        global::number_of_buffers++;
        result->initialize();

        return result;
    }

    // TODO(NEXT): Implement better file API than this horrible c++ streams.
    static buffer* create_buffer_from_file(char const* file_path)
    {
        auto file = fopen(file_path, "r");

        auto line_capacity = 128_u64;
        auto line_size = 0_u64;
        auto line = static_cast<int8*>(malloc(sizeof(int8) * line_capacity));
        line[0] = 0_u64;

        auto c = EOF;
        do
        {
            c = fgetc(file);

            if (c == EOF)
                break;
            else if(c == '\n')
            {
                LOG_WARN("LINE: %s", line);
                line_size = 0;
                line[0] = '\0';
                continue;
            }
            else if (line_size + 1 >= line_capacity)
            {
                line_capacity *= 2;
                // TODO: Fix common realloc mistake!
                line = static_cast<int8*>(realloc(line, sizeof(int8) * line_capacity));
                ASSERT(line);
            }

            line[line_size++] = c;
            line[line_size] = '\0';
        } while(c != EOF);

        LOG_WARN("LINE: %s", line);

        // may check feof here to make a difference between eof and io failure
        // -- network timeout for instance
        fclose(file);
        free(line);

        LOG_DEBUG("DONE");

        return CreateNewBuffer();
    }

    static buffer_point create_buffer_point(buffer* buffer_ptr)
    {
        auto result = buffer_point {};
        result.buffer_ptr = buffer_ptr;
        result.first_line = 0;
        result.curr_line = 0;
        result.curr_idx = 0;
        result.last_line_idx = -1;
        result.starting_from_top = true;

        return result;
    }
}
