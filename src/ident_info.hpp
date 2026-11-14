#pragma once

// what names and values are, the types below in reading order:
//
//   'read_only_cause'  why a name cannot be written
//   'folded_value'     a value known at compile time
//   'in_storage', 'at_address_register', 'in_value_register',
//   'var_placement'    where a variable lives
//   'var_info'         what a variable is declared as
//   'var_address'      what the operand of a variable is
//   'ident_info'       what a name resolves to
//   'value_use'        the width a value is used at
//   'destination'      where 'compile' writes a value
//
// a declaration makes a 'var_info', a name resolves to an 'ident_info', a
// node writes to a 'destination' (README 1.3)

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

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
// call, and a 'let' name of either; untyped: a literal or a 'let' name of
// one, checked by its value
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

// e.g. a global or a local 'var', in storage at an offset from a base
// register
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

// where a variable lives: in storage at an offset from a base register, at
// the memory a register points to, or in a register without storage
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

// what a variable is declared as: its name, type and array shape, the
// token of its declaration, why it is read-only and where it lives
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

// what the operand of a variable is
enum class var_address : uint8_t {
    // the storage of the variable
    storage,
    // the storage that holds the address of the variable, see 'in_storage'
    held,
    // an address an inlined call computed, e.g. '[rbp + r15 * 8 + 16]' for
    // the argument 'rooms[i]'
    by_call,
};

// what a name resolves to: a constant, a register or storage, with the type
// of the last element of its path, e.g. that of 'x' for 'p.x', which a
// register or storage has as the type of its operand; what else is known
// depends on the kind and is in its part, the accessors give what a kind does
// not have as empty, e.g. 'is_array' of a constant is false; a default one is
// not resolved yet
class ident_info final {
    // a constant and the type it is read as, e.g. a literal of the default type
    struct const_part {
        folded_value value;
        const type* type_ptr{};
    };

    // a variable or the storage that an argument names
    struct var_part {
        ::operand operand;
        var_address address{};
        array_shape shape;
        read_only_cause read_only_why{};
    };

    // a variable or an argument whose value is held in a register
    struct register_part {
        ::operand reg;
    };

    std::variant<std::monostate, const_part, register_part, var_part> part_;

  public:
    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        return shape().length();
    }

    // one element of the array a variable names, e.g. 'ps' of 'ps[1]' as the
    // element: the same storage without the shape
    [[nodiscard]] auto as_element() const -> ident_info {
        ident_info element{*this};

        // note: only a variable is an array
        std::get<var_part>(element.part_).shape = {};

        return element;
    }

    // the same storage at 'address', which an inlined call computed for an
    // argument or a destination
    [[nodiscard]] auto at_address(const ::operand& address) const
        -> ident_info {

        ident_info result{*this};
        var_part& var{std::get<var_part>(result.part_)};
        var.operand = address;
        var.address = var_address::by_call;
        return result;
    }

    [[nodiscard]] auto constant() const -> const folded_value& {
        return std::get<const_part>(part_).value;
    }

    // 1 for a name that is not an array, see 'discarded_array_len'
    [[nodiscard]] auto element_count() const -> size_t {
        return shape().element_count();
    }

    // the field 'fld' of the instance a variable names, its operand moved by
    // the offset of the field; a constant or a register has no fields
    [[nodiscard]] auto field(const type_field& fld) const -> ident_info {
        ident_info result{*this};
        var_part& var{std::get<var_part>(result.part_)};

        var.operand = ::operand::mem(var.operand, fld.type_ref());

        var.operand.increment_offset(address_offset(fld.offset));
        var.shape = fld.shape;

        return result;
    }

    [[nodiscard]] auto holds_address() const -> bool {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var != nullptr and var->address == var_address::held;
    }

    [[nodiscard]] auto is_address_by_call() const -> bool {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var != nullptr and var->address == var_address::by_call;
    }

    [[nodiscard]] auto is_array() const -> bool { return shape().is_array(); }

    [[nodiscard]] auto is_const() const -> bool {
        return std::holds_alternative<const_part>(part_);
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
    // '[rbp + 8]'
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

    // not an array when the name has no storage, e.g. a constant
    [[nodiscard]] auto shape() const -> array_shape {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var == nullptr ? array_shape{} : var->shape;
    }

    // note: a default info has no type
    [[nodiscard]] auto type_ref() const -> const type& {
        if (const const_part* const constant{std::get_if<const_part>(&part_)}) {
            return *constant->type_ptr;
        }

        return operand().type_ref();
    }

    // the bytes of the value, e.g. the whole array, not an address
    [[nodiscard]] auto value_size_bytes(const token& src_loc_tk) const
        -> size_t {

        return shape().value_size_bytes(src_loc_tk, type_ref());
    }

    //
    // statics
    //

    [[nodiscard]] static auto make_const(const type& tp,
                                         const folded_value& constant)
        -> ident_info {

        // note: a typed constant has its own type, e.g. that of its parameter
        assert(not constant.is_typed() or constant.own_type == &tp);

        ident_info info;

        info.part_ = const_part{
            .value{constant},
            .type_ptr{&tp},
        };

        return info;
    }

    [[nodiscard]] static auto make_register(const ::operand& reg)
        -> ident_info {

        assert(reg.is_register());

        ident_info info;

        info.part_ = register_part{
            .reg{reg},
        };

        return info;
    }

    // note: the type of a variable is that of its operand
    [[nodiscard]] static auto make_var(var_part var) -> ident_info {
        ident_info info;
        info.part_ = std::move(var);
        return info;
    }
};

// where a value is used: the width it is computed at and whether its
// arithmetic is checked for overflow; made by 'compile_toc::use_of' from a
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
class destination final {
    // e.g. a scratch register that an expression is computed in
    struct held_register {
        ::operand reg;
        // a name, of the source or a call temporary, e.g. the result of an
        // inlined call bound to a register; a register the compiler allocated
        // has none
        bool is_named{};
    };

    // what a name in storage resolved to and the root variable and the path
    // of it that it names, all stepped with 'field' and 'element'
    struct stored_name {
        // its operand is the address written; an address computed for it,
        // e.g. of an indexed element or loaded from a slot, is final
        // ('var_address::by_call'), the name need not be resolved again
        ident_info info;
        storage_path storage{};
    };

    // nowhere for a statement
    std::variant<std::monostate, held_register, stored_name> place_;
    // the destination as written, for the comments and the diagnostics, e.g.
    // 'w.ps[0]'; a temporary has its name, a register its register
    std::string text_;
    // its arithmetic wraps on purpose, see 'as_wrapping'; otherwise the
    // overflow check is as '--checks' asks
    bool wraps_{};

    // what a resolved name adds as a destination, see 'of_name'
    struct name_parts {
        std::optional<storage_path> storage;
        std::string text;
    };

  public:
    [[nodiscard]] auto array_len() const -> std::optional<size_t> {
        return shape().length();
    }

    // the argument of a conversion, whose arithmetic wraps, e.g. the '+' of
    // 'i8(a + b)'; a call in it builds destinations of its own, so its
    // arguments and body are checked as anywhere else
    [[nodiscard]] auto as_wrapping() const -> destination {
        destination result{*this};
        result.wraps_ = true;
        return result;
    }

    // what the result of an inlined call stands for when this is its
    // destination: the storage at the address computed for it, or as resolved
    [[nodiscard]] auto bound_info() const -> const ident_info& {
        // note: a register destination is bound as a register
        return stored().info;
    }

    // the element 'index' of the array, e.g. of an array literal; the type
    // of an array destination is its element type
    [[nodiscard]] auto element(const size_t index) const -> destination {
        // note: an array is in storage
        stored_name name{stored()};

        ::operand address{name.info.operand()};

        address.increment_offset(
            address_offset(index * type_ref().size_bytes()));

        name.info = name.info.as_element().at_address(address);
        name.storage.path.add_fixed_index(index);

        destination result{*this};
        result.place_ = std::move(name);
        result.text_ += std::format("[{}]", index);

        return result;
    }

    // the field 'fld', as the comments name it
    [[nodiscard]] auto field(const type_field& fld) const -> destination {
        // note: an instance is in storage
        stored_name name{stored()};
        const ::operand address{operand_at(fld.offset, fld.type_ref())};
        name.info = name.info.field(fld).at_address(address);
        name.storage.path.add_field(fld.offset);

        destination result{*this};
        result.place_ = std::move(name);
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

    [[nodiscard]] auto is_array() const -> bool { return shape().is_array(); }

    [[nodiscard]] auto is_empty() const -> bool {
        return std::holds_alternative<std::monostate>(place_);
    }

    // a register that holds a name, see 'held_register'; a stored
    // destination is always named
    [[nodiscard]] auto is_named_register() const -> bool {
        const held_register* const held{std::get_if<held_register>(&place_)};
        return held != nullptr and held->is_named;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return std::holds_alternative<held_register>(place_);
    }

    // the register or the address of the storage
    [[nodiscard]] auto operand() const -> const ::operand& {
        if (const held_register* const held{
                std::get_if<held_register>(&place_),
            }) {

            return held->reg;
        }

        return stored().info.operand();
    }

    // 'offset' bytes into the destination as 'value_type', e.g. the padding
    // after a field, which no path names
    [[nodiscard]] auto operand_at(const size_t offset,
                                  const type& value_type) const -> ::operand {

        ::operand result{::operand::mem(operand(), value_type)};
        result.increment_offset(address_offset(offset));
        return result;
    }

    // an unsized one gets its length from the argument
    [[nodiscard]] auto shape() const -> array_shape {
        const stored_name* const name{std::get_if<stored_name>(&place_)};
        return name != nullptr ? name->info.shape() : array_shape{};
    }

    [[nodiscard]] auto storage() const -> std::optional<storage_path> {
        const stored_name* const name{std::get_if<stored_name>(&place_)};

        if (name == nullptr) {
            return std::nullopt;
        }

        return name->storage;
    }

    [[nodiscard]] auto text() const -> const std::string& { return text_; }

    [[nodiscard]] auto type_ref() const -> const type& {
        if (const held_register* const held{
                std::get_if<held_register>(&place_),
            }) {

            return held->reg.type_ref();
        }

        // note: nowhere has no type
        return stored().info.type_ref();
    }

    [[nodiscard]] auto wraps() const -> bool { return wraps_; }

    //
    // statics
    //

    // a name resolved as 'info' at the address written, e.g. the destination
    // of an assignment or a call temporary
    [[nodiscard]] static auto of_name(const ident_info& info, name_parts parts)
        -> destination {

        // note: a constant is rejected as a destination before
        assert(info.is_var() or info.is_register());
        assert(parts.storage.has_value() == info.is_var());

        destination result;

        if (parts.storage) {
            // note: an address held in a slot is loaded before it is written
            assert(info.operand().is_memory() and not info.holds_address());

            result.place_ = stored_name{
                .info{info},
                .storage{std::move(*parts.storage)},
            };
        } else {
            result.place_ = held_register{
                .reg{info.operand()},
                .is_named{true},
            };
        }

        result.text_ = std::move(parts.text);

        return result;
    }

    // e.g. a scratch register that an expression is computed in
    [[nodiscard]] static auto of_register(const ::operand& reg) -> destination {
        assert(reg.is_register());

        destination result;

        result.place_ = held_register{
            .reg{reg},
            .is_named{},
        };

        result.text_ = reg.register_name();

        return result;
    }

  private:
    [[nodiscard]] auto stored() const -> const stored_name& {
        const stored_name* const name{std::get_if<stored_name>(&place_)};

        assert(name != nullptr);

        return *name;
    }
};
