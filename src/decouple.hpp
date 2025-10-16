#pragma once
// solves circular references
// implemented in 'main.cpp'

// reviewed: 2025-09-28

#include <memory>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class type;
class expr_any;

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, FIELD, IMPLIED };

    std::string id;
    std::string id_nasm; // NASM valid source
    int64_t const_value{};
    const type& type_ref;
    int32_t stack_ix{};
    size_t array_size{};
    const bool is_array{};
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<std::string> lea_path;
    const ident_type ident_type{ident_type::CONST};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }

    [[nodiscard]] auto is_memory_operand() const -> bool {
        return id_nasm.find_first_of('[') != std::string::npos;
    }
};

struct var_info {
    std::string name;
    const type& type_ref;
    token declared_at_tk; // token for position in the source
    int32_t stack_idx{};  // location relative to register rsp
    bool is_array{};
    size_t array_size{};
};

//
// functions necessary to solve circular references implemented in 'main.cpp'
//

inline auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement>;

inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement>;

inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any>;
