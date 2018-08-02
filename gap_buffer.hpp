#ifndef __GAP_BUFFER_HPP_INCLUDED__
#define __GAP_BUFFER_HPP_INCLUDED__

struct gap_buffer
{
    uint8* buffer;
    size_t alloced_mem_size;

    // Navigation pointers:
    uint8* gap_start;
    uint8* gap_end;

    // TODO(Cleaup): I think this should be removed. Possibly create other
    //               class called gap_buffer_ptr, or just just regular
    //               uint8*?
    uint8* curr_point;

    void initialize();
    void make_gap();

    bool cursor_forward();
    bool cursor_backward();
    bool delete_char_forward();
    bool delete_char_backward();

    void move_buffer_to_current_point();
    void insert_at_point(uint8 character);

#if 0
    // TODO(Cleaup): Change to size_t.
    int get_idx() const;
#endif

    // TODO: Temporary, remove later:
    uint8 const* to_c_str() const;

    void DEBUG_print_state() const;
};

#endif // __GAP_BUFFER_HPP_INCLUDED__
