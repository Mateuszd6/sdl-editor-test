namespace editor
{
    bool buffer::insert_character(uint8 character)
    {
        auto succeeded = false;
        if (character != '\0')
        {
            lines[curr_line].insert_at_point(character);
            curr_index++;

            succeeded = true; // Inserting character cannot fail.
        }

        return succeeded;
    }

    bool buffer::move_forward_curr_line()
    {
        auto succeeded = lines[curr_line].cursor_forward();
        if (succeeded)
            curr_index++;

        return succeeded;
    }

    bool buffer::move_backward_curr_line()
    {
        auto succeeded = lines[curr_line].cursor_backward();
        if (succeeded)
            curr_index--;

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

    bool buffer::delete_char_at_cursor_backward()
    {
        auto succeeded = lines[curr_line].delete_char_backward();

        if (succeeded)
            curr_index--;

        return succeeded;
    }

    bool buffer::delete_char_at_cursor_forward()
    {
        auto succeeded = lines[curr_line].delete_char_forward();
        return succeeded;
    }
}
