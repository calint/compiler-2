#pragma once

// what a name resolves to ('ident_info'), what a variable is declared as
// ('var_info'), where 'compile' writes a value ('destination'), what a
// caller asks of the address of an identifier ('lea_request'), why a name
// cannot be written ('read_only_cause'), a value known at compile time
// ('folded_value'), where a variable lives ('var_placement') and how a value
// is used ('value_use')

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

// what a name resolves to: nothing, a constant, a register or storage, with
// the type of the last element of its path, e.g. that of 'x' for 'p.x'; what
// else is known depends on the kind and is in its part, the accessors give
// what a kind does not have as empty, e.g. 'is_array' of a constant is false
class ident_info final {
    // a variable or the storage that an argument names
    struct var_part {
        ::operand operand;
        // the operand is an address an inlined call computed, e.g.
        // '[rbp + r15 * 8 + 16]' for the argument 'rooms[i]'
        bool is_address_by_call{};
        array_shape shape;
        bool holds_address{};
        read_only_cause read_only_why{};
    };

    // a variable or an argument whose value is held in a register
    struct register_part {
        ::operand reg;
    };

    const type* type_ptr_{};
    // a constant: a literal, a named constant or, typed, the value of an
    // argument of an inlined call, which has the type of its parameter
    std::variant<std::monostate, folded_value, register_part, var_part> part_;

  public:
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

    // the same storage at 'address', which an inlined call computed for an
    // argument or a destination
    [[nodiscard]] auto at_address(const ::operand& address) const
        -> ident_info {

        ident_info result{*this};
        var_part& var{std::get<var_part>(result.part_)};
        var.operand = address;
        var.is_address_by_call = true;
        var.holds_address = false;
        return result;
    }

    [[nodiscard]] auto constant() const -> const folded_value& {
        return std::get<folded_value>(part_);
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
        result.type_ptr_ = fld.type_ptr;

        return result;
    }

    [[nodiscard]] auto holds_address() const -> bool {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var != nullptr and var->holds_address;
    }

    [[nodiscard]] auto is_address_by_call() const -> bool {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var != nullptr and var->is_address_by_call;
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

    // not an array when the name has no storage, e.g. a constant
    [[nodiscard]] auto shape() const -> array_shape {
        const var_part* const var{std::get_if<var_part>(&part_)};
        return var == nullptr ? array_shape{} : var->shape;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(validate_invariants());

        return *type_ptr_;
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
        info.type_ptr_ = &tp;
        info.part_ = constant;

        return info;
    }

    [[nodiscard]] static auto make_empty() -> ident_info { return {}; }

    [[nodiscard]] static auto make_register(const ::operand& reg)
        -> ident_info {

        assert(reg.is_register());

        ident_info info;
        info.type_ptr_ = &reg.type_ref();

        info.part_ = register_part{
            .reg{reg},
        };

        return info;
    }

    [[nodiscard]] static auto make_var(const type& tp, var_part var)
        -> ident_info {

        ident_info info;
        info.type_ptr_ = &tp;
        info.part_ = std::move(var);
        return info;
    }

  private:
    // an empty info has no type
    [[nodiscard]] auto validate_invariants() const -> bool {
        const bool is_typed{type_ptr_ != nullptr};

        if (is_register()) {
            const ::operand& reg{std::get<register_part>(part_).reg};

            return is_typed and reg.is_register() and
                   not reg.register_name().empty() and
                   reg.index_register().empty() and reg.displacement() == 0;
        }

        return is_typed and (is_const() or is_var());
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
// note: the kind follows from the members: storage when 'stored_' is set, a
//       register when the operand is one, nowhere else

class destination final {
    ::operand operand_;
    // the destination as written, for the comments and the diagnostics, e.g.
    // 'w.ps[0]'; a temporary has its name, a register its register
    std::string text_;
    // the operand is the address, the name need not be resolved again, e.g.
    // an indexed element or an address loaded from a slot
    bool is_final_address_{};
    // its arithmetic wraps on purpose, see 'as_wrapping'; otherwise the
    // overflow check is as '--checks' asks
    bool wraps_{};
    // a name, of the source or a call temporary, e.g. the result of an
    // inlined call bound to a register; a register the compiler allocated has
    // none
    bool is_named_{};

    // what a name in storage resolved to and the root variable and the path
    // of it that it names, both stepped with 'field' and 'element'
    struct stored_name {
        ident_info info;
        storage_path storage{};
    };

    // none for a register or nowhere
    std::optional<stored_name> stored_;

    // what a resolved name adds when it is the destination of an assignment
    struct name_parts {
        ::operand address;
        bool is_final_address{};
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
    [[nodiscard]] auto bound_info() const -> ident_info {
        // note: a register destination is bound as a register
        assert(stored_);

        if (is_final_address_) {
            return stored_->info.at_address(operand_);
        }

        return stored_->info;
    }

    // the element 'index' of the array, e.g. of an array literal; the type
    // of an array destination is its element type
    [[nodiscard]] auto element(const size_t index) const -> destination {
        // note: an array is in storage
        assert(stored_);

        destination result{*this};

        result.operand_.increment_offset(
            address_offset(index * type_ref().size_bytes()));

        result.is_final_address_ = true;
        result.stored_->info = stored_->info.as_element();
        result.stored_->storage.path.add_fixed_index(index);
        result.text_ += std::format("[{}]", index);

        return result;
    }

    // the field 'fld', as the comments name it
    [[nodiscard]] auto field(const type_field& fld) const -> destination {
        // note: a record is in storage
        assert(stored_);

        destination result{*this};
        result.operand_ = operand_at(fld.offset, fld.type_ref());
        result.is_final_address_ = true;
        result.stored_->info = stored_->info.field(fld);
        result.stored_->storage.path.add_field(fld.offset);
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
        return not stored_ and not operand_.is_register();
    }

    [[nodiscard]] auto is_final_address() const -> bool {
        return is_final_address_;
    }

    // see 'array_shape::is_length_unknown'
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return shape().is_length_unknown();
    }

    [[nodiscard]] auto is_named() const -> bool { return is_named_; }

    [[nodiscard]] auto is_register() const -> bool {
        return not stored_ and operand_.is_register();
    }

    [[nodiscard]] auto operand() const -> const ::operand& { return operand_; }

    // 'offset' bytes into the destination as 'value_type', e.g. the padding
    // after a field, which no path names
    [[nodiscard]] auto operand_at(const size_t offset,
                                  const type& value_type) const -> ::operand {

        ::operand result{::operand::mem(operand_, value_type)};
        result.increment_offset(address_offset(offset));
        return result;
    }

    // an unsized one gets its length from the argument
    [[nodiscard]] auto shape() const -> array_shape {
        return stored_ ? stored_->info.shape() : array_shape{};
    }

    [[nodiscard]] auto storage() const -> std::optional<storage_path> {
        if (not stored_) {
            return std::nullopt;
        }

        return stored_->storage;
    }

    [[nodiscard]] auto text() const -> const std::string& { return text_; }

    [[nodiscard]] auto type_ref() const -> const type& {
        if (stored_) {
            return stored_->info.type_ref();
        }

        // note: nowhere has no type
        assert(operand_.is_register());

        return operand_.type_ref();
    }

    [[nodiscard]] auto wraps() const -> bool { return wraps_; }

    //
    // statics
    //

    // a name resolved as 'info' with its 'storage', at the operand of 'info',
    // e.g. a temporary
    [[nodiscard]] static auto of_name(const ident_info& info,
                                      const std::string_view name,
                                      std::optional<storage_path> storage)
        -> destination {

        return of_name(info, {
                                 .address{info.operand()},
                                 .is_final_address{info.is_address_by_call()},
                                 .storage{std::move(storage)},
                                 .text{std::string{name}},
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
        result.text_ = std::move(parts.text);

        result.is_final_address_ = parts.is_final_address;
        result.is_named_ = true;

        if (parts.storage) {
            result.stored_ = {
                .info{info},
                .storage{std::move(*parts.storage)},
            };
        }

        return result;
    }

    // e.g. a scratch register that an expression is computed in
    [[nodiscard]] static auto of_register(const ::operand& reg) -> destination {
        assert(reg.is_register());

        destination result;
        result.operand_ = reg;
        result.text_ = reg.register_name();

        return result;
    }
};

// what a caller asks of the address of an identifier
struct lea_request {
    // the path starts a range of at least this many elements, its last index
    // may be the end less them; none for an element
    std::optional<size_t> range_count;

    // a run-time start of the range is copied here, the caller checks it
    // against a count computed after the address
    std::optional<operand> start_register;

    // the register the caller prefers the address to start in, none without
    // a preference
    std::optional<operand> address_register;
};
