#include "undo_buffer.hpp"

void undo_buffer::DEBUG_print_state() const
{
    // printf("UNDO BUFFER:\n");
    auto idx = 1;
    auto curr = current_point;
    while(1)
    {
        auto op_ptr = reinterpret_cast<operation_info*>(curr);
        printf("%d: operation: %d at %lu:%lu, of characters%s: \"",
               idx++,
               op_ptr->operation,
               op_ptr->curr_line,
               op_ptr->curr_idx,
               (op_ptr->operation == operation_enum::IS_LAST
                ? "(LAST)"
                : (op_ptr->operation == operation_enum::IS_FIRST ? "(FIRST)" : "")));

        auto data_size = op_ptr->data_size;
        for(auto i = 0_u64; i < data_size; ++i)
            printf("%c", *(curr - data_size + i));
        printf("\"\n");

        if (op_ptr->operation == operation_enum::IS_FIRST)
            break;

        curr -= (sizeof(operation_info) + data_size);
        if (curr < buffer)
            PANIC("Undo buffer is corrupted!");
    }
}

operation_details undo_buffer::undo()
{
    auto op_ptr = reinterpret_cast<operation_info*>(current_point);
    auto result = operation_details { };
    result.data_weak_ref = misc::length_buffer { current_point - op_ptr->data_size, op_ptr->data_size };
    result.operation_type = op_ptr->operation;
    result.curr_line = op_ptr->curr_line;
    result.curr_idx = op_ptr->curr_idx;

    current_point -= (sizeof(operation_info) + op_ptr->data_size);

    return result;
}

/* SCENERIO:
   1) foobar |mateusz
   2) |mateusz
   3) |
*/
static void undo_buffer_initialize(undo_buffer* ub)
{
    ub->buffer = static_cast<uint8*>(std::malloc(sizeof(uint8) * UNDO_BUFFER_SIZE));

    // Insert none operation at the beginning:
    // TODO: This assumes that the UNDO_BUFFER_SIZE is greater than the
    // operation_info struct.
    auto text1 = "foobar  ";
    auto text2 = "mateusz";

    auto op_ptr = reinterpret_cast<operation_info*>(ub->buffer);
    op_ptr->operation = operation_enum::IS_FIRST;
    op_ptr->curr_line = 0_u64;
    op_ptr->curr_idx = 0_u64;
    op_ptr->data_size = 0;
    op_ptr->data_size_next = strlen(text1);

    std::memcpy(ub->buffer + sizeof(operation_info), reinterpret_cast<const uint8*>(text1), strlen(text1));
    op_ptr = reinterpret_cast<operation_info*>(ub->buffer + sizeof(operation_info) + strlen(text1));
    op_ptr->operation = operation_enum::REMOVE_CHARACTERS;
    op_ptr->curr_line = 0_u64;
    op_ptr->curr_idx = 0_u64;
    op_ptr->data_size = strlen(text1);
    op_ptr->data_size_next = strlen(text2);

    std::memcpy(ub->buffer + sizeof(operation_info) + strlen(text1) + sizeof(operation_info),
                reinterpret_cast<const uint8*>(text2),
                strlen(text2));
    op_ptr = reinterpret_cast<operation_info*>(
        ub->buffer + sizeof(operation_info) + strlen(text1) + sizeof(operation_info) + strlen(text2));
    op_ptr->operation = operation_enum::REMOVE_CHARACTERS;
    op_ptr->curr_line = 0_u64;
    op_ptr->curr_idx = 0_u64;
    op_ptr->data_size = strlen(text2);
    op_ptr->data_size_next = 0;

    ub->current_point = ub->buffer + sizeof(operation_info) + strlen(text1) + sizeof(operation_info) + strlen(text2);
}
