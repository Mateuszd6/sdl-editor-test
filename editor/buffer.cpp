// TODO(Cleanup): Try to use as less globals as possible...
namespace editor::global
{
    static int number_of_buffers = 0;
    static editor::buffer buffers[256];
}

namespace editor
{
    // TODO(Docs): Summary!
    static editor::buffer *CreateNewBuffer()
    {
        auto result = global::buffers + global::number_of_buffers;
        global::number_of_buffers++;

        // Initailize all lines.
        for (int i = 0; i < NUMBER_OF_LINES_IN_BUFFER; ++i)
            result->lines[i].initialize();

        return result;
    }

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
}
