#pragma once
// reviewed: 2025-09-28

// solves circular references
// implemented in 'decouple_impl.hpp'

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "token.hpp"

[[nodiscard]] inline auto line_and_col_num_for_char_index(
    const size_t at_line, size_t char_index_in_source,
    const std::string_view src) -> std::pair<size_t, size_t> {

    if (char_index_in_source >= src.size()) {
        return {at_line, 0};
    }

    size_t at_col{};
    while (src[char_index_in_source] != '\n') {
        ++at_col;
        if (char_index_in_source == 0) {
            break;
        }
        --char_index_in_source;
    }

    return {at_line, at_col};
}

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class stmt_call;
class stmt_block;
class type;
class expr_any;

struct operand {
    enum class operand_kind : uint8_t { empty, reg, memory, immediate };

    operand_kind kind_{operand_kind::empty};

  public:
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    std::string allocation_register;
    std::string base_register;
    std::string index_register;
    std::string immediate;
    int32_t displacement{};
    uint8_t scale{1};
    size_t size{};
    const type* type_ptr{};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    operand() = default;

    [[nodiscard]] static auto imm(std::string value, const type& value_type)
        -> operand {

        if (value.empty()) {
            throw std::invalid_argument("operand text must not be empty");
        }
        operand result;
        result.kind_ = operand_kind::immediate;
        result.immediate = std::move(value);
        result.type_ptr = &value_type;

        return result;
    }

    [[nodiscard]] static auto reg(const std::string_view name,
                                  const size_t operand_size) -> operand {

        if (name.empty()) {
            throw std::invalid_argument("operand text must not be empty");
        }
        operand result;
        result.kind_ = operand_kind::reg;
        result.base_register = name;
        result.size = operand_size;

        return result;
    }

    [[nodiscard]] static auto mem(const std::string_view base,
                                  const std::string_view index,
                                  const uint8_t index_scale,
                                  const int32_t offset) -> operand {

        if (base.empty() and index.empty() and offset == 0) {
            throw std::invalid_argument("operand address must not be empty");
        }
        operand result;
        result.kind_ = operand_kind::memory;
        result.base_register = base;
        result.index_register = index;
        result.scale = index_scale;
        result.displacement = offset;

        return result;
    }

    [[nodiscard]] auto kind() const -> operand_kind { return kind_; }

    [[nodiscard]] auto is_register() const -> bool {
        return kind_ == operand_kind::reg;
    }

    [[nodiscard]] auto is_memory() const -> bool {
        return kind_ == operand_kind::memory;
    }

    [[nodiscard]] auto is_immediate() const -> bool {
        return kind_ == operand_kind::immediate;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return kind_ == operand_kind::empty;
    }

    [[nodiscard]] auto is_indexed() const -> bool {
        return not index_register.empty() or displacement != 0;
    }
};

struct var_info {
    std::string name;
    const type* type_ptr{};
    token src_loc_tk;     // token for position in the source
    int32_t stack_idx{};  // location relative to register rbp
    bool is_array{};
    size_t array_size{};
    operand reg;
};

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, EMPTY };

    std::string id;
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<::operand> lea_path;
    operand operand; // nasm valid source
    int32_t stack_idx{};
    int64_t const_value{};
    size_t array_size{};
    bool is_array{};
    bool use_operand{};
    ident_type ident_type{};

    [[nodiscard]] static auto make_empty() -> ident_info {
        return {
            .id{},
            .elem_path{},
            .type_path{},
            .lea_path{},
            .operand{},
            .ident_type{ident_type::EMPTY},
        };
    }

    [[nodiscard]] static auto make_register(const std::string_view ident,
                                            const ::operand& reg)
        -> ident_info {

        assert(not ident.empty());
        assert(reg.is_register() and reg.type_ptr);

        return {
            .id{ident},
            .elem_path{reg.base_register},
            .type_path{reg.type_ptr},
            .lea_path{::operand{}},
            .operand{reg},
            .ident_type{ident_type::REGISTER},
        };
    }

    [[nodiscard]] static auto make_const(const std::string_view ident,
                                         const std::string_view elem,
                                         const type& tp, const int64_t value)
        -> ident_info {

        assert(not ident.empty());
        assert(not elem.empty());

        return {
            .id{ident},
            .elem_path{std::string{elem}},
            .type_path{&tp},
            .lea_path{::operand{}},
            .operand{},
            .const_value{value},
            .ident_type{ident_type::CONST},
        };
    }

    [[nodiscard]] static auto
    make_var(std::string ident, std::vector<std::string> elem_path,
             std::vector<const type*> type_path, const ::operand& op,
             const int32_t stack_idx, const size_t array_size,
             const bool is_array) -> ident_info {

        assert(not ident.empty());
        assert(not elem_path.empty());
        assert(elem_path.size() == type_path.size());

        const size_t lea_size{elem_path.size()};

        return {
            .id{std::move(ident)},
            .elem_path{std::move(elem_path)},
            .type_path{std::move(type_path)},
            .lea_path{lea_size, ::operand{}},
            .operand{op},
            .stack_idx{stack_idx},
            .array_size{array_size},
            .is_array{is_array},
            .ident_type{ident_type::VAR},
        };
    }

    [[nodiscard]] auto validate_invariants() const -> bool {
        if (is_empty()) {
            return id.empty() and elem_path.empty() and type_path.empty() and
                   lea_path.empty();
        }

        if (id.empty() or elem_path.empty() or type_path.empty()) {
            return false;
        }

        if (elem_path.size() != type_path.size() or
            elem_path.size() != lea_path.size()) {

            return false;
        }

        if (is_const()) {
            return elem_path.size() == 1 and operand.is_empty();
        }

        if (is_register()) {
            return elem_path.size() == 1 and operand.is_register() and
                   not operand.base_register.empty() and
                   operand.index_register.empty() and operand.displacement == 0;
        }

        return is_var();
    }

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return ident_type == ident_type::EMPTY;
    }

    [[nodiscard]] auto has_lea() const -> bool {
        return std::ranges::any_of(lea_path, [](const ::operand& lea) -> bool {
            return not lea.is_empty();
        });
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(validate_invariants());

        return *type_path.back();
    }

    void push(std::string path_elem, const type* tp, ::operand lea) {
        assert(validate_invariants());

        id += "." + path_elem;
        elem_path.emplace_back(std::move(path_elem));
        type_path.emplace_back(tp);
        lea_path.emplace_back(std::move(lea));

        assert(validate_invariants());
    }

    void pop() {
        assert(validate_invariants());

        id.resize(id.rfind('.'));
        elem_path.pop_back();
        type_path.pop_back();
        lea_path.pop_back();

        assert(validate_invariants());
    }

    void increment_offset(const int32_t n) {
        assert(validate_invariants());
        assert(stack_idx + n >= 0);

        stack_idx += n;
        operand.displacement += n;

        assert(validate_invariants());
    }
};

//
// functions necessary to solve circular references, implemented in
// 'decouple_impl.hpp'
//

[[nodiscard]] auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  token tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_call(toc& tc, tokenizer& tz,
                                    const stmt_identifier& si,
                                    token open_paren_tk)
    -> std::unique_ptr<statement>;
