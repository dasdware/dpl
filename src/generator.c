#include <dpl/generator.h>
#include <error.h>

void dpl_generate(DPL_Generator *generator, DPL_Bound_Node *node, DPL_Program *program)
{
    switch (node->kind)
    {
    case BOUND_NODE_VALUE:
    {
        if (dpl_symbols_is_type_base(node->type, TYPE_BASE_NUMBER))
        {
            dplp_write_push_number(program, node->as.value.as.number);
        }
        else if (dpl_symbols_is_type_base(node->type, TYPE_BASE_STRING))
        {
            dplp_write_push_string(program, node->as.value.as.string.data);
        }
        else if (dpl_symbols_is_type_base(node->type, TYPE_BASE_BOOLEAN))
        {
            dplp_write_push_boolean(program, node->as.value.as.boolean);
        }
        else
        {
            DW_ERROR("Cannot generate program for value node of type " SV_Fmt ".",
                     SV_Arg(node->type->name));
        }
    }
    break;
    case BOUND_NODE_OBJECT:
    {
        DPL_Bound_Object object = node->as.object;
        for (size_t i = 0; i < object.field_count; ++i)
        {
            dpl_generate(generator, object.fields[i].expression, program);
        }

        dplp_write_create_object(program, object.field_count);
    }
    break;
    case BOUND_NODE_LOAD_FIELD:
    {
        dpl_generate(generator, node->as.load_field.expression, program);
        dplp_write_load_field(program, node->as.load_field.field_index);
    }
    break;
    case BOUND_NODE_FUNCTIONCALL:
    {
        DPL_Bound_FunctionCall f = node->as.function_call;
        for (size_t i = 0; i < f.arguments_count; ++i)
        {
            dpl_generate(generator, f.arguments[i], program);
        }

        switch (f.function->as.function.kind)
        {
        case FUNCTION_INSTRUCTION:
            dplp_write(program, f.function->as.function.as.instruction_function);
            break;
        case FUNCTION_INTRINSIC:
            dplp_write_call_intrinsic(program, f.function->as.function.as.intrinsic_function);
            break;
        case FUNCTION_USER:
        {
            DPL_Binding_UserFunction *uf = &generator->user_functions.items[f.function->as.function.as.user_function.user_handle];
            dplp_write_call_user(program, uf->arity, uf->begin_ip);
        }
        break;
        default:
            DW_UNIMPLEMENTED_MSG("Function kind %d", f.function->as.function.kind);
        }
    }
    break;
    case BOUND_NODE_SCOPE:
    {
        DPL_Bound_Scope s = node->as.scope;
        bool prev_was_persistent = false;
        size_t persistent_count = 0;
        for (size_t i = 0; i < s.expressions_count; ++i)
        {
            if (i > 0)
            {
                if (!prev_was_persistent)
                {

                    dplp_write_pop(program);
                }
                else
                {
                    persistent_count++;
                }
            }
            dpl_generate(generator, s.expressions[i], program);
            prev_was_persistent = s.expressions[i]->persistent;
        }

        if (persistent_count > 0)
        {
            dplp_write_pop_scope(program, persistent_count);
        }
    }
    break;
    case BOUND_NODE_ARGREF:
    case BOUND_NODE_VARREF:
    {
        dplp_write_push_local(program, node->as.varref);
    }
    break;
    case BOUND_NODE_ASSIGNMENT:
    {
        dpl_generate(generator, node->as.assignment.expression, program);
        dplp_write_store_local(program, node->as.assignment.scope_index);
    }
    break;
    case BOUND_NODE_CONDITIONAL:
    {
        dpl_generate(generator, node->as.conditional.condition, program);

        // jump over then clause if condition is false
        size_t then_jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);

        dplp_write_pop(program);
        dpl_generate(generator, node->as.conditional.then_clause, program);

        // jump over else clause if condition is true
        size_t else_jump = dplp_write_jump(program, INST_JUMP);
        dplp_patch_jump(program, then_jump);

        // else clause
        dplp_write_pop(program);
        dpl_generate(generator, node->as.conditional.else_clause, program);

        dplp_patch_jump(program, else_jump);
    }
    break;
    case BOUND_NODE_LOGICAL_OPERATOR:
    {
        dpl_generate(generator, node->as.logical_operator.lhs, program);

        size_t jump;
        if (node->as.logical_operator.operator.kind == TOKEN_AND_AND)
        {
            jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);
        }
        else
        {
            jump = dplp_write_jump(program, INST_JUMP_IF_TRUE);
        }

        dpl_generate(generator, node->as.logical_operator.rhs, program);

        dplp_patch_jump(program, jump);
    }
    break;
    case BOUND_NODE_WHILE_LOOP:
    {
        // Initialize result array
        dplp_write_begin_array(program);
        dplp_write_end_array(program);

        size_t loop_start = program->code.count;

        dpl_generate(generator, node->as.while_loop.condition, program);

        // jump over loop if condition is false
        size_t exit_jump = dplp_write_jump(program, INST_JUMP_IF_FALSE);

        dplp_write_pop(program);
        dpl_generate(generator, node->as.while_loop.body, program);

        if (node->as.while_loop.in_assignment)
        {
            // Add body value to result array
            dplp_write_concat_array(program);
        }
        else
        {
            dplp_write_pop(program);
        }

        // jump over else clause if condition is true
        dplp_write_loop(program, loop_start);
        dplp_patch_jump(program, exit_jump);

        dplp_write_pop(program);
    }
    break;
    case BOUND_NODE_INTERPOLATION:
    {
        size_t count = node->as.interpolation.expressions_count;
        for (size_t i = 0; i < count; ++i)
        {
            dpl_generate(generator, node->as.interpolation.expressions[i], program);
        }

        dplp_write_interpolation(program, count);
    }
    break;
    case BOUND_NODE_ARRAY:
    {
        dplp_write_begin_array(program);
        size_t count = node->as.array.element_count;
        for (size_t i = 0; i < count; ++i)
        {
            dpl_generate(generator, node->as.array.elements[i], program);
        }
        dplp_write_end_array(program);
    }
    break;
    case BOUND_NODE_SPREAD:
    {
        dpl_generate(generator, node->as.spread, program);
        dplp_write_spread(program);
    }
    break;
    default:
        DW_UNIMPLEMENTED_MSG("`%s`", dpl_bind_nodekind_name(node->kind));
    }
}
