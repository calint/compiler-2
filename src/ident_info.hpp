#pragma once

// what a name resolves to ('ident_info'), what a variable is declared as
// ('var_info'), where 'compile' writes a value ('destination') and what a
// caller asks of the address of an identifier ('lea_request')

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "operand.hpp"
#include "token.hpp"
#include "ub_alias.hpp"

class type;

// why a name cannot be written, for the diagnostic; 'none' is writable
enum class read_only_cause : uint8_t {
    none,
    let,
    param,
    foo_element,
    foo_counter,
};

// the array that the element 'e' of a 'foo' belongs to: its root variable and
// the path to it, empty when not known
class foo_array_info final {
  public:
    foo_array_info(std::string root_in,
                   const std::optional<access_path>& path_in)
        : root{std::move(root_in)}, path{path_in} {}

    foo_array_info() = default;

    std::string root;
    std::optional<access_path> path;
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

    // empty unless this is the element 'e' of a 'foo'
    foo_array_info foo_array;
};

// the address of an element of a path computed at an inlined call, e.g.
// '[rbp + r15 * 8 + 16]' for 'rooms[i]' of the argument 'world.rooms[i]';
// 'trailing_count' is the elements of the path after it, e.g. 1 for the 'x'
// of 'p.x' when 'p' names 'world.rooms[i]'
struct known_address {
    operand address;
    size_t trailing_count{};
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
    // the deepest address known from the calls, empty when none is
    std::optional<known_address> known;
    operand operand;
    int64_t offset{}; // location offset from base register
    int64_t const_value{};
    size_t array_len{};
    bool is_array{};
    bool is_pointer{};
    read_only_cause read_only_why{};
    kind kind{};

    [[nodiscard]] auto has_lea() const -> bool { return known.has_value(); }

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
                elem_path.size() == type_path.size(),
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
            .known{},
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
            .known{},
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
            .known{},
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

        return {
            .id{std::move(ident)},
            .src_loc_tk{},
            .elem_path{std::move(elem_path)},
            .type_path{std::move(type_path)},
            .known{},
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

// where 'compile' writes a value: nowhere for a statement, a register, or the
// storage that a name of the source resolves to, e.g. 'p.x'; it is not changed
// after it is made, a field or an element of it is a destination of its own
class destination final {
    enum class kind : uint8_t { none, reg, storage };

    kind kind_{kind::none};
    ::operand operand_;
    const type* type_ptr_{};
    // e.g. 'p.x', or the register, as the comments name it
    std::string name_;
    // where the name is in the source, empty when it is not
    token src_loc_tk_;
    // the operand is the address, the name need not be resolved again, e.g.
    // an indexed element or a pointer that was loaded
    bool is_address_known_{};
    bool is_array_{};
    // the elements of an array, an unsized one gets them from the argument
    size_t array_len_{};
    // a reference to storage that the value may read under another name,
    // e.g. a 'mut' parameter or the element of a 'foo'
    bool is_reference_{};
    // the path of the root variable, empty when unknown or the whole variable
    std::optional<access_path> accessed_path_;

    // what a resolved name adds when it is the destination of an assignment
    struct name_parts {
        ::operand address;
        bool is_address_known{};
        std::optional<access_path> accessed_path;
        bool is_reference{};
    };

  public:
    destination() = default;

    [[nodiscard]] auto accessed_path() const
        -> const std::optional<access_path>& {

        return accessed_path_;
    }

    [[nodiscard]] auto array_len() const -> size_t { return array_len_; }

    // the destination at 'address', e.g. an element of an array literal
    [[nodiscard]] auto as_element(const ::operand& address) const
        -> destination {

        destination element{with_operand(address)};
        element.is_address_known_ = true;
        element.is_array_ = false;
        element.array_len_ = 0;
        return element;
    }

    // at 'address', e.g. a field that a call writes in place
    [[nodiscard]] auto at_address(const ::operand& address) const
        -> destination {

        destination located{with_operand(address)};
        located.is_address_known_ = true;
        return located;
    }

    // the name for an error about it, 'fallback' without a place in the source
    [[nodiscard]] auto error_token(const token& fallback) const -> token {
        return src_loc_tk_.is_empty() ? fallback : src_loc_tk_;
    }

    // the field 'field_name' of type 'field_type', as the comments name it
    [[nodiscard]] auto field(const std::string_view field_name,
                             const type& field_type) const -> destination {

        destination result{*this};
        result.name_ += std::format(".{}", field_name);
        result.type_ptr_ = &field_type;
        return result;
    }

    [[nodiscard]] auto is_address_known() const -> bool {
        return is_address_known_;
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_empty() const -> bool { return kind_ == kind::none; }

    [[nodiscard]] auto is_reference() const -> bool { return is_reference_; }

    [[nodiscard]] auto is_register() const -> bool {
        return kind_ == kind::reg;
    }

    // storage named in the source, a variable or a temporary
    [[nodiscard]] auto is_storage() const -> bool {
        return kind_ == kind::storage;
    }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    [[nodiscard]] auto operand() const -> const ::operand& { return operand_; }

    // the variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return ident_info::root_of(name_);
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(type_ptr_ != nullptr);

        return *type_ptr_;
    }

    // the same destination at another operand, e.g. its address in a register
    [[nodiscard]] auto with_operand(const ::operand& op) const -> destination {
        destination moved{*this};
        moved.operand_ = op;
        return moved;
    }

    //
    // statics
    //

    // a name resolved as 'info', at the operand of 'info'
    [[nodiscard]] static auto of_name(const ident_info& info) -> destination {
        return of_name(info, {
                                 .address{info.operand},
                                 .is_address_known{},
                                 .accessed_path{},
                                 .is_reference{},
                             });
    }

    // a name resolved as 'info', e.g. the destination of an assignment
    [[nodiscard]] static auto of_name(const ident_info& info, name_parts parts)
        -> destination {

        // note: a constant is rejected as a destination before
        assert(info.is_var() or info.is_register());

        destination result;
        result.kind_ = info.is_register() ? kind::reg : kind::storage;
        result.operand_ = std::move(parts.address);
        result.type_ptr_ = &info.type_ref();
        result.name_ = info.id;
        result.src_loc_tk_ = info.src_loc_tk;
        result.is_address_known_ = parts.is_address_known or info.has_lea();
        result.is_array_ = info.is_array;
        result.array_len_ = info.array_len;
        result.is_reference_ = parts.is_reference;
        result.accessed_path_ = std::move(parts.accessed_path);

        return result;
    }

    // e.g. a scratch register that an expression is computed in
    [[nodiscard]] static auto of_register(const ::operand& reg) -> destination {
        assert(reg.is_register());

        destination result;
        result.kind_ = kind::reg;
        result.operand_ = reg;
        result.type_ptr_ = &reg.type_ref();
        result.name_ = reg.base_register();

        return result;
    }
};

// what a caller asks of the address of an identifier
struct lea_request {
    // the count of a span operation on an array, empty without one
    operand reg_count;

    // the deepest address of the path known from the calls
    std::optional<known_address> known;

    // the register the caller prefers the address to start in
    operand address_register;
};
