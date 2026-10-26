#pragma once

// what a name resolves to ('ident_info'), what a variable is declared as
// ('var_info'), where 'compile' writes a value ('destination'), what a
// caller asks of the address of an identifier ('lea_request'), why a name
// cannot be written ('read_only_cause'), an address computed at an inlined
// call ('call_address'), a value known at compile time ('folded_value'),
// where a variable lives ('var_placement') and how a value is used
// ('value_use')

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

#include "access_path.hpp"
#include "constant_arith.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"

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

// where a variable lives: in storage at an offset from a base register, at
// the memory a register points to, or in a register without storage
struct in_storage {
    // from the base register, set when the storage is allocated
    int64_t offset{};
    // the base of the frame that holds the storage, empty for the base the
    // name is resolved with, e.g. the variables base
    std::string_view base_register;
    // the storage holds the address of the variable, e.g. a parameter of a
    // noinline body
    bool holds_address{};
};

// e.g. the element 'e' of a 'foo' or a parameter whose address the call
// passes in a register
struct at_address_register {
    operand reg;
};

// e.g. the counter 'i' of a 'foo'
struct in_value_register {
    operand reg;
};

using var_placement =
    std::variant<in_storage, at_address_register, in_value_register>;

// the storage of the variable holds its address, see 'in_storage'
[[nodiscard]] inline auto holds_address(const var_placement& placement)
    -> bool {

    const in_storage* const stored{std::get_if<in_storage>(&placement)};
    return stored != nullptr and stored->holds_address;
}

// the register of a variable in a register or at one, none in storage
[[nodiscard]] inline auto register_of(const var_placement& placement)
    -> std::optional<operand> {

    if (const at_address_register* const pointed{
            std::get_if<at_address_register>(&placement),
        }) {

        return pointed->reg;
    }

    if (const in_value_register* const in_register{
            std::get_if<in_value_register>(&placement),
        }) {

        return in_register->reg;
    }

    return std::nullopt;
}

// the name of a variable the compiler adds, e.g. 'call-arg-12-0', it has a
// '-' so no identifier of the source can have it; the offset of the token
// tells the temporaries of calls in one block apart
[[nodiscard]] inline auto temporary_name(const token& src_loc_tk,
                                         const std::string_view role,
                                         const size_t index) -> std::string {

    return std::format("{}-{}-{}", role, src_loc_tk.start_index(), index);
}

// a name that 'temporary_name' made
[[nodiscard]] inline auto is_temporary_name(const std::string_view name)
    -> bool {

    return name.contains('-');
}

struct var_info {
    std::string name;
    const type* type_ptr{};
    token src_loc_tk;
    array_shape shape;
    read_only_cause read_only_why{};
    var_placement placement;

    // the array that the element 'e' of a 'foo' belongs to, empty unless this
    // is that element
    std::optional<storage_path> foo_array;

    // a variable the compiler adds, see 'temporary_name'
    [[nodiscard]] auto is_temporary() const -> bool {
        return is_temporary_name(name);
    }
};

// the address of an element of a path computed at an inlined call, e.g.
// '[rbp + r15 * 8 + 16]' for 'rooms[i]' of the argument 'world.rooms[i]';
// 'trailing_count' is the elements of the path after it, e.g. 1 for the 'x'
// of 'p.x' when 'p' names 'world.rooms[i]'
struct call_address {
    operand address;
    size_t trailing_count{};
};

// what a name resolves to: nothing, a constant, a register or storage; the
// path has one entry per element from the root with its type, e.g. 'p' and
// 'x' for 'p.x'; what else is known depends on the kind and is in its part,
// the accessors give what a kind does not have as empty, e.g. 'is_array' of a
// constant is false
class ident_info final {
    // a variable or the storage that an argument names
    struct var_part {
        ::operand operand;
        // the deepest address an inlined call computed, empty when none did
        std::optional<call_address> address_by_call;
        array_shape shape;
        bool holds_address{};
        read_only_cause read_only_why{};
    };

    // a variable or an argument whose value is held in a register
    struct register_part {
        ::operand reg;
    };

    std::string id_;
    // where the name is in the source, set by 'make_ident_info(tc, st)',
    // empty when resolved without a statement
    token src_loc_tk_;
    std::vector<std::string> elem_path_;
    std::vector<const type*> type_path_;
    // a constant: a literal, a named constant or, typed, the value of an
    // argument of an inlined call, which has the type of its parameter
    std::variant<std::monostate, folded_value, register_part, var_part> part_;

  public:
    [[nodiscard]] auto address_by_call() const -> std::optional<call_address> {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var == nullptr ? std::nullopt : var->address_by_call;
    }

    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        return shape().length();
    }

    // one element of the array a variable names, e.g. 'ps' of 'ps[1]' as the
    // element: the same storage without the shape; another kind is unchanged
    [[nodiscard]] auto as_element() const -> ident_info {
        ident_info element{*this};

        if (var_part* const var{std::get_if<var_part>(&element.part_)}) {
            var->shape = {};
        }

        return element;
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
        return shape().element_count();
    }

    [[nodiscard]] auto has_address_by_call() const -> bool {
        return address_by_call().has_value();
    }

    [[nodiscard]] auto holds_address() const -> bool {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var != nullptr and var->holds_address;
    }

    // e.g. 'p.x', the name as resolved
    [[nodiscard]] auto id() const -> const std::string& { return id_; }

    // the root holds an address, or is an argument whose address an inlined
    // call computed at run time
    [[nodiscard]] auto is_address_computed() const -> bool {
        return holds_address() or has_address_by_call();
    }

    [[nodiscard]] auto is_array() const -> bool { return shape().is_array(); }

    [[nodiscard]] auto is_const() const -> bool {
        return std::holds_alternative<folded_value>(part_);
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return std::holds_alternative<std::monostate>(part_);
    }

    // see 'array_shape::is_length_unknown'
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return shape().is_length_unknown();
    }

    [[nodiscard]] auto is_read_only() const -> bool {
        return read_only_why() != read_only_cause::none;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return std::holds_alternative<register_part>(part_);
    }

    [[nodiscard]] auto is_var() const -> bool {
        return std::holds_alternative<var_part>(part_);
    }

    // the register that holds the value, or the storage operand, e.g.
    // '[rbp - 8]'
    [[nodiscard]] auto operand() const -> const ::operand& {
        if (const register_part* const reg{
                std::get_if<register_part>(&part_),
            }) {

            return reg->reg;
        }

        return std::get<var_part>(part_).operand;
    }

    [[nodiscard]] auto read_only_why() const -> read_only_cause {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var == nullptr ? read_only_cause::none : var->read_only_why;
    }

    // variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return root_of(id_);
    }

    auto set_src_loc_tk(const token& tk) -> void { src_loc_tk_ = tk; }

    // not an array when the name has no storage, e.g. a constant
    [[nodiscard]] auto shape() const -> array_shape {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var == nullptr ? array_shape{} : var->shape;
    }

    // where the name is in the source, set by 'make_ident_info(tc, st)',
    // empty when resolved without a statement
    [[nodiscard]] auto src_loc_tk() const -> const token& {
        return src_loc_tk_;
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
                   reg.is_register() and not reg.register_name().empty() and
                   reg.index_register().empty() and reg.displacement() == 0;
        }

        return paths_match and is_var();
    }

    // the bytes of the value, e.g. the whole array, not an address
    [[nodiscard]] auto value_size_bytes(const token& src_loc_tk) const
        -> size_t {

        return shape().value_size_bytes(src_loc_tk, type_ref());
    }

    //
    // statics
    //

    [[nodiscard]] static auto make_const(const std::string_view ident,
                                         const type& tp,
                                         const folded_value& constant)
        -> ident_info {

        assert(not ident.empty());

        // note: a typed constant has the type of its parameter
        assert(not constant.is_typed() or constant.own_type == &tp);

        ident_info info;
        info.id_ = ident;
        info.elem_path_ = {std::string{ident}};
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
        info.elem_path_ = {std::string{ident}};
        info.type_path_ = {&reg.type_ref()};

        info.part_ = register_part{
            .reg{reg},
        };

        return info;
    }

    [[nodiscard]] static auto make_var(std::string ident,
                                       std::vector<std::string> elem_path,
                                       std::vector<const type*> type_path,
                                       var_part var) -> ident_info {

        assert(not ident.empty());
        assert(not elem_path.empty());
        assert(elem_path.size() == type_path.size());

        ident_info info;
        info.id_ = std::move(ident);
        info.elem_path_ = std::move(elem_path);
        info.type_path_ = std::move(type_path);
        info.part_ = std::move(var);

        return info;
    }

    // 'p' of 'p.x.y', the variable name without the field path
    [[nodiscard]] static auto root_of(const std::string_view path)
        -> std::string_view {

        return path.substr(0, path.find('.'));
    }
};

// where a value is used: the width it is computed at and whether its
// arithmetic is checked for overflow; made by 'toc::use_of' from a
// destination and by 'toc::use_at' for a value without one
struct value_use {
    const type* width{};
    bool checks_overflow{};

    [[nodiscard]] auto width_ref() const -> const type& {
        assert(width != nullptr);

        return *width;
    }
};

// where 'compile' writes a value: nowhere for a statement, a register, or the
// storage that a name of the source resolves to, e.g. 'p.x'; it is not changed
// after it is made, a field or an element of it is a destination of its own
// note: the kind follows from the members: storage when 'storage_' is set, a
//       register when the operand is one, nowhere without a type

class destination final {
    ::operand operand_;
    const type* type_ptr_{};
    // the path without indexes, e.g. 'ps.x' of 'ps[i].x', the root and the
    // binding of a result name it; the register of a register destination
    std::string name_;
    // the destination as written, for the comments and the diagnostics, e.g.
    // 'w.ps[0]'; a temporary has its name, a register its register
    std::string text_;
    // where the name is in the source, empty when it is not
    token src_loc_tk_;
    // the operand is the address, the name need not be resolved again, e.g.
    // an indexed element or an address loaded from a slot
    bool is_final_address_{};
    // an unsized one gets its length from the argument
    array_shape shape_;
    // its arithmetic wraps on purpose, see 'as_wrapping'; otherwise the
    // overflow check is as '--checks' asks
    bool wraps_{};
    // the root variable and the path of it that this names, set exactly for
    // storage
    std::optional<storage_path> storage_;

    // what a resolved name adds when it is the destination of an assignment
    struct name_parts {
        ::operand address;
        bool is_final_address{};
        std::optional<storage_path> storage;
        std::string text;
    };

  public:
    destination() = default;

    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        return shape_.length();
    }

    // the argument of a conversion, whose arithmetic wraps, e.g. the '+' of
    // 'i8(a + b)'; a call in it builds destinations of its own, so its
    // arguments and body are checked as anywhere else
    [[nodiscard]] auto as_wrapping() const -> destination {
        destination result{*this};
        result.wraps_ = true;
        return result;
    }

    // the element 'index' of the array, e.g. of an array literal; the type
    // of an array destination is its element type
    [[nodiscard]] auto element(const size_t index) const -> destination {
        // note: an array is in storage
        assert(storage_);

        destination result{*this};

        result.operand_.increment_offset(
            address_offset(index * type_ref().size_bytes()));

        result.is_final_address_ = true;
        result.shape_ = {};
        result.storage_->path.add_fixed_index(index);
        result.text_ += std::format("[{}]", index);

        return result;
    }

    // the name for an error about it, 'fallback' without a place in the source
    [[nodiscard]] auto error_token(const token& fallback) const -> token {
        return src_loc_tk_.is_empty() ? fallback : src_loc_tk_;
    }

    // the field 'fld', as the comments name it; the source names no field of
    // a literal, so an error about it is at its value, e.g. the 't()' of
    // 'rec{2, t()}'
    [[nodiscard]] auto field(const type_field& fld) const -> destination {
        // note: a record is in storage
        assert(storage_);

        destination result{*this};
        result.operand_ = operand_at(fld.offset, fld.type());
        result.is_final_address_ = true;
        result.name_ += std::format(".{}", fld.name);
        result.type_ptr_ = &fld.type();
        result.src_loc_tk_ = {};
        result.storage_->path.add_field(fld.offset);
        result.text_ += std::format(".{}", fld.name);

        return result;
    }

    // a scratch register that holds the value on its way to this
    // destination, its arithmetic is checked alike
    [[nodiscard]] auto in_register(const ::operand& reg) const -> destination {
        destination result{of_register(reg)};
        result.wraps_ = wraps_;
        return result;
    }

    [[nodiscard]] auto is_array() const -> bool { return shape_.is_array(); }

    [[nodiscard]] auto is_empty() const -> bool { return type_ptr_ == nullptr; }

    [[nodiscard]] auto is_final_address() const -> bool {
        return is_final_address_;
    }

    // see 'array_shape::is_length_unknown'
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return shape_.is_length_unknown();
    }

    [[nodiscard]] auto is_register() const -> bool {
        return not storage_ and operand_.is_register();
    }

    // storage named in the source, a variable or a temporary
    [[nodiscard]] auto is_storage() const -> bool {
        return storage_.has_value();
    }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    [[nodiscard]] auto operand() const -> const ::operand& { return operand_; }

    // 'offset' bytes into the destination as 'value_type', e.g. the padding
    // after a field, which no path names
    [[nodiscard]] auto operand_at(const size_t offset,
                                  const type& value_type) const -> ::operand {

        ::operand result{::operand::mem(operand_, value_type)};
        result.increment_offset(address_offset(offset));
        return result;
    }

    // the variable name without the field path
    [[nodiscard]] auto root_id() const -> std::string_view {
        return ident_info::root_of(name_);
    }

    [[nodiscard]] auto storage() const -> const std::optional<storage_path>& {
        return storage_;
    }

    [[nodiscard]] auto text() const -> const std::string& { return text_; }

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

    // a name resolved as 'info' with its 'storage', at the operand of 'info'
    [[nodiscard]] static auto of_name(const ident_info& info,
                                      std::optional<storage_path> storage)
        -> destination {

        return of_name(info, {
                                 .address{info.operand()},
                                 .is_final_address{info.has_address_by_call()},
                                 .storage{std::move(storage)},
                                 .text{info.id()},
                             });
    }

    // a name resolved as 'info', e.g. the destination of an assignment
    [[nodiscard]] static auto of_name(const ident_info& info, name_parts parts)
        -> destination {

        // note: a constant is rejected as a destination before
        assert(info.is_var() or info.is_register());
        assert(parts.storage.has_value() == info.is_var());

        destination result;
        result.operand_ = std::move(parts.address);
        result.type_ptr_ = &info.type_ref();
        result.name_ = info.id();
        result.text_ = std::move(parts.text);
        result.src_loc_tk_ = info.src_loc_tk();

        result.is_final_address_ = parts.is_final_address;

        result.shape_ = info.shape();
        result.storage_ = std::move(parts.storage);

        return result;
    }

    // e.g. a scratch register that an expression is computed in
    [[nodiscard]] static auto of_register(const ::operand& reg) -> destination {
        assert(reg.is_register());

        destination result;
        result.operand_ = reg;
        result.type_ptr_ = &reg.type_ref();
        result.name_ = reg.register_name();
        result.text_ = reg.register_name();

        return result;
    }
};

// what a caller asks of the address of an identifier
struct lea_request {
    // the count of a span operation on an array, none without one
    std::optional<operand> reg_count;

    // the deepest address of the path an inlined call computed
    std::optional<call_address> address_by_call;

    // the register the caller prefers the address to start in, none without
    // a preference
    std::optional<operand> address_register;
};
