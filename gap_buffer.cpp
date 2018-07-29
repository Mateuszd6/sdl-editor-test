#include <cstdlib>
#include <cstring>

typedef uint8_t uint8;

struct gap_buffer
{
    uint8* buffer;
    size_t alloced_mem_size;

    // Navigation pointers:
    uint8* gap_start;
    uint8* gap_end;

    // TODO(Cleaup): I think this should be removed. Possibly create other class called
    //               gap_buffer_ptr, or just just regulrat uint8*?
    uint8* curr_point;

    void initialize();
    void make_gap();

    void cursor_backwards();
    void cursor_forward();
    void insert_at_point(uint8 character);

    void DEBUG_print_state() const;

    // TODO: Temporary, remove later:
    uint8 const* to_c_str() const;
};

uint8 const* gap_buffer::to_c_str() const
{
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

    printf("result is: \"%s\"\n", result);
    return result;
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

void gap_buffer::cursor_backwards()
{
    if (curr_point != buffer)
    {
        if (curr_point == gap_end + 1) // TODO(Cleanup): Is this legal -> gap_end + 1.
            curr_point = gap_start;
        else
            curr_point--;
    }
    else
        PANIC("Already at the begin of the line!");
}

void gap_buffer::cursor_forward()
{
    if ((buffer + alloced_mem_size) != curr_point) // TODO: Make sure this is correct.
    {
        if (curr_point == gap_start)
            curr_point = gap_end + 1; // TODO(Cleanup): Is this legal -> gap_end + 1.
        else
            curr_point++;
    }
    else
        PANIC("Already at the end of the line!");
}

void gap_buffer::insert_at_point(uint8 character) // LATIN2 characters only.
{
    // If cursor is inside the buffer.
    if (curr_point == gap_start)
    {
        *curr_point = character;
        curr_point++;
        gap_start++;
    }
    else if (IS_EARLIER_IN_ARR(buffer, curr_point, gap_start))
    {
        auto diff = gap_start - curr_point;
        memcpy(gap_end - diff, curr_point, sizeof(uint8) * diff);
        gap_start -= diff;
        gap_end -= diff;
        insert_at_point(character);
    }
    else if (IS_EARLIER_IN_ARR(buffer, gap_start, curr_point))
    {
        auto diff = curr_point - gap_end;
        memcpy(gap_start, gap_end, sizeof(uint8) * diff);
        gap_start += diff;
        gap_end += diff;
        curr_point = gap_start;
        insert_at_point(character);
    }
    else
        UNREACHABLE();
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
    // print_buffer[print_buffer_idx++] = '0';
    // ASSERT(print_buffer_idx < alloced_mem_size); // TODO: Or <= ????
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

#if 0
// ---------------------------------

int main()
{
    std::printf("Size of gap_buffer struct: %ld\n", sizeof(gap_buffer));

    gap_buffer test_line;
    test_line.initialize();
    test_line.DEBUG_print_state();
    test_line.insert_at_point('a');
    test_line.insert_at_point('b');
    test_line.insert_at_point('c');
    test_line.insert_at_point('d');
    test_line.insert_at_point('e');
    test_line.insert_at_point('f');
    test_line.insert_at_point('g');
    test_line.DEBUG_print_state();
    test_line.cursor_backwards();
    test_line.cursor_backwards();
    test_line.cursor_backwards();
    test_line.cursor_backwards();
    test_line.DEBUG_print_state();
    test_line.insert_at_point('o');
    test_line.DEBUG_print_state();
    test_line.cursor_forward();
    test_line.cursor_forward();
    // test_line.cursor_forward();
    test_line.DEBUG_print_state();
    test_line.insert_at_point('x');

    while(1)
    {
        char c;
        scanf("%c", &c);

        switch(static_cast<int>(c))
        {
            case 113:
            {
                test_line.cursor_backwards();
            } break;

            case 119:
            {
                test_line.cursor_forward();
            } break;

            default:
            {
                if ('a' <= c && c <= 'z')
                    test_line.insert_at_point(c);
            } break;
        }

        system("clear");
        test_line.DEBUG_print_state();
    }
    return 0;
}
#endif
