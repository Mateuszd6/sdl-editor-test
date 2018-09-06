#ifndef __GAP_BUFFER_HPP_INCLUDED__
#define __GAP_BUFFER_HPP_INCLUDED__

/// Initial size of the gap after initialization.
#define GAP_BUF_DEFAULT_CAPACITY (80)

/// Minimal size of the buffer before it gets expanded.
#define GAP_BUF_MIN_SIZE_BEFORE_MEM_EXPAND (2)

/// The size of a gap after expanding the memory.
#define GAP_BUF_SIZE_AFTER_REALLOC (64)

/// Max size of the gap before shrinking.
#define GAP_BUF_MAX_SIZE_BEFORE_MEM_SHRINK (128)

struct gap_buffer
{
    /// Allocated capacity of the buffer.
    size_t capacity;

    /// The content of the text buffer.
    uint8* buffer;

    /// Pointer to the first character that is not in the gap. Character pointer
    /// by this pointer is not in the structure and is not defined.
    uint8* gap_start;

    /// Pointer to the first vaild character that is outside of the gap. If the
    /// gap is at the end of a buffer, this poitner poitns outside to the
    /// allocated structure, and should not be referenced.
    uint8* gap_end;

    /// Must be called before any action with this data structure is done.
    void initialize();

    /// Move tha gap so that gap_start is at the point. Called by inserting
    /// character if the gap is at the different place by the start of the
    /// operation. Assertion: gap_start == buffer + point after this operation.
    void move_gap_to_point(size_t point);

    /// Place the gap and the end of tha buffer.
    void move_gap_to_buffer_end();

    /// Set the gap size to 'n' characters if is is smaller. Does nothing if the
    /// gap size is already greater or equal 'n'.
    void reserve_gap(size_t n);

    /// Insert character at point. Will move the gap to the pointed location if
    /// necesarry. Can exapnd buffer memory.
    void insert_at_point(size_t point, uint8 character); // LATIN2 characters only.

    /// Insert character at point. This doesn't move a gap or expand memory.
    void replace_at_point(size_t point, uint8 character);

    /// Delete the character currently on the given point. Will move the gap to
    /// the pointed location if necesarry. Can shrink buffer memory.
    bool delete_char_backward(size_t point);

    /// Delete the character currently on the given point. Should give the same
    /// result as moving forward one character and removing character backward.
    /// Will move the gap to the pointed location if necesarry. Can shrink
    /// buffer memory.
    bool delete_char_forward(size_t point);

    /// Efficiently clears the text to the end of the line by moving a gap.
    bool delete_to_the_end_of_line(size_t point);

    /// Returns the number of characters stored in the buffer. Assertion: size()
    /// + gap_size() == capacity for every vaild state of gap buffer.
    size_t size() const;

    /// Returns a size of the gap. Assertion: size() + gap_size() == capacity,
    /// for every vaild state of gap buffer.
    size_t gap_size() const;

    /// Return the idx'th character in the buffer.
    uint8 get(size_t idx) const;

    /// Operator alias for the 'get' function. Returns the idx'th character in
    /// the buffer.
    uint8 operator [](size_t idx) const;

    /// Returns the c_str representation of the line. Allocates the memory for
    /// the string. The caller is repsonsible for freeing this memory.
    int8* to_c_str() const;

    /// Pretty prints the structure state to console.
    void DEBUG_print_state() const;

#if 0

    struct iterator
    {
        gap_buffer* gapb;
        uint8* curr;

        iterator operator=(const iterator&);
        bool operator==(const iterator&);
        bool operator!=(const iterator&);
        void operator++(); //prefix increment
        uint8 operator*() const;
    };

    iterator begin();
    iterator end();

#endif
};

/// The position from where the buffer has been moved is undefined.
static void move_gap_bufffer(gap_buffer* from, gap_buffer* to);

#endif // __GAP_BUFFER_HPP_INCLUDED__
