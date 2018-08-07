#include <cstdlib>
#include <cstring>

#include "../config.h"

static void move_gap_bufffer(gap_buffer* from, gap_buffer* to)
{
    to->buffer = from->buffer;
    to->alloced_mem_size = from->alloced_mem_size;

    auto gap_start_offset = from->gap_start - from->buffer;
    auto gap_end_offset = from->gap_end - from->buffer;
    auto curr_point_offset = from->curr_point - from->buffer;;
    to->gap_start = (to->buffer + gap_start_offset);
    to->gap_end = (to->buffer + gap_end_offset);
    to->curr_point = (to->buffer + curr_point_offset);

    from->buffer = nullptr;
    from->gap_start = nullptr;
    from->gap_end = nullptr;
    from->curr_point = nullptr;
}

// TODO(Cleaup): This should be removed by the cleaup, or ;
uint8 const* gap_buffer::to_c_str() const
{
    // TODO: Alloc function.
    auto result = static_cast<uint8*>(
        std::malloc(sizeof(uint8) * alloced_mem_size));
    result[0] = '\0';
    auto resutl_idx = 0;
    auto in_gap = false;

    for (auto i = 0_u64; i < alloced_mem_size; ++i)
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

int64 gap_buffer::get_idx() const
{
    if (IS_EARLIER_IN_ARR(buffer, curr_point, gap_start)
        || gap_start == curr_point)
    {
        return curr_point - buffer;
    }
    else
        return (gap_start - buffer) + (curr_point - gap_end);
}

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

void gap_buffer::set_point_at_idx(int64 index)
{
    auto buffer_offset = gap_start - buffer;
    if (buffer_offset <= index)
        curr_point = buffer + index;
    else
        curr_point = gap_end + (index - buffer_offset);
}

int64 gap_buffer::get_gap_size() const
{
    return (gap_end - gap_start);
}

bool gap_buffer::cursor_forward()
{
    // If we are not at the end of buffer
    if ((buffer + alloced_mem_size) != curr_point)
    {
        // If we are at the last character before gap.
        if (curr_point == gap_start)
        {
            // If the gap is at the end of allocated memory we cannot move further.
            if (gap_end == buffer + alloced_mem_size)
                return false;
            // Otherwise we just throught the gap.
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

bool gap_buffer::delete_char_backward()
{
    if (curr_point != buffer)
    {
        move_gap_to_current_point();

        gap_start--;
        curr_point--;

        // TODO(#3): This is the only place that the memory can be shrinked, if
        //           we want to shrink it on the deleteion which im not sure if
        //           this is good idea.

        return true;
    }
    else
        return false;
}

bool gap_buffer::delete_char_forward()
{
    auto moved_forward = cursor_forward();

    // Cursor position should be updated in the delete_char_backward.
    return moved_forward ? delete_char_backward() : false;
}

void gap_buffer::move_gap_to_current_point()
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

void gap_buffer::move_gap_to_buffer_end()
{
    if (gap_end == buffer + alloced_mem_size)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto move_from_the_end = (buffer + alloced_mem_size) - gap_end;
        memcpy(gap_start, gap_end, sizeof(uint8) * move_from_the_end);
        gap_start += move_from_the_end;
        gap_end += move_from_the_end;
    }
}

// TODO(Testing): Battle-test this. Watch out for allocations!
void gap_buffer::insert_at_point(uint8 character) // LATIN2 characters only.
{
    move_gap_to_current_point();

    *curr_point = character;
    curr_point++;
    gap_start++;

    // Expand memory if needed:
    auto gap_size = get_gap_size();
    if (gap_size <= GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND)
    {
        move_gap_to_buffer_end();
        LOG_DEBUG("Doing realloc. Line has %lu characters, gap moved to the end.",
                  gap_start-buffer);

        auto gap_start_idx = gap_start - buffer;
        auto curr_point_idx = curr_point - buffer;
        auto new_gap_size = GAP_BUF_SIZE_AFTER_REALLOC - gap_size;
        auto new_size = alloced_mem_size + new_gap_size;
        // TODO(Testing): Test it with custom realloc that always moves the memory.
        auto new_ptr = static_cast<uint8*>(realloc(buffer, new_size));

        if (new_ptr)
            buffer = new_ptr;
        else
            PANIC("Realloc has failed.");

        alloced_mem_size = new_size;
        gap_start = buffer + gap_start_idx;
        curr_point = buffer + curr_point_idx;
        gap_end = gap_start + new_gap_size;
    }

    ASSERT(get_gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
}

bool gap_buffer::jump_start_dumb()
{
    auto has_moved = false;
    while (cursor_backward())
        has_moved = true;

    return has_moved;
}

bool gap_buffer::jump_end_dumb()
{
    auto has_moved = false;
    while (cursor_forward())
        has_moved = true;

    return has_moved;
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

    ASSERT(print_buffer_idx <= alloced_mem_size); // TODO: Or < ????
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
