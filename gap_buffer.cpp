#include <cstdlib>
#include <cstring>

uint8 const* gap_buffer::to_c_str() const
{
    // TODO: Alloc function.
    auto result = static_cast<uint8*>(std::malloc(sizeof(uint8) * alloced_mem_size));
    result[0] = '\0';
    auto resutl_idx = 0;
    auto in_gap = false;

    for (auto i = 0LLU; i < alloced_mem_size; ++i)
    {
        if (buffer + i == gap_start)
            in_gap = true;

        if (buffer + i == gap_end)
            in_gap = false;

        if (!in_gap)
        {
            result[resutl_idx++] = *(buffer + i);
            result[resutl_idx] = '\0';
        }
    }

    return result;
}

#if 0
int gap_buffer::get_idx() const
{
    if (IS_EARLIER_IN_ARR(buffer, curr_point, gap_start)
        || gap_start == curr_point)
    {
        return curr_point - buffer;
    }
    else
        return (gap_start - buffer) + (curr_point - gap_end);
}
#endif

void gap_buffer::initialize()
{
    alloced_mem_size = 80;
    buffer = static_cast<uint8*>(std::malloc(sizeof(uint8) * alloced_mem_size));
    // TODO(Cleanup): Panic on fail!
    // buffer_end = buffer;

    gap_start = buffer;
    gap_end = buffer + alloced_mem_size;

    curr_point = buffer;
}

bool gap_buffer::cursor_backward()
{
    if (curr_point != buffer)
    {
        if (curr_point == gap_end + 1) // TODO(Cleanup): Is this legal -> gap_end + 1.
            curr_point = gap_start;
        else
            curr_point--;

        return true;
    }
    else
        return false;
}

bool gap_buffer::cursor_forward()
{
    if ((buffer + alloced_mem_size) != curr_point) // TODO: Make sure this is correct.
    {
        if (curr_point == gap_start)
        {
            if (gap_end == buffer + alloced_mem_size)
                return false;
            else
                curr_point = gap_end + 1; // TODO(Cleanup): Is this legal -> gap_end + 1.
        }
        else
            curr_point++;
    }
    else
        return false;

    return true;
}

bool gap_buffer::delete_char_backward()
{
    if (curr_point != buffer)
    {
        move_buffer_to_current_point();

        gap_start--;
        curr_point--;

        return true;
    }
    else
        return false;
}

bool gap_buffer::delete_char_forward()
{
    if ((buffer + alloced_mem_size) != curr_point) // TODO: Make sure this is correct.
    {
        move_buffer_to_current_point();

        // If gap end is at the end of the buffer.
        // We must check this after moving the buffer.
        // TODO(Cleaup): Think if we can do it without moving the buffer first.
        if (gap_end == buffer + alloced_mem_size)
            return false;

        gap_end++; // Check if gap end is not at the end of the buffer
    }
    else
        return false;

    return true;
}

void gap_buffer::move_buffer_to_current_point()
{
    if (IS_EARLIER_IN_ARR(buffer, curr_point, gap_start))
    {
        auto diff = gap_start - curr_point;
        memcpy(gap_end - diff, curr_point, sizeof(uint8) * diff);
        gap_start -= diff;
        gap_end -= diff;
    }
    else if (IS_EARLIER_IN_ARR(buffer, gap_start, curr_point))
    {
        auto diff = curr_point - gap_end;
        memcpy(gap_start, gap_end, sizeof(uint8) * diff);
        gap_start += diff;
        gap_end += diff;
        curr_point = gap_start;
    }
    else if (curr_point == gap_start)
    {
        // TODO(Cleanup): Don't do anything?
    }
    else
        UNREACHABLE();
}

void gap_buffer::insert_at_point(uint8 character) // LATIN2 characters only.
{
    move_buffer_to_current_point();

    *curr_point = character;
    curr_point++;
    gap_start++;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

void gap_buffer::DEBUG_print_state() const
{
    char print_buffer[alloced_mem_size + 5];
    auto print_buffer_idx = 0LLU;
    auto in_gap = false;
    auto printed_chars = 0LLU;

    auto ptr = buffer;
    for (auto i = 0ULL; i < alloced_mem_size; ++i)
    {
        if (ptr == gap_start)
            in_gap = true;

        if (ptr == gap_end)
            in_gap = false;

        print_buffer[print_buffer_idx++] = (in_gap ? '?' : *ptr);
        printed_chars++;

        ptr++;
    }

    ASSERT(print_buffer_idx < alloced_mem_size); // TODO: Or <= ????
    for (auto i = print_buffer_idx; i < alloced_mem_size; ++i)
        print_buffer[print_buffer_idx++] = '_';
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf(print_buffer);

    print_buffer_idx = 0;
    ptr = buffer;
    for (auto i = 0ULL; i <= alloced_mem_size; ++i)
    {
        if (ptr == curr_point)
            print_buffer[print_buffer_idx++] = '^';
        else if (ptr == gap_start)
            print_buffer[print_buffer_idx++] = 'b';
        else if (ptr == gap_end)
            print_buffer[print_buffer_idx++] = 'e';
        else
            print_buffer[print_buffer_idx++] = ' ';
        ptr++;
    }
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf(print_buffer);

}

#pragma clang diagnostic pop
