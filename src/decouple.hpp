#pragma once
// reviewed: 2025-09-28

// solves circular references, holds the cross-file declarations
// the statement factories at the end are implemented in 'decouple_impl.hpp'

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "operand.hpp"
#include "token.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class stmt_call;
class type;
class expr_any;
struct generic_type_instance;
// why a name cannot be written, for the diagnostic; 'none' is writable
enum class read_only_cause : uint8_t {
    none,
    let,
    param,
    foo_element,
    foo_counter,
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
    // a typed constant is the value of an argument of an inlined call, it has
    // the type of its parameter and the rules of a variable of that type
    enum class kind : uint8_t { empty, constant, typed_constant, var, reg };

    // where a variable is and what it holds, besides its operand
    struct var_layout {
        int64_t offset;
        size_t array_len;
        bool is_array;
        bool is_pointer;
    };

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

    auto increment_offset(const int64_t n) -> void {
        assert(validate_invariants());

        offset = add_address_offset(offset, n);

        operand.increment_offset(n);

        assert(validate_invariants());
    }

    [[nodiscard]] auto is_const() const -> bool {
        return kind == kind::constant or kind == kind::typed_constant;
    }

    [[nodiscard]] auto is_empty() const -> bool { return kind == kind::empty; }

    [[nodiscard]] auto is_read_only() const -> bool {
        return read_only_why != read_only_cause::none;
    }

    [[nodiscard]] auto is_register() const -> bool { return kind == kind::reg; }

    [[nodiscard]] auto is_typed_const() const -> bool {
        return kind == kind::typed_constant;
    }

    [[nodiscard]] auto is_var() const -> bool { return kind == kind::var; }

    auto pop() -> void {
        assert(validate_invariants());

        id.resize(id.rfind('.'));
        elem_path.pop_back();
        type_path.pop_back();
        lea_path.pop_back();

        assert(validate_invariants());
    }

    auto push(std::string path_elem, const type* const tp, ::operand lea)
        -> void {

        assert(validate_invariants());

        id += "." + path_elem;
        elem_path.emplace_back(std::move(path_elem));
        type_path.emplace_back(tp);
        lea_path.emplace_back(std::move(lea));

        assert(validate_invariants());
    }

    // variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return root_of(id);
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

    [[nodiscard]] static auto
    make_const(const std::string_view ident, const std::string_view elem,
               const type& tp, const int64_t value, const bool is_typed = {})
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
            .kind{is_typed ? kind::typed_constant : kind::constant},
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
            .kind{kind::reg},
        };
    }

    [[nodiscard]] static auto make_var(std::string ident,
                                       std::vector<std::string> elem_path,
                                       std::vector<const type*> type_path,
                                       const ::operand& op,
                                       const var_layout& layout) -> ident_info {

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
            .offset{layout.offset},
            .array_len{layout.array_len},
            .is_array{layout.is_array},
            .is_pointer{layout.is_pointer},
            .kind{kind::var},
        };
    }

    // 'p' of 'p.x.y', the variable name without the field path
    [[nodiscard]] static auto root_of(const std::string_view path)
        -> std::string_view {

        return path.substr(0, path.find('.'));
    }
};

// what a caller asks of the address of an identifier
struct lea_request {
    // the count of a span operation on an array, empty without one
    operand reg_count;

    // the addresses of the elements of the path known from the call stack
    std::span<const operand> lea_path;

    // the register the caller prefers the address to start in
    operand address_register;
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

// compiles the element count of a bulk operation into the register the
// machine asks for, the statement must outlive the emitter
[[nodiscard]] auto make_count_emitter(toc& tc, size_t indent,
                                      const statement& count)
    -> std::function<void(const operand&)>;

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
