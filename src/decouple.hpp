#pragma once
// reviewed: 2025-09-28

// solves circular references
// the statement factories at the end are implemented in 'decouple_impl.hpp'

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "token.hpp"

[[nodiscard]] inline auto line_and_col_num_for_char_index(
    const size_t at_line, size_t char_index_in_source,
    const std::string_view src) -> std::pair<size_t, size_t> {

    if (char_index_in_source >= src.size()) {
        return {at_line, 0};
    }

    size_t at_col{};
    while (src.at(char_index_in_source) != '\n') {
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
struct generic_type_instance;

// alignment padding of a size at the limit stays in the signed 64-bit range
inline constexpr size_t max_storage_size_bytes{
    static_cast<size_t>(std::numeric_limits<int64_t>::max()) - 16,
};
// note: -16 leaves room for the alignment padding

[[nodiscard]] inline auto add_storage_size(const token& src_loc_tk,
                                           const size_t base,
                                           const size_t size_bytes) -> size_t {

    if (size_bytes > max_storage_size_bytes or
        base > max_storage_size_bytes - size_bytes) {

        throw compiler_exception{src_loc_tk,
                                 "storage size exceeds signed 64-bit range"};
    }

    return base + size_bytes;
}

[[nodiscard]] inline auto multiply_storage_size(const token& src_loc_tk,
                                                const size_t size_bytes,
                                                const size_t count) -> size_t {

    if (count != 0 and size_bytes > max_storage_size_bytes / count) {
        throw compiler_exception{src_loc_tk,
                                 "storage size exceeds signed 64-bit range"};
    }

    return size_bytes * count;
}

// the sum of sizes that 'add_storage_size' or 'multiply_storage_size' already
// accepted, in a place with no source location
[[nodiscard]] inline auto sum_storage_size(const size_t base,
                                           const size_t size_bytes) -> size_t {

    assert(size_bytes <= max_storage_size_bytes);
    assert(base <= max_storage_size_bytes - size_bytes);

    return base + size_bytes;
}

// rounds 'size_bytes' up to a multiple of 'alignment', a power of two
[[nodiscard]] inline auto align_storage_size(const size_t size_bytes,
                                             const size_t alignment) -> size_t {

    assert(std::has_single_bit(alignment));

    return sum_storage_size(size_bytes,
                            (alignment - (size_bytes % alignment)) % alignment);
}

// the alignment known at 'offset' bytes from an address aligned to
// 'alignment'
[[nodiscard]] inline auto offset_alignment(const size_t offset,
                                           const size_t alignment) -> size_t {

    if (offset == 0) {
        return alignment;
    }

    const unsigned trailing_zeros{
        static_cast<unsigned>(std::countr_zero(offset)),
    };

    return std::min(alignment, size_t{1} << trailing_zeros);
}

[[nodiscard]] inline auto address_offset(const size_t size_bytes) -> int64_t {
    // storage sizes are limited to the signed 64-bit range
    assert(std::in_range<int64_t>(size_bytes));

    return static_cast<int64_t>(size_bytes);
}

// the byte offset of 'count' elements of 'size_bytes', count may be negative
[[nodiscard]] inline auto scaled_address_offset(const int64_t count,
                                                const size_t size_bytes)
    -> int64_t {

    const int64_t size{address_offset(size_bytes)};

    // constant indices are bounds checked against the storage size
    assert(size == 0 or (count <= std::numeric_limits<int64_t>::max() / size and
                         count >= std::numeric_limits<int64_t>::min() / size));

    return count * size;
}

[[nodiscard]] inline auto add_address_offset(const int64_t base,
                                             const int64_t offset) -> int64_t {

    assert(offset <= 0 or base <= std::numeric_limits<int64_t>::max() - offset);
    assert(offset >= 0 or base >= std::numeric_limits<int64_t>::min() - offset);

    return base + offset;
}

class operand {
    enum class kind : uint8_t { empty, reg, memory, immediate };

    kind kind_{kind::empty};
    const type* type_ptr_{};
    std::string allocation_register_;
    std::string base_register_;
    std::string index_register_;
    std::string immediate_;
    int64_t displacement_{};
    uint64_t scale_{1};

  public:
    operand() = default;

    [[nodiscard]] auto allocation_register() const -> const std::string& {
        return allocation_register_;
    }

    [[nodiscard]] auto base_register() const -> const std::string& {
        return base_register_;
    }

    [[nodiscard]] auto displacement() const -> int64_t { return displacement_; }

    [[nodiscard]] auto immediate() const -> const std::string& {
        return immediate_;
    }

    void increment_offset(const int64_t offset) {
        assert(is_memory());

        displacement_ = add_address_offset(displacement_, offset);
    }

    [[nodiscard]] auto index_register() const -> const std::string& {
        return index_register_;
    }

    [[nodiscard]] auto is_empty() const -> bool { return kind_ == kind::empty; }

    [[nodiscard]] auto is_immediate() const -> bool {
        return kind_ == kind::immediate;
    }

    [[nodiscard]] auto is_memory() const -> bool {
        return kind_ == kind::memory;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return kind_ == kind::reg;
    }

    [[nodiscard]] auto scale() const -> uint64_t { return scale_; }

    void set_allocation_register(const std::string_view name) {
        assert(is_register());

        allocation_register_ = name;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(type_ptr_);

        return *type_ptr_;
    }

    //
    // statics
    //

    [[nodiscard]] static auto imm(std::string value, const type& value_type)
        -> operand {

        assert(not value.empty());

        operand result;
        result.kind_ = kind::immediate;
        result.type_ptr_ = &value_type;
        result.immediate_ = std::move(value);

        return result;
    }

    [[nodiscard]] static auto mem(const std::string_view base,
                                  const std::string_view index,
                                  const uint64_t index_scale,
                                  const int64_t offset, const type& value_type)
        -> operand {

        assert(std::has_single_bit(index_scale));
        assert(not base.empty() or not index.empty() or offset != 0);

        operand result;
        result.kind_ = kind::memory;
        result.type_ptr_ = &value_type;
        result.base_register_ = base;
        result.index_register_ = index;
        result.scale_ = index_scale;
        result.displacement_ = offset;

        return result;
    }

    [[nodiscard]] static auto mem(const operand& address,
                                  const type& value_type) -> operand {

        assert(address.is_memory() or address.is_register());

        return mem(address.base_register_, address.index_register_,
                   address.scale_, address.displacement_, value_type);
    }

    [[nodiscard]] static auto reg(const std::string_view name,
                                  const type& value_type) -> operand {

        assert(not name.empty());

        operand result;
        result.kind_ = kind::reg;
        result.type_ptr_ = &value_type;
        result.base_register_ = name;

        return result;
    }
};

// why a name cannot be written, for the diagnostic; NONE is writable
enum class read_only_cause : uint8_t {
    NONE,
    LET,
    PARAM,
    FOO_ELEMENT,
    FOO_COUNTER,
};

struct var_info {
    std::string name;
    const type* type_ptr{};
    token src_loc_tk; // token for position in the source
    int64_t offset{}; // location offset from base register
    bool is_array{};
    bool is_pointer{};
    read_only_cause read_only_why{};
    size_t array_len{};
    operand pointer_register; // variable location is in register
    std::string_view base_register;
    operand value_register; // variable value is in register, no storage
};

struct ident_info {
  private:
    enum class kind : uint8_t { EMPTY, CONST, VAR, REGISTER };

  public:
    std::string id;
    // where the name is in the source, empty for a register
    token src_loc_tk;
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<::operand> lea_path;
    operand operand;
    int64_t offset{}; // location offset from base register
    int64_t const_value{};
    size_t array_len{};
    bool is_array{};
    bool is_pointer{};
    read_only_cause read_only_why{};
    bool use_operand{}; // operand overrides any location calculation
    kind kind{};

    // the name for an error about it, 'fallback' without a place in the source
    [[nodiscard]] auto error_token(const token& fallback) const -> token {
        return src_loc_tk.is_empty() ? fallback : src_loc_tk;
    }

    [[nodiscard]] auto has_lea() const -> bool {
        return std::ranges::any_of(lea_path, [](const ::operand& lea) -> bool {
            return not lea.is_empty();
        });
    }

    void increment_offset(const int64_t n) {
        assert(validate_invariants());

        offset = add_address_offset(offset, n);

        operand.increment_offset(n);

        assert(validate_invariants());
    }

    [[nodiscard]] auto is_const() const -> bool { return kind == kind::CONST; }

    [[nodiscard]] auto is_empty() const -> bool { return kind == kind::EMPTY; }

    [[nodiscard]] auto is_read_only() const -> bool {
        return read_only_why != read_only_cause::NONE;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return kind == kind::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool { return kind == kind::VAR; }

    void pop() {
        assert(validate_invariants());

        id.resize(id.rfind('.'));
        elem_path.pop_back();
        type_path.pop_back();
        lea_path.pop_back();

        assert(validate_invariants());
    }

    void push(std::string path_elem, const type* const tp, ::operand lea) {
        assert(validate_invariants());

        id += "." + path_elem;
        elem_path.emplace_back(std::move(path_elem));
        type_path.emplace_back(tp);
        lea_path.emplace_back(std::move(lea));

        assert(validate_invariants());
    }

    // variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return std::string_view{id}.substr(0, id.find('.'));
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(validate_invariants());

        return *type_path.back();
    }

    // an empty info has no type, every path has one entry per element
    [[nodiscard]] auto validate_invariants() const -> bool {
        const bool paths_match{
            not id.empty() and not elem_path.empty() and
                elem_path.size() == type_path.size() and
                elem_path.size() == lea_path.size(),
        };

        if (is_const()) {
            return paths_match and elem_path.size() == 1 and operand.is_empty();
        }

        if (is_register()) {
            return paths_match and elem_path.size() == 1 and
                   operand.is_register() and
                   not operand.base_register().empty() and
                   operand.index_register().empty() and
                   operand.displacement() == 0;
        }

        return paths_match and is_var();
    }

    //
    // statics
    //

    [[nodiscard]] static auto make_const(const std::string_view ident,
                                         const std::string_view elem,
                                         const type& tp, const int64_t value)
        -> ident_info {

        assert(not ident.empty());
        assert(not elem.empty());

        return {
            .id{ident},
            .src_loc_tk{},
            .elem_path{std::string{elem}},
            .type_path{&tp},
            .lea_path{::operand{}},
            .operand{},
            .const_value{value},
            .kind{kind::CONST},
        };
    }

    [[nodiscard]] static auto make_empty() -> ident_info {
        return {
            .id{},
            .src_loc_tk{},
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
            .src_loc_tk{},
            .elem_path{reg.base_register()},
            .type_path{&reg.type_ref()},
            .lea_path{::operand{}},
            .operand{reg},
            .kind{kind::REGISTER},
        };
    }

    [[nodiscard]] static auto
    make_var(std::string ident, std::vector<std::string> elem_path,
             std::vector<const type*> type_path, const ::operand& op,
             const int64_t offset, const size_t array_len, const bool is_array,
             const bool is_pointer = {}) -> ident_info {

        assert(not ident.empty());
        assert(not elem_path.empty());
        assert(elem_path.size() == type_path.size());

        const size_t lea_count{elem_path.size()};

        return {
            .id{std::move(ident)},
            .src_loc_tk{},
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
};

//
// statement factories implemented in 'decouple_impl.hpp' because they create
// every statement class, while every statement header includes this file
//

[[nodiscard]] auto create_statement_in_expr_arith(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_call(toc& tc, tokenizer& tz,
                                    const stmt_identifier& si,
                                    const token open_paren_tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_constructor_call(toc& tc, tokenizer& tz,
                                                const token type_tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_method_call(toc& tc, tokenizer& tz,
                                           stmt_identifier receiver)
    -> std::unique_ptr<statement>;

// parses the method of a generic type again for an instance of the type
auto instantiate_generic_method(toc& tc, const token& func_tk,
                                const token& start_tk,
                                const generic_type_instance& instance) -> void;

// the methods defined so far for a new instance of a generic type
auto instantiate_generic_methods(toc& tc, const generic_type_instance& instance)
    -> void;

// parses the generic function again with the type arguments and returns the
// name of the instance, an instance made before is reused
[[nodiscard]] auto
instantiate_generic_func(toc& tc, const token& call_tk,
                         std::string_view generic_name,
                         std::span<const type* const> type_args) -> std::string;

// e.g. 'show<name>(x)', a generic function name followed by '<' has no other
// meaning
[[nodiscard]] auto is_generic_call(const toc& tc, std::string_view name,
                                   tokenizer& tz) -> bool;

// e.g. 'show<name>(x)' where 'show' is not generic, says so instead of reading
// the '<' as an operator
auto assert_no_type_args_for_plain_func(const toc& tc, const token& tk,
                                        tokenizer& tz) -> void;

[[nodiscard]] auto is_array_literal(const toc& tc, const token& tk,
                                    tokenizer& tz) -> bool;

[[nodiscard]] auto is_default_array_literal(tokenizer& tz) -> bool;

[[nodiscard]] auto is_constructor_call(const toc& tc, const token& tk,
                                       tokenizer& tz) -> bool;

[[nodiscard]] auto is_record_literal(const toc& tc, const token& tk,
                                     tokenizer& tz) -> bool;

[[nodiscard]] auto is_bare_record_type(const toc& tc, const token& tk,
                                       tokenizer& tz) -> bool;

[[nodiscard]] auto is_bare_builtin_type(const toc& tc, const token& tk,
                                        tokenizer& tz) -> bool;
