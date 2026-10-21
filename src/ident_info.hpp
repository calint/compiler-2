#pragma once

// what a name resolves to ('ident_info'), what a variable is declared as
// ('var_info'), where 'compile' writes a value ('destination'), what a
// caller asks of the address of an identifier ('lea_request'), why a name
// cannot be written ('read_only_cause'), the array of the element of a 'foo'
// ('foo_array_info'), an address computed at an inlined call
// ('known_address') and a value known at compile time ('folded_value')

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "constant_arith.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_alias.hpp"

class type;

// why a name cannot be written, for the diagnostic; 'none' is writable
enum class read_only_cause : uint8_t {
    none,
    let,
    param,
    foo_element,
    foo_counter,
    foo_length,
};

// a value known at compile time; typed: it has the rules of a variable of
// 'own_type', e.g. a folded conversion or a constant argument of an inlined
// call; untyped: a literal or a 'let' name, checked by its value
struct folded_value {
    int64_t value{};
    // null when untyped
    const type* own_type{};

    [[nodiscard]] auto is_typed() const -> bool { return own_type != nullptr; }

    // the immediate that a destination of 'width' receives, wrapped to it
    [[nodiscard]] auto operand_at(const type& width) const -> operand {
        return operand::imm(constant_arith::wrap_to_width(value, width), width);
    }

    //
    // statics
    //

    // checked by its value, e.g. a literal or a 'let' name
    [[nodiscard]] static auto untyped(const int64_t value) -> folded_value {
        return {
            .value{value},
            .own_type{},
        };
    }

    // empty when computed at run time
    [[nodiscard]] static auto
    value_of(const std::optional<folded_value>& folded)
        -> std::optional<int64_t> {

        if (not folded) {
            return std::nullopt;
        }

        return folded->value;
    }
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

// the length that an array of unknown length is parsed and compiled with,
// what it gives is discarded, see 'ident_info::is_length_unknown'
inline constexpr size_t discarded_array_len{};

struct var_info {
    std::string name;
    const type* type_ptr{};
    token src_loc_tk;
    // location offset from base register
    int64_t offset{};
    bool is_array{};
    bool is_pointer{};
    read_only_cause read_only_why{};
    // a variable the compiler adds, see 'toc::temporary_name'
    bool is_temporary{};
    // the elements of an array, empty when not an array or not known, see
    // 'ident_info::is_length_unknown'
    std::optional<size_t> array_len;
    // variable location is in register
    operand pointer_register;
    std::string_view base_register;
    // variable value is in register, no storage
    operand value_register;

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

// what a name resolves to: nothing, a constant, a register or storage; the
// path has one entry per element from the root with its type, e.g. 'p' and
// 'x' for 'p.x'; what else is known depends on the kind and is in its part,
// the accessors give what a kind does not have as empty, e.g. 'is_array' of a
// constant is false
class ident_info final {
  public:
    // a variable or the storage that an argument names
    struct storage_part {
        ::operand operand;
        // the deepest address known from the calls, empty when none is
        std::optional<known_address> known;
        // see 'var_info::array_len'
        std::optional<size_t> array_len;
        bool is_array{};
        bool is_pointer{};
        read_only_cause read_only_why{};
    };

  private:
    // a variable or an argument whose value is held in a register
    struct register_part {
        ::operand reg;
    };

    std::string id_;
    // where the name is in the source, empty for a register
    token src_loc_tk_;
    std::vector<std::string> elem_path_;
    std::vector<const type*> type_path_;
    // a constant: a literal, a named constant or, typed, the value of an
    // argument of an inlined call, which has the type of its parameter
    std::variant<std::monostate, folded_value, register_part, storage_part>
        part_;

  public:
    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        const storage_part* const storage{std::get_if<storage_part>(&part_)};
        return storage == nullptr ? std::nullopt : storage->array_len;
    }

    [[nodiscard]] auto constant() const -> const folded_value& {
        return std::get<folded_value>(part_);
    }

    // e.g. 'p', 'x' for 'p.x'
    [[nodiscard]] auto elem_path() const -> const std::vector<std::string>& {
        return elem_path_;
    }

    // 1 for a name that is not an array, see 'discarded_array_len'
    [[nodiscard]] auto element_count() const -> size_t {
        return is_array() ? array_len().value_or(discarded_array_len) : 1;
    }

    [[nodiscard]] auto has_known_address() const -> bool {
        return known().has_value();
    }

    // e.g. 'p.x', the name as resolved
    [[nodiscard]] auto id() const -> const std::string& { return id_; }

    // the root is a pointer, or an argument whose address an inlined call
    // computed at run time
    [[nodiscard]] auto is_address_computed() const -> bool {
        return is_pointer() or has_known_address();
    }

    [[nodiscard]] auto is_array() const -> bool {
        const storage_part* const storage{std::get_if<storage_part>(&part_)};
        return storage != nullptr and storage->is_array;
    }

    [[nodiscard]] auto is_const() const -> bool {
        return std::holds_alternative<folded_value>(part_);
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return std::holds_alternative<std::monostate>(part_);
    }

    // an unsized array parameter where no call gave it a length: in the parse
    // and in the check of a function that nothing calls
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return is_array() and not array_len();
    }

    [[nodiscard]] auto is_pointer() const -> bool {
        const storage_part* const storage{std::get_if<storage_part>(&part_)};
        return storage != nullptr and storage->is_pointer;
    }

    [[nodiscard]] auto is_read_only() const -> bool {
        return read_only_why() != read_only_cause::none;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return std::holds_alternative<register_part>(part_);
    }

    [[nodiscard]] auto is_var() const -> bool {
        return std::holds_alternative<storage_part>(part_);
    }

    [[nodiscard]] auto known() const -> std::optional<known_address> {
        const storage_part* const storage{std::get_if<storage_part>(&part_)};
        return storage == nullptr ? std::nullopt : storage->known;
    }

    // the register that holds the value, or the storage operand, e.g.
    // '[rbp - 8]'
    [[nodiscard]] auto operand() const -> const ::operand& {
        if (const register_part* const reg{
                std::get_if<register_part>(&part_),
            }) {

            return reg->reg;
        }

        return std::get<storage_part>(part_).operand;
    }

    [[nodiscard]] auto read_only_why() const -> read_only_cause {
        const storage_part* const storage{std::get_if<storage_part>(&part_)};

        return storage == nullptr ? read_only_cause::none
                                  : storage->read_only_why;
    }

    // variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return root_of(id_);
    }

    auto set_src_loc_tk(const token& tk) -> void { src_loc_tk_ = tk; }

    // where the name is in the source, empty for a register
    [[nodiscard]] auto src_loc_tk() const -> const token& {
        return src_loc_tk_;
    }

    // the part of storage, for the resolver that completes it
    [[nodiscard]] auto storage() -> storage_part& {
        return std::get<storage_part>(part_);
    }

    // the type of each element of 'elem_path'
    [[nodiscard]] auto type_path() const -> const std::vector<const type*>& {
        return type_path_;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(validate_invariants());

        return *type_path_.back();
    }

    // an empty info has no type, every path has one entry per element
    [[nodiscard]] auto validate_invariants() const -> bool {
        const bool paths_match{
            not id_.empty() and not elem_path_.empty() and
                elem_path_.size() == type_path_.size(),
        };

        if (is_const()) {
            return paths_match and elem_path_.size() == 1;
        }

        if (is_register()) {
            const ::operand& reg{std::get<register_part>(part_).reg};

            return paths_match and elem_path_.size() == 1 and
                   reg.is_register() and not reg.base_register().empty() and
                   reg.index_register().empty() and reg.displacement() == 0;
        }

        return paths_match and is_var();
    }

    //
    // statics
    //

    [[nodiscard]] static auto
    make_const(const std::string_view ident, const std::string_view elem,
               const type& tp, const folded_value& constant) -> ident_info {

        assert(not ident.empty());
        assert(not elem.empty());

        // note: a typed constant has the type of its parameter
        assert(not constant.is_typed() or constant.own_type == &tp);

        ident_info info;
        info.id_ = ident;
        info.elem_path_ = {std::string{elem}};
        info.type_path_ = {&tp};
        info.part_ = constant;

        return info;
    }

    [[nodiscard]] static auto make_empty() -> ident_info { return {}; }

    [[nodiscard]] static auto make_register(const std::string_view ident,
                                            const ::operand& reg)
        -> ident_info {

        assert(not ident.empty());
        assert(reg.is_register());

        ident_info info;
        info.id_ = ident;
        info.elem_path_ = {std::string{reg.base_register()}};
        info.type_path_ = {&reg.type_ref()};

        info.part_ = register_part{
            .reg{reg},
        };

        return info;
    }

    [[nodiscard]] static auto make_var(std::string ident,
                                       std::vector<std::string> elem_path,
                                       std::vector<const type*> type_path,
                                       storage_part storage) -> ident_info {

        assert(not ident.empty());
        assert(not elem_path.empty());
        assert(elem_path.size() == type_path.size());

        ident_info info;
        info.id_ = std::move(ident);
        info.elem_path_ = std::move(elem_path);
        info.type_path_ = std::move(type_path);
        info.part_ = std::move(storage);

        return info;
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
    // an operand of an operator, e.g. the 'g()' of 'w + g()', see 'as_operand'
    bool is_operand_{};
    // its arithmetic wraps on purpose, see 'as_wrapping'; otherwise the
    // overflow check is as '--checks' asks
    bool wraps_{};
    // the elements of an array, an unsized one gets them from the argument,
    // see 'var_info::array_len'
    std::optional<size_t> array_len_;
    // the path of the root variable, empty when unknown or the whole variable
    std::optional<access_path> accessed_path_;

    // what a resolved name adds when it is the destination of an assignment
    struct name_parts {
        ::operand address;
        bool is_address_known{};
        std::optional<access_path> accessed_path;
    };

  public:
    destination() = default;

    [[nodiscard]] auto accessed_path() const
        -> const std::optional<access_path>& {

        return accessed_path_;
    }

    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        return array_len_;
    }

    // the element 'index' of the array at 'address', e.g. of an array
    // literal
    [[nodiscard]] auto as_element(const size_t index,
                                  const ::operand& address) const
        -> destination {

        destination element{with_operand(address)};
        element.is_address_known_ = true;
        element.is_array_ = false;
        element.array_len_ = std::nullopt;

        if (element.accessed_path_) {
            element.accessed_path_->add_fixed_index(index);
        }

        return element;
    }

    // a call result narrower than an operand widens into it like a variable,
    // alone it has the type of its destination, e.g. 'x = g()'
    [[nodiscard]] auto as_operand() const -> destination {
        destination result{*this};
        result.is_operand_ = true;
        return result;
    }

    // the argument of a conversion, whose arithmetic wraps, e.g. the '+' of
    // 'i8(a + b)'; a call in it builds destinations of its own, so its
    // arguments and body are checked as anywhere else
    [[nodiscard]] auto as_wrapping() const -> destination {
        destination result{*this};
        result.wraps_ = true;
        return result;
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

    // the field 'fld', as the comments name it; the source names no field of
    // a literal, so an error about it is at its value, e.g. the 't()' of
    // 'rec{2, t()}'
    [[nodiscard]] auto field(const type_field& fld) const -> destination {
        destination result{*this};
        result.name_ += std::format(".{}", fld.name);
        result.type_ptr_ = &fld.type();
        result.src_loc_tk_ = {};

        if (result.accessed_path_) {
            result.accessed_path_->add_field(fld.offset);
        }

        return result;
    }

    // a scratch register that holds the value on its way to this
    // destination, its arithmetic is checked alike
    [[nodiscard]] auto in_register(const ::operand& reg) const -> destination {
        destination result{of_register(reg)};
        result.wraps_ = wraps_;
        return result;
    }

    [[nodiscard]] auto is_address_known() const -> bool {
        return is_address_known_;
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_empty() const -> bool { return kind_ == kind::none; }

    // see 'ident_info::is_length_unknown'
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return is_array_ and not array_len_;
    }

    [[nodiscard]] auto is_operand() const -> bool { return is_operand_; }

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

    [[nodiscard]] auto wraps() const -> bool { return wraps_; }

    //
    // statics
    //

    // a name resolved as 'info', at the operand of 'info'
    [[nodiscard]] static auto of_name(const ident_info& info) -> destination {
        return of_name(info, {
                                 .address{info.operand()},
                                 .is_address_known{},
                                 .accessed_path{},
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
        result.name_ = info.id();
        result.src_loc_tk_ = info.src_loc_tk();

        result.is_address_known_ =
            parts.is_address_known or info.has_known_address();

        result.is_array_ = info.is_array();
        result.array_len_ = info.array_len();
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
