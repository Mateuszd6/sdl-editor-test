#ifndef __GAP_BUFFER_HPP_INCLUDED__
#define __GAP_BUFFER_HPP_INCLUDED__

// TODO: Document these macros!
#define GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND (2)
#define GAP_BUF_SIZE_AFTER_REALLOC (64)
#define GAP_BUF_MAX_SIZE_BEFORE_MEM_SHRINK (128)

struct gap_buffer
{
    uint8* buffer;
    size_t alloced_mem_size;

    // Navigation pointers:
    uint8* gap_start;
    uint8* gap_end;

    // TODO(Cleaup): I think this should be removed. Possibly create other class
    //               called gap_buffer_ptr, or just just regular uint8*?
    uint8* curr_point;

    void initialize();
    void set_point_at_idx(int64 index);
    void move_gap_to_current_point();
    void move_gap_to_buffer_end();
    int64 get_gap_size() const;

    // Only these should be used.
    bool cursor_forward();
    bool cursor_backward();

    bool jump_start_dumb();
    bool jump_end_dumb();

    // Can shrink buffer memory.
    bool delete_char_forward();
    bool delete_char_backward();

    // Can exapnd buffer memory.
    void insert_at_point(uint8 character);

    // TODO(Cleaup): Change to size_t.
    int64 get_size() const;
    int64 get_idx() const;

    // TODO: Temporary, remove later:
    uint8 const* to_c_str() const;

    void DEBUG_print_state() const;
};

// The position from where the buffer has been moved is undefined.
static void move_gap_bufffer(gap_buffer* from, gap_buffer* to);

#endif // __GAP_BUFFER_HPP_INCLUDED__
