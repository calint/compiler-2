#pragma once
// reviewed: 2025-09-28

// solves circular references
// implemented in 'decouple_impl.hpp'

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
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

class operand {
    enum class kind : uint8_t { empty, reg, memory, immediate };

    kind kind_{kind::empty};
    const type* type_ptr_{};
    std::string allocation_register_;
    std::string base_register_;
    std::string index_register_;
    std::string immediate_;
    int32_t displacement_{};
    uint8_t scale_{1};

  public:
    operand() = default;

    [[nodiscard]] static auto imm(std::string value, const type& value_type)
        -> operand;

    [[nodiscard]] static auto reg(const std::string_view name,
                                  const type& value_type) -> operand;

    [[nodiscard]] static auto
    mem(const std::string_view base, const std::string_view index,
        const uint8_t index_scale, const int32_t offset, const type& value_type)
        -> operand;

    [[nodiscard]] static auto mem(const operand& address,
                                  const type& value_type) -> operand {
        assert(address.is_memory() or address.is_register());

        return mem(address.base_register_, address.index_register_,
                   address.scale_, address.displacement_, value_type);
    }

    [[nodiscard]] auto allocation_register() const -> const std::string& {
        return allocation_register_;
    }

    [[nodiscard]] auto base_register() const -> const std::string& {
        return base_register_;
    }

    [[nodiscard]] auto index_register() const -> const std::string& {
        return index_register_;
    }

    [[nodiscard]] auto immediate() const -> const std::string& {
        return immediate_;
    }

    [[nodiscard]] auto displacement() const -> int32_t { return displacement_; }

    [[nodiscard]] auto scale() const -> uint8_t { return scale_; }

    void set_allocation_register(const std::string_view name) {
        assert(is_register());
        allocation_register_ = name;
    }

    void increment_offset(const int32_t offset) {
        assert(is_memory());

        displacement_ += offset;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(type_ptr_);

        return *type_ptr_;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return kind_ == kind::reg;
    }

    [[nodiscard]] auto is_memory() const -> bool {
        return kind_ == kind::memory;
    }

    [[nodiscard]] auto is_immediate() const -> bool {
        return kind_ == kind::immediate;
    }

    [[nodiscard]] auto is_empty() const -> bool { return kind_ == kind::empty; }

    [[nodiscard]] auto is_indexed() const -> bool {
        return not index_register_.empty() or displacement_ != 0;
    }
};

struct var_info {
    std::string name;
    const type* type_ptr{};
    token src_loc_tk; // token for position in the source
    int32_t offset{}; // location offset from base register
    bool is_array{};
    bool is_pointer{};
    size_t array_len{};
    operand reg; // variable location is in register
    std::string_view base_register;
};

struct ident_info {
    enum class kind : uint8_t { EMPTY, CONST, VAR, REGISTER };

    std::string id;
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<::operand> lea_path;
    operand operand;  // nasm valid source
    int32_t offset{}; // location offset from base register
    int64_t const_value{};
    size_t array_len{};
    bool is_array{};
    bool is_pointer{};
    bool use_operand{}; // operand overrides any location calculation
    kind kind{};

    [[nodiscard]] static auto make_empty() -> ident_info {
        return {
            .id{},
            .elem_path{},
            .type_path{},
            .lea_path{},
            .operand{},
        };
    }

    [[nodiscard]] static auto make_register(const std::string_view ident,
                                            const ::operand& reg)
        -> ident_info {

        assert(not ident.empty());
        assert(reg.is_register());

        return {
            .id{ident},
            .elem_path{reg.base_register()},
            .type_path{&reg.type_ref()},
            .lea_path{::operand{}},
            .operand{reg},
            .kind{kind::REGISTER},
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
            .kind{kind::CONST},
        };
    }

    [[nodiscard]] static auto
    make_var(std::string ident, std::vector<std::string> elem_path,
             std::vector<const type*> type_path, const ::operand& op,
             const int32_t offset, const size_t array_len, const bool is_array,
             const bool is_pointer = {}) -> ident_info {

        assert(not ident.empty());
        assert(not elem_path.empty());
        assert(elem_path.size() == type_path.size());

        const size_t lea_count{elem_path.size()};

        return {
            .id{std::move(ident)},
            .elem_path{std::move(elem_path)},
            .type_path{std::move(type_path)},
            .lea_path{lea_count, ::operand{}},
            .operand{op},
            .offset{offset},
            .array_len{array_len},
            .is_array{is_array},
            .is_pointer{is_pointer},
            .kind{kind::VAR},
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
                   not operand.base_register().empty() and
                   operand.index_register().empty() and
                   operand.displacement() == 0;
        }

        return is_var();
    }

    [[nodiscard]] auto is_const() const -> bool { return kind == kind::CONST; }

    [[nodiscard]] auto is_register() const -> bool {
        return kind == kind::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool { return kind == kind::VAR; }

    [[nodiscard]] auto is_empty() const -> bool { return kind == kind::EMPTY; }

    [[nodiscard]] auto has_lea() const -> bool {
        return std::ranges::any_of(lea_path, [](const ::operand& lea) -> bool {
            return not lea.is_empty();
        });
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(validate_invariants());

        return *type_path.back();
    }

    void push(std::string path_elem, const type* const tp, ::operand lea) {
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
        assert(offset + n >= 0);

        offset += n;
        operand.increment_offset(n);

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
                                                  const token tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_call(toc& tc, tokenizer& tz,
                                    const stmt_identifier& si,
                                    const token open_paren_tk)
    -> std::unique_ptr<statement>;
