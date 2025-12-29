#include <dpl/debugger/instructions.h>
#include <dpl/debugger/ui.h>

#include <dw_byte_buffer.h>
#include <dw_error.h>

void dplg_load_instructions(DPLG_Instructions* instructions, DPL_Program* program)
{
    DW_ByteStream code = {0};
    code.buffer = program->code;
    DW_ByteStream constants = {0};
    constants.buffer = program->constants;

    while (!bs_at_end(&code))
    {
        DPLG_Instruction instruction = {0};
        instruction.ip = code.position;
        instruction.kind = bs_read_u8(&code);
        instruction.name = dplp_inst_kind_name(instruction.kind);
        instruction.bounds.x = 0;
        instruction.bounds.y = instructions->count * DPLG_INSTRUCTION_HEIGHT;
        instruction.bounds.width = DPLG_INSTRUCTION_WIDTH;
        instruction.bounds.height = DPLG_INSTRUCTION_HEIGHT;
        instruction.parameter_count = 0;

        switch (instruction.kind)
        {
        case INST_PUSH_NUMBER:
            instruction.parameter0 = dpl_value_make_number(bs_read_f64(&code));
            instruction.parameter_count = 1;
            break;
        case INST_PUSH_STRING:
            {
                constants.position = bs_read_u64(&code);
                size_t length = bs_read_u64(&constants);
                instruction.parameter0 = dpl_value_make_string(
                    &instructions->pool, length, (char*)constants.buffer.items + constants.position);
                instruction.parameter_count = 1;
            }
            break;
        case INST_PUSH_BOOLEAN:
            instruction.parameter0 = dpl_value_make_boolean(bs_read_u8(&code) == 1);
            instruction.parameter_count = 1;
            break;
        case INST_PUSH_LOCAL:
            instruction.parameter0 = dpl_value_make_number(bs_read_u64(&code));
            instruction.parameter_count = 1;
            break;
        case INST_CREATE_OBJECT:
            instruction.parameter0 = dpl_value_make_number(bs_read_u8(&code));
            instruction.parameter_count = 1;
            break;
        case INST_LOAD_FIELD:
            instruction.parameter0 = dpl_value_make_number(bs_read_u8(&code));
            instruction.parameter_count = 1;
            break;
        case INST_NOOP:
        case INST_POP:
        case INST_NEGATE:
        case INST_NOT:
        case INST_ADD:
        case INST_SUBTRACT:
        case INST_MULTIPLY:
        case INST_DIVIDE:
        case INST_LESS:
        case INST_LESS_EQUAL:
        case INST_GREATER:
        case INST_GREATER_EQUAL:
        case INST_EQUAL:
        case INST_NOT_EQUAL:
        case INST_RETURN:
        case INST_BEGIN_ARRAY:
        case INST_END_ARRAY:
        case INST_CONCAT_ARRAY:
        case INST_SPREAD:
            break;
        case INST_CALL_INTRINSIC:
            const char* name = dpl_intrinsic_kind_name(bs_read_u8(&code));
            instruction.parameter0 = dpl_value_make_string(
                &instructions->pool, strlen(name), name);
            instruction.parameter_count = 1;
            break;
        case INST_CALL_USER:
            instruction.parameter0 = dpl_value_make_number(bs_read_u8(&code));
            instruction.parameter1 = dpl_value_make_number(bs_read_u64(&code));
            instruction.parameter_count = 2;
            break;
        case INST_STORE_LOCAL:
            instruction.parameter0 = dpl_value_make_number(bs_read_u64(&code));
            instruction.parameter_count = 1;
            break;
        case INST_POP_SCOPE:
            instruction.parameter0 = dpl_value_make_number(bs_read_u64(&code));
            instruction.parameter_count = 1;
            break;
        case INST_JUMP:
        case INST_JUMP_IF_FALSE:
        case INST_JUMP_IF_TRUE:
        case INST_JUMP_LOOP:
            instruction.parameter0 = dpl_value_make_number(bs_read_u16(&code));
            instruction.parameter_count = 1;
            break;
        case INST_INTERPOLATION:
            instruction.parameter0 = dpl_value_make_number(bs_read_u8(&code));
            instruction.parameter_count = 1;
            break;
        default:
            DW_UNIMPLEMENTED_MSG("%s", dplp_inst_kind_name(instruction.kind));
        }

        nob_da_append(instructions, instruction);
    }
}
