#include "undo_buffer.hpp"

void undo_buffer::DEBUG_print_state() const
{
    printf("UNDO BUFFER:\n");
    auto idx = 1;
    auto curr = buffer;
    while(1)
    {
        auto op_ptr = reinterpret_cast<operation_info*>(curr);
        printf("%d: operation: %d at %lu:%lu, of characters: \"",
               idx++,
               (op_ptr->operation_mask | 1_u16 << 14 | 1_u16 << 15)  ^ (1_u16 << 14 | 1_u16 << 15),
               op_ptr->curr_line,
               op_ptr->curr_idx);

        auto data_size = op_ptr->data_size;
        for(auto i = 0_u64; i < data_size; ++i)
            printf("%c", *(curr + (sizeof(operation_info) + i)));
        printf("\"\n");

        if (op_ptr->operation_mask & operation_enum::IS_LAST)
            break;
        else
            curr += (sizeof(operation_info) + data_size);
    }

}

static void undo_buffer_initialize(undo_buffer* ub)
{
    ub->buffer = static_cast<uint8*>(std::malloc(sizeof(uint8) * UNDO_BUFFER_SIZE));
    ub->last_operation = ub->buffer;

    // Insert none operation at the beginning:
    // TODO: This assumes that the UNDO_BUFFER_SIZE is greater than the
    // operation_info struct.
    auto operation_ptr = reinterpret_cast<operation_info*>(ub->last_operation);
    operation_ptr->operation_mask = operation_enum::NONE | operation_enum::IS_LAST;
    operation_ptr->curr_line = 0_u64;
    operation_ptr->curr_idx = 0_u64;
    operation_ptr->data_size = 0_u64;
}
