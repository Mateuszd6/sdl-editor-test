#include "undo_buffer.hpp"

void undo_buffer::DEBUG_print_state() const
{
}

operation_info const* undo_buffer::undo()
{
    if(operation_index == -1)
        return nullptr;

    current_operation = NONE;
    return &operations[operation_index--];
}

operation_info const* undo_buffer::redo()
{
    if(operation_index + 1== operation_size)
        return nullptr;

    current_operation = NONE;
    operation_index++;
    return &operations[operation_index];
}

void undo_buffer::add_undo_info(uint64 curr_line,
                                uint64 curr_idx,
                                uint64 data_size,
                                operation_enum operation,
                                misc::length_buffer text_buffer_weak_ptr)
{
    if(operation == current_operation)
    {
        switch(operation)
        {
            case INSERT_CHARACTERS:
            {
#if 1
                if(operations[operation_index].data_ptr[operations[operation_index].data_size - 1] != ' '
                    && text_buffer_weak_ptr.data[0] == ' ')
                {
                    // We wont append because we ended up with the space.
                    break;
                }
#endif

                // Append the text to the back.
                // TODO: Handle the case, when there is not enought memory in data_ptr.
                memcpy(operations[operation_index].data_ptr + operations[operation_index].data_size,
                       text_buffer_weak_ptr.data,
                       data_size);
                operations[operation_index].data_size += data_size;

                // TODO: For now we assume that we dont over-extend the buffer.
                buffer_size += data_size;
                ASSERT(buffer_size < buffer_capacity);
            } return;

            case NONE:
            case REMOVE_CHARACTERS:
                break;
        }
    }

    // Appending to the back
    operation_index++;
    operation_size = operation_index + 1;

    if(buffer_size + text_buffer_weak_ptr.length >= buffer_capacity)
        PANIC("Should get rid of the old undo data, which is not implemetned");

    memcpy(buffer + buffer_size, text_buffer_weak_ptr.data, text_buffer_weak_ptr.length);
    auto data_ptr = buffer + buffer_size;
    buffer_size += text_buffer_weak_ptr.length;

    if (operation_index + 1 == operation_capacity)
    {
        operation_capacity *= 2;
        operations = static_cast<operation_info*>(realloc(operations,
                                                          sizeof(operation_info) * operation_capacity));
    }

    operations[operation_index].curr_line = curr_line;
    operations[operation_index].curr_idx = curr_idx;
    operations[operation_index].data_size = data_size;
    operations[operation_index].operation = operation;
    operations[operation_index].data_ptr = data_ptr;

    current_operation = operation;
}

void undo_buffer::stop_current_operation()
{
    current_operation = NONE;
}

/* SCENERIO:
   1) foobar |mateusz
   2) |mateusz
   3) |
*/
static void undo_buffer_initialize(undo_buffer* ub)
{
    ub->buffer = static_cast<uint8*>(malloc(sizeof(uint8) * UNDO_BUFFER_SIZE));
    ub->buffer_size = 0;
    ub->buffer_capacity = UNDO_BUFFER_MAX_SIZE; // TODO!
    ub->operation_index = -1;
    ub->operation_size = 0;
    ub->operation_capacity = 32;
    ub->operations = static_cast<operation_info*>(malloc(sizeof(operation_info) * ub->operation_capacity));

    /////

#if 1
    auto text1 = "mateusz ";
    auto text2 = "foobar ";

    ub->add_undo_info(0, 0, 7, REMOVE_CHARACTERS, misc::length_buffer(text2, 7));
    ub->add_undo_info(0, 0, 8, REMOVE_CHARACTERS, misc::length_buffer(text1, 8));
#endif

}
