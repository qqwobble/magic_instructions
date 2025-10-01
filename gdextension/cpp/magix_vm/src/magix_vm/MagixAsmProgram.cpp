#include "magix_vm/MagixAsmProgram.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/compilation/assembler.hpp"
#include "magix_vm/compilation/lexer.hpp"
#include "magix_vm/convert_magix_godot.hpp"
#include "magix_vm/variant_helper.hpp"

void
magix::MagixAsmProgram::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_asm_source"), &magix::MagixAsmProgram::get_asm_source);
    godot::ClassDB::bind_method(godot::D_METHOD("set_asm_source", "source_code"), &magix::MagixAsmProgram::set_asm_source);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING, "asm_source", godot::PROPERTY_HINT_MULTILINE_TEXT), "set_asm_source", "get_asm_source"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("compile"), &magix::MagixAsmProgram::compile);

    godot::ClassDB::bind_method(godot::D_METHOD("get_bytecode"), &MagixAsmProgram::get_bytecode);

    godot::ClassDB::bind_method(godot::D_METHOD("get_error_count"), &MagixAsmProgram::get_error_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_error_info", "index"), &MagixAsmProgram::get_error_info);

    ADD_SIGNAL(godot::MethodInfo(MAGIX_ASM_PROGRAM_SIG_BYTECODE_INVALIDATED));
    ADD_SIGNAL(godot::MethodInfo(MAGIX_ASM_PROGRAM_SIG_COMPILE_FAILED));
    ADD_SIGNAL(godot::MethodInfo(MAGIX_ASM_PROGRAM_SIG_COMPILE_OK));
    ADD_SIGNAL(godot::MethodInfo(MAGIX_ASM_PROGRAM_SIG_COMPILED, godot::PropertyInfo(godot::Variant::BOOL, "success")));
}

void
magix::MagixAsmProgram::set_asm_source(const godot::String &source_code)
{
    reset();
    asm_source = source_code;
    emit_changed();
}

void
magix::MagixAsmProgram::reset()
{
    tried_compile = false;
    if (byte_code.is_valid())
    {
        emit_signal(MAGIX_ASM_PROGRAM_SIG_BYTECODE_INVALIDATED);
        byte_code.unref();
    }
    errors.clear();
    asm_source = godot::String{};
}

auto
magix::MagixAsmProgram::compile() -> bool
{
    if (tried_compile)
    {
        return errors.empty();
    }
    tried_compile = true;

    godot::Ref<MagixByteCode> new_bc;
    new_bc.instantiate();

    const godot::String &source = get_asm_source();

    std::vector<magix::compile::SrcToken> tokens = magix::compile::lex(magix::compile::strview_from_godot(source));
    errors = assemble(tokens, new_bc->get_code_write());

    bool result = errors.empty();
    if (result)
    {
        byte_code = std::move(new_bc);
        emit_signal(MAGIX_ASM_PROGRAM_SIG_COMPILE_OK);
    }
    else
    {
        emit_signal(MAGIX_ASM_PROGRAM_SIG_COMPILE_FAILED);
    }
    emit_signal(MAGIX_ASM_PROGRAM_SIG_COMPILED, result);
    return result;
}

auto
magix::MagixAsmProgram::get_bytecode() -> godot::Ref<magix::MagixByteCode>
{
    compile();
    return byte_code;
}

auto
magix::MagixAsmProgram::get_error_count() -> size_t
{
    compile();
    return errors.size();
}

auto
magix::MagixAsmProgram::get_error_info(size_t index) -> godot::Dictionary
{
    compile();
    ERR_FAIL_UNSIGNED_INDEX_V(index, errors.size(), {});

    magix::overload overloader{
        [](const magix::compile::assembler_errors::NumberInvalid &err) {
            godot::Dictionary result;
            result["type"] = "NUMBER_INVALID";
            result["start_line"] = err.token.begin.line;
            result["start_column"] = err.token.begin.column;
            result["end_line"] = err.token.end.line;
            result["end_column"] = err.token.end.column;
            result["number"] = magix::compile::srcview_to_godot(err.token.content);
            return result;
        },
        [](const magix::compile::assembler_errors::NumberNotRepresentable &err) {
            godot::Dictionary result;
            result["type"] = "NUMBER_INVALID";
            result["start_line"] = err.token.begin.line;
            result["start_column"] = err.token.begin.column;
            result["end_line"] = err.token.end.line;
            result["end_column"] = err.token.end.column;
            result["number"] = magix::compile::srcview_to_godot(err.token.content);
            return result;
        },
        [](const magix::compile::assembler_errors::UnexpectedToken &err) {
            godot::Dictionary result;
            result["type"] = "UNEXPECTED_TOKEN";
            result["start_line"] = err.got.begin.line;
            result["start_column"] = err.got.begin.column;
            result["end_line"] = err.got.end.line;
            result["end_column"] = err.got.end.column;
            godot::PackedStringArray expected;
            for (const auto &expected_token : err.expected)
            {
                expected.append(magix::compile::srcview_to_godot(enum_name(expected_token)));
            }
            result["expected"] = expected;
            return result;
        },
        [](const magix::compile::assembler_errors::UnknownInstruction &err) {
            godot::Dictionary result;
            result["type"] = "UNKNOWN_INSTRUCTION";
            result["start_line"] = err.instruction_name.begin.line;
            result["start_column"] = err.instruction_name.begin.column;
            result["end_line"] = err.instruction_name.end.line;
            result["end_column"] = err.instruction_name.end.column;
            result["name"] = magix::compile::srcview_to_godot(err.instruction_name.content);
            return result;
        },
        [](const magix::compile::assembler_errors::DuplicateLabels &err) {
            godot::Dictionary result;
            result["type"] = "DUPLICATE_LABEL";
            result["start_line"] = err.second_declaration.begin.line;
            result["start_column"] = err.second_declaration.begin.column;
            result["end_line"] = err.second_declaration.end.line;
            result["end_column"] = err.second_declaration.end.column;
            result["first_declaration_start_line"] = err.first_declaration.begin.line;
            result["first_declaration_start_column"] = err.first_declaration.begin.column;
            result["first_declaration_end_line"] = err.first_declaration.end.line;
            result["first_declaration_end_column"] = err.first_declaration.end.column;
            result["name"] = magix::compile::srcview_to_godot(err.second_declaration.content);
            return result;
        },
        [](const magix::compile::assembler_errors::MissingArgument &err) {
            godot::Dictionary result;
            result["type"] = "MISSING_ARGUMENT";
            result["start_line"] = err.source_instruction.begin.line;
            result["start_column"] = err.source_instruction.begin.column;
            result["end_line"] = err.source_instruction.end.line;
            result["end_column"] = err.source_instruction.end.column;
            result["mnenomic"] = magix::compile::srcview_to_godot(err.mnenomic);
            result["missing_reg_name"] = magix::compile::srcview_to_godot(err.reg_name);
            result["missing_reg_number"] = err.reg_number;
            return result;
        },
        [](const magix::compile::assembler_errors::TooManyArguments &err) {
            godot::Dictionary result;
            result["type"] = "TOO_MANY_ARGUMENTS";
            result["start_line"] = err.additional_reg.begin.line;
            result["start_column"] = err.additional_reg.begin.column;
            result["end_line"] = err.additional_reg.end.line;
            result["end_column"] = err.additional_reg.end.column;
            result["mnenomic"] = magix::compile::srcview_to_godot(err.mnenomic);
            result["additional_reg_name"] = magix::compile::srcview_to_godot(err.additional_reg.content);
            result["additional_reg_number"] = err.reg_number;
            return result;
        },
        [](const magix::compile::assembler_errors::ExpectedLocalGotImmediate &err) {
            godot::Dictionary result;
            result["type"] = "EXPECTED_REGISTER";
            result["start_line"] = err.mismatched.begin.line;
            result["start_column"] = err.mismatched.begin.column;
            result["end_line"] = err.mismatched.end.line;
            result["end_column"] = err.mismatched.end.column;
            result["mnenomic"] = magix::compile::srcview_to_godot(err.mnenomic);
            result["reg_name"] = magix::compile::srcview_to_godot(err.reg_name);
            result["reg_number"] = err.reg_number;
            return result;
        },
        [](const magix::compile::assembler_errors::ExpectedImmediateGotLocal &err) {
            godot::Dictionary result;
            result["type"] = "EXPECTED_IMMEDIATE";
            result["start_line"] = err.mismatched.begin.line;
            result["start_column"] = err.mismatched.begin.column;
            result["end_line"] = err.mismatched.end.line;
            result["end_column"] = err.mismatched.end.column;
            result["mnenomic"] = magix::compile::srcview_to_godot(err.mnenomic);
            result["reg_name"] = magix::compile::srcview_to_godot(err.reg_name);
            result["reg_number"] = err.reg_number;
            return result;
        },
        [](const magix::compile::assembler_errors::EntryMustPointToCode &err) {
            godot::Dictionary result;
            result["type"] = "ENTRY_POINT_NOT_POINTING_TO_CODE";
            result["start_line"] = err.label_declaration.begin.line;
            result["start_column"] = err.label_declaration.begin.column;
            result["end_line"] = err.label_declaration.end.line;
            result["end_column"] = err.label_declaration.end.column;
            result["label"] = magix::compile::srcview_to_godot(err.label_declaration.content);
            return result;
        },
        [](const magix::compile::assembler_errors::UnknownDirective &err) {
            godot::Dictionary result;
            result["type"] = "UNKNOWN_DIRECTIVE";
            result["start_line"] = err.directive.begin.line;
            result["start_column"] = err.directive.begin.column;
            result["end_line"] = err.directive.end.line;
            result["end_column"] = err.directive.end.column;
            result["label"] = magix::compile::srcview_to_godot(err.directive.content);
            return result;
        },
        [](const magix::compile::assembler_errors::CompilationTooBig &err) {
            godot::Dictionary result;
            result["type"] = "COMPILATION_TOO_BIG";
            result["start_line"] = 0;
            result["start_column"] = 0;
            result["end_line"] = 0;
            result["end_column"] = 0;
            result["max_size"] = err.maximum;
            result["is_size"] = err.data_size;
            return result;
        },
        [](const magix::compile::assembler_errors::UnboundLabel &err) {
            godot::Dictionary result;
            result["type"] = "LABEL_UNBOUND";
            result["start_line"] = err.which.begin.line;
            result["start_column"] = err.which.begin.column;
            result["end_line"] = err.which.end.line;
            result["end_column"] = err.which.end.column;
            return result;
        },
        [](const magix::compile::assembler_errors::ConfigRedefinition &err) {
            godot::Dictionary result;
            result["type"] = "CONFIG_REDIFINED";
            result["start_line"] = err.redef.begin.line;
            result["start_column"] = err.redef.begin.column;
            result["end_line"] = err.redef.end.line;
            result["end_column"] = err.redef.end.column;
            return result;
        },
        [](const magix::compile::assembler_errors::InternalError &err) {
            godot::Dictionary result;
            result["type"] = "COMPILATION_TOO_BIG";
            result["start_line"] = 0;
            result["start_column"] = 0;
            result["end_line"] = 0;
            result["end_column"] = 0;
            result["ID"] = err.line_number;
            return result;
        },
    };
    return std::visit(overloader, errors[index]);
}
