#include <cstdlib>
#include <cstring>

#include "../config.h"

void gap_buffer::initialize()
{
    capacity = GAP_BUF_DEFAULT_CAPACITY;
    buffer = static_cast<uint8*>(std::malloc(sizeof(uint8) * capacity));

    if (!buffer)
        PANIC("Allocating memory failed.");

    gap_start = buffer;
    gap_end = buffer + capacity;
}

void gap_buffer::move_gap_to_point(size_t point)
{
    ASSERT(point <= size());
    if (buffer + point == gap_start)
    {
    }
    else if (buffer + point < gap_start)
    {
        auto diff = gap_start - (buffer + point);
        memcpy(gap_end - diff, buffer + point, sizeof(uint8) * diff);
        gap_start -= diff;
        gap_end -= diff;
    }
    else if (buffer + point > gap_start)
    {
        auto diff = point - (gap_start - buffer);
        memcpy(gap_start, gap_end, sizeof(uint8) * diff);
        gap_start += diff;
        gap_end += diff;
    }
    else
        UNREACHABLE();
}

void gap_buffer::move_gap_to_buffer_end()
{
    if (gap_end == buffer + capacity)
        LOG_WARN("Gap already at the end.");
    else
    {
        auto move_from_the_end = (buffer + capacity) - gap_end;
        memcpy(gap_start, gap_end, sizeof(uint8) * move_from_the_end);
        gap_start += move_from_the_end;
        gap_end += move_from_the_end;
    }
}

// TODO(Testing): Battle-test this. Watch out for allocations!
void gap_buffer::insert_at_point(size_t point, uint8 character) // LATIN2 characters only.
{
    ASSERT(point <= size());
    move_gap_to_point(point);

    *gap_start = character;
    gap_start++;

    // Expand memory if needed:
    auto old_gap_size = gap_size();
    if (old_gap_size <= GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND)
    {
        move_gap_to_buffer_end();
        LOG_DEBUG("Doing realloc. Line has %lu characters, gap moved to the end.",
                  gap_start-buffer);

        auto gap_start_idx = gap_start - buffer;
        auto new_gap_size = GAP_BUF_SIZE_AFTER_REALLOC - old_gap_size;
        auto new_size = capacity + new_gap_size;

        // TODO(Testing): Test it with custom realloc that always moves the memory.
        auto new_ptr = static_cast<uint8*>(malloc(sizeof(uint8) * new_size));
        memcpy(new_ptr, buffer, sizeof(uint8) * capacity);
        free(buffer);

        if (new_ptr)
            buffer = new_ptr;
        else
            PANIC("Realloc has failed.");

        capacity = new_size;
        gap_start = buffer + gap_start_idx;
        gap_end = gap_start + new_gap_size;
    }

    ASSERT(gap_size() > GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND);
}

bool gap_buffer::delete_char_backward(size_t point)
{
    ASSERT(0 < point && point <= size());

    auto curr_point = buffer + point;
    if (curr_point != buffer)
    {
        if (curr_point != gap_start)
            move_gap_to_point(point);

        gap_start--;

        // TODO(#3): This is the only place that the memory can be shrinked, if
        //           we want to shrink it on the deleteion which im not sure if
        //           this is good idea.
        return true;
    }
    else
        return false;
}

bool gap_buffer::delete_char_forward(size_t point)
{
    ASSERT(point < size());

#if 0
    auto can_move_forward = (point + 1 < size());
    return can_move_forward ? delete_char_backward(point + 1) : false;
#else

    delete_char_backward(point + 1);
    return true;
#endif
}

size_t gap_buffer::size() const
{
    return capacity - gap_size();
}

size_t gap_buffer::gap_size() const
{
    return (gap_end - gap_start);
}

int8* gap_buffer::to_c_str() const
{
    // TODO: Alloc function.
    auto result = static_cast<int8*>(std::malloc(sizeof(uint8) * capacity));
    auto resutl_idx = 0;
    auto in_gap = false;
    result[0] = '\0';

    for (auto i = 0_u64; i < capacity; ++i)
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

void gap_buffer::DEBUG_print_state() const
{
    char print_buffer[capacity + 5];
    auto print_buffer_idx = 0LLU;
    auto in_gap = false;
    auto printed_chars = 0LLU;

    auto ptr = buffer;
    for (auto i = 0ULL; i < capacity; ++i)
    {
        if (ptr == gap_start)
            in_gap = true;

        if (ptr == gap_end)
            in_gap = false;

        print_buffer[print_buffer_idx++] = (in_gap ? '?' : *ptr);
        printed_chars++;

        ptr++;
    }

    ASSERT(print_buffer_idx <= capacity); // TODO: Or < ????
    for (auto i = print_buffer_idx; i < capacity; ++i)
        print_buffer[print_buffer_idx++] = '_';
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf("%s", print_buffer);

    print_buffer_idx = 0;
    ptr = buffer;
    for (auto i = 0ULL; i <= capacity; ++i)
    {
        if (ptr == gap_start)
            print_buffer[print_buffer_idx++] = 'b';
        else if (ptr == gap_end)
            print_buffer[print_buffer_idx++] = 'e';
        else
            print_buffer[print_buffer_idx++] = ' ';
        ptr++;
    }
    print_buffer[print_buffer_idx++] = '\n';
    print_buffer[print_buffer_idx++] = '\0';
    printf("%s", print_buffer);
}

static void move_gap_bufffer(gap_buffer* from, gap_buffer* to)
{
    auto gap_start_offset = from->gap_start - from->buffer;
    auto gap_end_offset = from->gap_end - from->buffer;

    to->capacity = from->capacity;
    to->buffer = from->buffer;
    to->gap_start = (to->buffer + gap_start_offset);
    to->gap_end = (to->buffer + gap_end_offset);

    // TODO(Cleaup): After doing buffer_chunk - try to hide it, because memory
    // does not have to be cleared.
    from->capacity = 0;
    from->buffer = nullptr;
    from->gap_start = nullptr;
    from->gap_end = nullptr;
}
