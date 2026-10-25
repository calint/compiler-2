#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// a constant index out of bounds is always a bug; an unsized array parameter
// without a length rejects only a negative index; 'allow_end' accepts the end,
// where a range of a zero count starts
inline auto assert_constant_index(const ident_info& array_info,
                                  const token& index_tk,
                                  const std::string_view array_name,
                                  const int64_t index, const bool allow_end)
    -> void {

    const std::optional<size_t> array_len{array_info.array_len()};

    if (not array_len and index >= 0) {
        return;
    }

    if (not array_len) {
        throw compile_error{index_tk,
                            std::format("index {} is negative", index)};
    }

    const bool is_past_end{
        allow_end ? std::cmp_greater(index, *array_len)
                  : std::cmp_greater_equal(index, *array_len),
    };

    if (index >= 0 and not is_past_end) {
        return;
    }

    throw compile_error{
        index_tk,
        std::format("index {} is out of bounds for array {} of size {}", index,
                    in_quotes(array_name), *array_len)};
}

// the constant range of a bulk operation, 'count' elements from 'start', e.g.
// 'array_copy(a[1], b, 5)' or 'write(1, a, 5, 1)'
struct constant_range {
    // empty when computed at run time, then a count is known to pass the end
    // only when it is larger than the array
    std::optional<int64_t> start;
    int64_t count{};
    // the written start, empty when the range starts at the array
    std::optional<token> start_tk;
    // where an error about the count points
    token count_tk;
    // e.g. 'count 5' or 'a sector of 4 elements'
    std::string count_text;
    // the path of the array, e.g. 'a.b' of 'a.b[1]'
    std::string_view array_name;
};

// where the range of a bulk operation is checked: a constant count with a
// constant start was checked at compile time, e.g. 'array_copy(a[1], b, 2)'
enum class range_check : uint8_t { run_time, compile_time };

// a constant range is known to be negative or to pass the end of the array,
// e.g. 5 in 'array_copy(a, b, 5)' of 'a = i8[4]'; a run-time one is left to
// the bounds check at run time
inline auto assert_constant_range(const ident_info& array_info,
                                  const constant_range& range) -> void {

    if (range.count < 0) {
        throw compile_error{range.count_tk,
                            std::format("count {} is negative", range.count)};
    }

    const bool has_constant_start{range.start and range.start_tk};

    // a start at the end is the empty range of a zero count
    if (has_constant_start) {
        assert_constant_index(array_info, *range.start_tk, range.array_name,
                              *range.start, true);
    }

    const std::optional<size_t> array_len{array_info.array_len()};

    if (not array_len) {
        return;
    }

    // note: a start computed at run time that passes its bounds check is not
    //       negative, so at most the whole array remains

    const size_t remaining{
        range.start ? *array_len - static_cast<size_t>(*range.start)
                    : *array_len,
    };

    if (std::cmp_less_equal(range.count, remaining)) {
        return;
    }

    const std::string from_index{
        has_constant_start ? std::format(" from index {}", *range.start)
                           : std::string{},
    };

    throw compile_error{
        range.count_tk,
        std::format("{}{} is past the end of array {} of size {}",
                    range.count_text, from_index, in_quotes(range.array_name),
                    *array_len)};
}

class expr_any;
class stmt_identifier;

[[nodiscard]] inline auto make_ident_info(const toc& tc,
                                          const stmt_identifier& ident)
    -> ident_info;

[[nodiscard]] inline auto make_scalar_ident_info(const toc& tc,
                                                 const stmt_identifier& ident)
    -> ident_info;

// a path of names, fields and indexes, e.g. 'w.rooms[i].items[j].x', also the
// receiver of a method call, or a literal, e.g. '3' or 'true'; it is a place
// ('as_place') when it reaches storage: a variable, a field or an element; the
// parse resolves the type and the bytes of the root variable that the path
// reaches ('accessed_bytes_', the whole array after a run-time index), the
// compile resolves the names again in the current scopes and builds one memory
// operand ('compile_lea', see 'etc/docs/README.md' chapter 6); a path that
// names a constant has no storage; the members that use
// 'expr_any' (the constructor, the element parse, the index steps of the walk)
// are in 'stmt_identifier_impl.hpp'
class stmt_identifier final : public statement {
    // e.g. the '[i]' of 'rooms[i]'
    struct index_part {
        token open_bracket_tk;
        std::unique_ptr<expr_any> expr; // never null
        token close_bracket_tk;
    };

    struct ident_elem {
        token name_tk;
        // none without an index, e.g. 'name'
        std::optional<index_part> index;

        // the index of an indexed element
        [[nodiscard]] auto index_expr() const -> const expr_any& {
            if (not index) {
                std::unreachable();
            }

            return *index->expr;
        }

        auto source_to(std::ostream& os) const -> void;
    };

    // one element of a walk of the path from its root, see 'for_each_prefix'
    struct prefix_step {
        const ident_elem* elem{};
        // the path up to the element, e.g. 'w.rooms' of 'w.rooms[i].x'
        std::string_view path;
        // what 'path' resolves to
        const ident_info* info{};
        // the instance that holds the field, null for the root
        const type* parent_type{};
        // the offset of the field in 'parent_type', 0 for the root
        size_t field_offset{};
        bool is_last{};
    };

    // builds the memory operand of a path one element at a time, with the
    // scratch registers it allocates; the registers from 'owned_from_' on are
    // the path's own and may be overwritten, the earlier ones are the caller's
    class path_address final {
        std::reference_wrapper<toc> tc_;
        size_t indent_;
        token src_loc_tk_;
        std::reference_wrapper<machine::held_registers> held_;
        size_t owned_from_;
        // where the caller prefers the address, empty without a preference
        operand address_register_;
        // kept between the indexes of the path while it is not in the way
        operand index_register_;

      public:
        path_address(toc& tc, const size_t indent, const token& src_loc_tk,
                     machine::held_registers& held, operand address_register)
            : tc_{tc}, indent_{indent}, src_loc_tk_{src_loc_tk}, held_{held},
              owned_from_{held.registers().size()},
              address_register_{std::move(address_register)} {}

        // the range of an unindexed array is checked once, at the last element
        auto check_unindexed_range(const ident_info& cur_info,
                                   const bool is_last_elem,
                                   const operand& reg_count) const -> void {

            if (not cur_info.is_array() or reg_count.is_empty()) {
                return;
            }

            // note: an array before the last element is indexed
            assert(is_last_elem);

            check_array_bounds(tc_, indent_, src_loc_tk_,
                               machine::bounds_range::count_alone(
                                   reg_count, cur_info.element_count()));
        }

        // the address after the element index, a constant index goes into the
        // displacement and any other one into an index register
        [[nodiscard]] auto indexed_element(const ident_elem& cur_elem,
                                           std::string_view path,
                                           const ident_info& cur_info,
                                           const operand& range_count,
                                           const operand& address) -> operand;

        // the address a call computed, the address a slot holds or the
        // storage
        [[nodiscard]] auto start(const operand& known_operand,
                                 const ident_info& storage) -> operand {

            if (not known_operand.is_empty()) {
                return known_operand;
            }

            if (storage.holds_address()) {
                return load_held_address(storage.operand());
            }

            return storage.operand();
        }

      private:
        // adds the element index of 'cur_elem' to 'address', which has no
        // index yet, keeping its base and displacement
        [[nodiscard]] auto add_index(const ident_elem& cur_elem,
                                     const ident_info& cur_info,
                                     const operand& reg_count,
                                     const operand& address) -> operand;

        // computes 'address' into a register so another index can be added
        [[nodiscard]] auto fold_indexed_address(const operand& address)
            -> operand {

            machine& x{tc_.get().machine()};

            operand target{
                fold_register(address),
            };

            if (target.is_empty()) {
                target = x.alloc_scratch_register(
                    src_loc_tk_, indent_, tc_.get().builtins().address());

                held_.get().add(target);
            }

            x.address_of(src_loc_tk_, indent_, target, address);

            return target;
        }

        // picks a register that 'lea target, [address]' may overwrite, or
        // empty when a new one must be allocated
        // note: only 'address' refers to an owned register and the fold
        //       replaces 'address', so an owned base is dead once 'lea' has
        //       read it
        [[nodiscard]] auto fold_register(const operand& address) -> operand {
            // the caller loads the final address into it anyway, so it is free
            // until then and folding there may save the final move
            if (not address_register_.is_empty()) {
                return address_register_;
            }

            // an owned base is a loaded held address or an earlier fold
            // target, reusing it keeps 'index_register_' for the next index
            for (const operand& r : owned()) {
                // the frame register and the caller's registers are never
                // owned, so a match proves the base is private to this path
                if (r.register_name() == address.base_register()) {
                    return r;
                }
            }

            // the base must survive, so without an index register a new one is
            // needed
            if (index_register_.is_empty()) {
                return {};
            }

            // 'lea' may write the index register it reads, the next index then
            // gets its own register so it never overwrites the new base
            const operand folded_into{index_register_};
            index_register_ = {};

            return folded_into;
        }

        [[nodiscard]] auto load_held_address(const operand& address_slot)
            -> operand {

            toc& tc{tc_.get()};
            machine& x{tc.machine()};

            const operand address_register{
                x.alloc_scratch_register(src_loc_tk_, indent_,
                                         tc.builtins().address()),
            };

            held_.get().add(address_register);

            x.copy_value(src_loc_tk_, indent_, address_register,
                         operand::mem(address_slot, tc.builtins().address()));

            return operand::mem(address_register, tc.builtins().address());
        }

        [[nodiscard]] auto owned() const -> std::span<const operand> {
            return held_.get().registers().subspan(owned_from_);
        }
    };

    std::vector<ident_elem> elems_;
    std::vector<token> elem_delims_tk_;
    std::string dotted_path_;
    // of what the path names, not an array after an index
    array_shape shape_;

    // bytes of the root variable accessed by this path, after a run-time index
    // the whole array
    byte_range accessed_bytes_;

    // the fields and indexes of this path
    access_path access_path_;

    // e.g. '.add' in 'lst.add(x)', the path is the receiver
    struct method_tokens {
        token dot_tk;
        token name_tk;
    };

    // none when the path is not the receiver of a method call
    std::optional<method_tokens> method_;

    // e.g. the '.x' of 'p.x'
    struct path_field {
        token dot_tk;
        token name_tk;
    };

    struct path_end {};

    // what follows an element of the path, see 'read_continuation'
    using path_continuation = std::variant<path_end, path_field, method_tokens>;

  public:
    // a path of names, fields and indexes from 'tk', which is read
    stmt_identifier(toc& tc, token tk, tokenizer& tz);

    // note: 'expr_any' is incomplete here, the special members are defined in
    //       'stmt_identifier_impl.hpp', where a 'unique_ptr' of it is complete

    stmt_identifier();

    stmt_identifier(stmt_identifier&&) noexcept;

    stmt_identifier(const stmt_identifier&) = delete;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = delete;

    ~stmt_identifier() override;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        path_source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info src_info{make_scalar_ident_info(tc, *this)};

        // note: a list copies a constant itself
        assert(not src_info.is_const());

        machine::held_registers held{x, tok(), indent};

        const operand op{get_lea_operand(tc, indent, src_info, held)};

        x.copy_value(tok(), indent, dst.operand(), op);

        get_unary_ops().compile(x, indent, tok(), dst.operand(),
                                tc.overflow_options(dst));
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        // instances are copied whole and cannot narrow
        assert(dst_type.is_builtin());

        const ident_info info{make_ident_info(tc, *this)};

        if (info.is_const() and not info.constant().is_typed()) {
            assert_constant_fits(info, dst_type);
            return;
        }

        const type& src_type{info.type_ref()};

        // a built-in destination takes a number, e.g. not the instance 'a'
        // of 'y = a'
        if (not src_type.is_builtin()) {
            throw_type_mismatch(first_token(), dst_type.name(),
                                src_type.name());
        }

        if (src_type.size_bytes() <= dst_type.size_bytes()) {
            return;
        }

        throw_narrowed(first_token(), statement::trimmed_source(*this),
                       src_type, dst_type);
    }

    // a typed constant is a value of its type, so it wraps at the width of
    // its use like a variable, e.g. '-p' of an 'i8' 'p' bound to -128 is -128
    // at 'i8'
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        const ident_info info{make_ident_info(tc, *this)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        const folded_value& constant{info.constant()};

        const int64_t value{
            get_unary_ops().evaluate_at(constant.value, use.width_ref(),
                                        use.checks_overflow),
        };

        return folded_value{
            .value{constant_arith::wrap_to_width(value, use.width_ref())},
            .own_type{constant.own_type},
        };
    }

    // the low bytes are at the start address of the storage
    [[nodiscard]] auto
    keeps_low_bits([[maybe_unused]] const width_change change) const
        -> bool override {

        return true;
    }

    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        return this;
    }

    // a call in an index, e.g. the 'h(i)' of 'a[h(i)]'
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override;

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        // a path such as 'p.y' reads its root variable
        if (not var or first_token().is_text(*var)) {
            reader(first_token(), path_text(), accessed_bytes_, access_path_);
        }

        visit_index_reads(var, reader);
    }

    //
    // class methods
    //

    // the fields and indexes from the root variable to what this path names,
    // e.g. 'es[1].items[i]'
    [[nodiscard]] auto accessed_path() const -> access_path {
        return access_path_;
    }

    // the emitter of the address of this statement for a bulk operation,
    // 'info' is what 'toc' resolved it to
    // nothing is emitted here, the backend calls the returned emitter when it
    // needs the address: the emitter builds it with 'compile_address', which
    // passes it to the backend's 'use' while the scratch registers that built
    // it are still allocated and frees them afterwards. it refers to its
    // arguments, so it is only valid within the call that made it
    [[nodiscard]] auto
    address_emitter_of(toc& tc, const size_t indent, const token& src_loc_tk,
                       const ident_info& info,
                       const range_check check = range_check::run_time) const
        -> machine::address_emitter {

        return [this, &tc, indent, &src_loc_tk, &info, check](
                   const operand& reg_count, const operand& address_register,
                   const machine::address_use use) -> void {
            const operand range_count{
                check == range_check::run_time ? reg_count : operand{},
            };

            compile_address(tc, indent, src_loc_tk,
                            {
                                .reg_count{range_count},
                                .address_by_call{info.address_by_call()},
                                .address_register{address_register},
                            },
                            use);
        };
    }

    // an array or an element of one, where a bulk operation starts
    auto assert_array_or_element() const -> void {
        assert_not_method();

        if (shape_.is_array() or is_array_element()) {
            return;
        }

        throw compile_error{
            first_token(),
            std::format("{} is not an array", in_quotes(dotted_path()))};
    }

    // a constant count of a bulk operation that starts at this path, see
    // 'assert_constant_range'
    auto assert_count_in_bounds(const toc& tc, const token& count_tk,
                                const int64_t count) const -> void;

    // where only an array is expected, the path does not end at a method,
    // e.g. 'array_length(a.m)'
    auto assert_not_method() const -> void {
        if (not method_) {
            return;
        }

        throw compile_error{
            method_->name_tk,
            std::format("'{}.{}' is a method, expected an array", dotted_path_,
                        method_->name_tk.text())};
    }

    // the address of this place from 'compile_lea': 'use' runs while the
    // scratch registers that build it are still allocated, then they are
    // freed
    auto
    compile_address(toc& tc, const size_t indent, const token& src_loc_tk,
                    const lea_request& request,
                    const std::function_ref<void(const operand&)> use) const
        -> void {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        machine::held_registers held{x, src_loc_tk, indent};

        const operand address{compile_lea(tc, indent, tok(), held, request)};

        use(address);
    }

    // the memory operand of this identifier, e.g. 'rbp + 4 * r15 + 248',
    // 'held' receives the scratch registers of the indexing and
    // 'machine' says which scalings an operand can have
    [[nodiscard]] auto
    compile_lea(toc& tc, const size_t indent, const token& src_loc_tk,
                machine::held_registers& held, const lea_request& request) const
        -> operand {

        const call_address* const by_call{
            by_call_in_path(request.address_by_call),
        };

        path_address builder{tc, indent, src_loc_tk, held,
                             request.address_register};

        operand address;
        const type* value_type{};

        for_each_prefix(tc, src_loc_tk, [&](const prefix_step& step) -> void {
            const ident_info& cur_info{*step.info};

            if (step.parent_type == nullptr) {
                address = builder.start(by_call == nullptr ? operand{}
                                                           : by_call->address,
                                        cur_info);
            } else {
                // field offsets stay in the operand, not in the base register
                address.increment_offset(address_offset(step.field_offset));
            }

            value_type = &cur_info.type_ref();

            // an unindexed element only needs a possible range bounds check
            if (not step.elem->index) {
                builder.check_unindexed_range(cur_info, step.is_last,
                                              request.reg_count);

                return;
            }

            address = builder.indexed_element(
                *step.elem, step.path, cur_info,
                step.is_last ? request.reg_count : operand{}, address);
        });

        return operand::mem(address, *value_type);
    }

    // the value of this path where 'info' resolves it to a variable: its
    // storage or computed address, with unary ops a scratch register of
    // 'register_type' that holds the result; the registers go to 'held'
    [[nodiscard]] auto
    compile_value_operand(toc& tc, const size_t indent, const token& src_loc_tk,
                          const ident_info& info, const type& register_type,
                          const machine::runtime_check& overflow_check,
                          machine::held_registers& held) const -> operand {

        // note: a constant is folded by the caller, each by its own policy
        assert(not info.is_const());

        if (get_unary_ops().is_empty()) {
            return get_lea_operand(tc, indent, info, held);
        }

        const operand storage{get_lea_operand(tc, indent, info, held)};

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src_loc_tk, indent, register_type),
        };

        held.add(reg);
        x.copy_value(src_loc_tk, indent, reg, storage);
        get_unary_ops().compile(x, indent, src_loc_tk, reg, overflow_check);

        return reg;
    }

    // the index where a bulk operation starts, 0 for a whole array, empty when
    // computed at run time
    [[nodiscard]] auto constant_start_index(const toc& tc) const
        -> std::optional<int64_t>;

    // the text the resolver looks up, the path without indexes, e.g. 'b.items'
    // of 'b.items[i]', or a literal such as '3' or 'true', which resolve to
    // constants
    [[nodiscard]] auto dotted_path() const -> std::string_view {
        return dotted_path_;
    }

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    // the operand of what 'info' resolves this statement to, a computed
    // address goes into 'held'
    [[nodiscard]] auto get_lea_operand(toc& tc, const size_t indent,
                                       const ident_info& info,
                                       machine::held_registers& held) const
        -> operand {

        if (not has_computed_address(info)) {
            return info.operand();
        }

        return compile_lea(tc, indent, tok(), held,
                           {
                               .reg_count{},
                               .address_by_call{info.address_by_call()},
                               .address_register{},
                           });
    }

    // the address of what 'info' resolves this statement to is computed into
    // a register: an indexed access, an address held in a slot or an address
    // a call computed
    [[nodiscard]] auto has_computed_address(const ident_info& info) const
        -> bool {

        return is_indexed() or info.is_address_computed();
    }

    [[nodiscard]] auto is_array() const -> bool { return shape_.is_array(); }

    [[nodiscard]] auto is_array_element() const -> bool {
        return not elems_.empty() and elems_.back().index.has_value();
    }

    // false when a run-time index leaves the element unknown
    [[nodiscard]] auto is_exact_access() const -> bool {
        return not access_path_.has_run_time_index();
    }

    [[nodiscard]] auto is_indexed() const -> bool {
        return std::ranges::any_of(elems_, [](const ident_elem& e) -> bool {
            return e.index.has_value();
        });
    }

    [[nodiscard]] auto is_method_receiver() const -> bool {
        return method_.has_value();
    }

    // the name at the end of the path, e.g. 'data' of 'q1.data'
    [[nodiscard]] auto last_token() const -> const token& {
        return elems_.back().name_tk;
    }

    [[nodiscard]] auto method_dot_token() const -> const token& {
        assert(method_);

        return method_->dot_tk;
    }

    [[nodiscard]] auto method_name_token() const -> const token& {
        assert(method_);

        return method_->name_tk;
    }

    auto operator=(stmt_identifier&&) noexcept -> stmt_identifier&;

    auto path_source_to(std::ostream& os) const -> void;

    // the path as written, with indexes but without unary operators
    [[nodiscard]] auto path_text() const -> std::string {
        std::stringstream ss;
        path_source_to(ss);
        return statement::trimmed_source(ss.view());
    }

    // a run-time index does not prove which element is written
    auto record_assignment(assignment_flow& flow) const -> void {
        const std::optional<std::string_view> var{flow.traced_var()};

        if (not var or not first_token().is_text(*var) or
            not is_exact_access()) {

            return;
        }

        flow.assign(accessed_bytes_);
    }

    // this path is written by 'action', e.g. "assign to" or "read into":
    // a read-only name or a constant is rejected, the write is recorded in the
    // uses of the function; 'src_loc_tk' is where an error points
    auto record_write(toc& tc, const token& src_loc_tk,
                      const std::string_view action,
                      const ident_info& info) const -> void {

        assert_not_read_only(src_loc_tk, action, dotted_path_, info);

        // note: a parameter that would name a constant argument is read-only
        //       or 'mut', which takes no constant, so this is the only check

        if (info.is_const()) {
            throw compile_error{src_loc_tk,
                                std::format("cannot assign to constant {}",
                                            in_quotes(dotted_path_))};
        }

        tc.record_use(ident_info::root_of(dotted_path_), accessed_path(),
                      use_kind::write);
    }

    [[nodiscard]] auto shape() const -> array_shape { return shape_; }

    // the root variable and the path inside it that this names, see
    // 'toc::storage_of'; empty when it holds no storage, e.g. a constant
    [[nodiscard]] auto storage(const toc& tc) const
        -> std::optional<storage_path> {

        return tc.storage_of(first_token().text(), access_path_);
    }

    // index expressions are read even when the path is written
    auto visit_index_reads(const read_filter var,
                           const read_visitor reader) const -> void;

    //
    // statics
    //

    // the array of a built-in, e.g. the 'a' of 'array_length(a)' or of
    // 'foo a', is a variable, a field or an element; a call, a type or a
    // function has none
    [[nodiscard]] static auto parse_array_argument(toc& tc, tokenizer& tz)
        -> stmt_identifier {

        const token tk{tz.next_token()};
        const term tm{classify_term(tc, tk, tz)};

        if (tm.is_bare_type() or tm.is(term_kind::array_literal) or
            tm.is(term_kind::record_literal)) {

            throw compile_error{
                tk, std::format("{} is a type, not an array", in_quotes(tk))};
        }

        if (tm.is(term_kind::call) or tm.is(term_kind::type_called) or
            tm.is(term_kind::generic_call) or tm.is(term_kind::conversion) or
            tm.is(term_kind::builtin_function) or
            tm.is(term_kind::constructor_call)) {

            throw compile_error{
                tk, std::format("a call of {} is not an array, store its "
                                "result in a variable first",
                                in_quotes(tk))};
        }

        if (tm.is(term_kind::function_name)) {
            throw compile_error{tk, std::format("{} is a function, not an "
                                                "array",
                                                in_quotes(tk))};
        }

        stmt_identifier array{tc, tk, tz};

        array.assert_not_method();

        return array;
    }

  private:
    // only signed types exist so a constant must fit the signed range
    auto assert_constant_fits(const ident_info& info,
                              const type& dst_type) const -> void {

        const int64_t value{
            get_unary_ops().evaluate_constant(info.constant().value),
        };

        if (constant_arith::fits_width(value, dst_type)) {
            return;
        }

        const std::string source{statement::trimmed_source(*this)};

        throw compile_error{
            first_token(),
            std::format("constant {} does not fit {}, use '{}(...)'",
                        in_quotes(source), in_quotes(dst_type.name()),
                        dst_type.name())};
    }

    // 'ps.x' does not mean 'ps[0].x'
    auto assert_element_selected(const ident_info& prefix,
                                 const token& src_loc_tk) const -> void {

        if (elems_.back().index) {
            return;
        }

        if (not prefix.is_array()) {
            return;
        }

        throw compile_error{src_loc_tk, std::format("array {} must be indexed",
                                                    in_quotes(dotted_path_))};
    }

    // the '[' of an element needs an array
    auto assert_indexable(const ident_info& prefix, tokenizer& tz,
                          const token& src_loc_tk) const -> void {

        if (tz.peek_char_after_whitespace() == '[' and not prefix.is_array()) {

            throw compile_error{src_loc_tk,
                                std::format("cannot index non-array {}",
                                            in_quotes(dotted_path_))};
        }
    }

    // the address a call computed when it is of the root of this path, null
    // when none did or it is before the path, e.g. in the argument that a
    // parameter names
    [[nodiscard]] auto
    by_call_in_path(const std::optional<call_address>& by_call) const
        -> const call_address* {

        // note: the first binding met while resolving a path is its root, so a
        //       call address is of the root or before the path

        if (not by_call or by_call->trailing_count != elems_.size() - 1) {
            return nullptr;
        }

        return &*by_call;
    }

    // walks the elements from the root, each with the path up to it resolved,
    // e.g. 'w', 'w.rooms' and 'w.rooms.items'; a resolve error is located at
    // 'src_loc_tk', at the name of each element when none
    auto for_each_prefix(
        const toc& tc, const std::optional<token>& src_loc_tk,
        const std::function_ref<void(const prefix_step&)> visit) const -> void {

        std::string path;
        const type* parent_type{};

        for (size_t i{}; i < elems_.size(); ++i) {
            const ident_elem& elem{elems_.at(i)};

            size_t field_offset{};

            if (parent_type != nullptr) {
                path.push_back('.');

                field_offset = parent_type->field_offset(elem.name_tk,
                                                         elem.name_tk.text());
            }

            path += elem.name_tk.text();

            const ident_info info{
                tc.resolve_declared(src_loc_tk.value_or(elem.name_tk), path),
            };

            visit({
                .elem{&elem},
                .path{path},
                .info{&info},
                .parent_type{parent_type},
                .field_offset{field_offset},
                .is_last{i == elems_.size() - 1},
            });

            parent_type = &info.type_ref();
        }
    }

    // note: after a run-time index the range stays the whole array
    auto narrow_accessed_bytes(const size_t offset, const size_t size_bytes_in)
        -> void {

        if (not is_exact_access()) {
            return;
        }

        accessed_bytes_ = {
            .offset{accessed_bytes_.offset + offset},
            .size_bytes{size_bytes_in},
        };
    }

    // an unknown element leaves the whole array as the accessed range and
    // adds a run-time index to the path
    auto narrow_to_element(toc& tc, const expr_any& index_expr,
                           const ident_info& array_info) -> void;

    // a path element with an optional '[index]'
    auto parse_element(toc& tc, tokenizer& tz, const token& tk) -> void;

    auto resolve_accessed_bytes(toc& tc) -> void;

    // the path is an array only while its last element is not indexed
    auto resolve_type(const ident_info& info) -> void {
        set_type(info.type_ref());

        // an indexed element is technically no longer an array but an element
        if (elems_.back().index.has_value()) {
            return;
        }

        shape_ = info.shape();
    }

    // the variable keeps the offset and covers its whole storage
    [[nodiscard]] auto root_range(const ident_info& info) const -> byte_range {
        return {
            .offset{},
            .size_bytes{info.value_size_bytes(tok())},
        };
    }

    //
    // statics
    //

    // without a method 'T.m' a following '(' would otherwise be reported as a
    // missing field
    static auto assert_not_method_call(const ident_info& prefix, tokenizer& tz,
                                       const token& name_tk) -> void {

        if (tz.peek_char_after_whitespace() != '(') {
            return;
        }

        throw compile_error{name_tk,
                            std::format("method {} not found in type {}",
                                        in_quotes(name_tk.text()),
                                        in_quotes(prefix.type_ref().name()))};
    }

    static auto check_array_bounds(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   const machine::bounds_range& range) -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, range, tc.checks().bounds_options());
    }

    // the index goes into 'index_register', returns the constant a trailing
    // '+ c' or '- c' leaves out for the displacement, e.g. 'ix + 1' compiles
    // 'ix' and returns 1, only without bounds checks since they check the sum
    // in the register
    [[nodiscard]] static auto compile_checked_index(
        toc& tc, const size_t indent, const expr_any& index_expr,
        const operand& index_register, const size_t array_length,
        const operand& range_count) -> int64_t;

    // empty when the index is computed at run time or is out of range
    [[nodiscard]] static auto
    in_range_constant_index(const toc& tc, const expr_any& index_expr,
                            const ident_info& array_info)
        -> std::optional<size_t>;

    // 'name_tk' after the path so far names a method of the path's type
    [[nodiscard]] static auto is_method_name(const toc& tc, tokenizer& tz,
                                             const ident_info& prefix,
                                             const token& name_tk) -> bool {

        const type& path_type{prefix.type_ref()};

        const std::string method_name{
            method_name_of(path_type.name(), name_tk.text()),
        };

        if (not tc.is_func(method_name)) {
            return false;
        }

        // a method and a field may share a name, only the call has '(' or the
        // type arguments of a generic method
        const char next{tz.peek_char_after_whitespace()};

        if (next == '(' or
            (next == '<' and tc.generics().has_func(method_name))) {

            return true;
        }

        // without a field 'lst.clear = 1' reports the missing arguments
        return not path_type.has_field(name_tk.text());
    }

    // what follows the path read so far, resolved as 'prefix': nothing, a '.'
    // and a field, or a '.' and a method name, which ends the path as its
    // receiver; reads the '.' and the name
    [[nodiscard]] static auto read_continuation(const toc& tc, tokenizer& tz,
                                                const ident_info& prefix)
        -> path_continuation {

        // e.g. the '.entities' of 'rooms[i].entities[j]'
        const std::optional<token> dot_tk{tz.is_next_char_token('.')};

        if (not dot_tk) {
            return path_end{};
        }

        const token next_tk{tz.next_token()};

        // e.g. 'p..x', a path may go on after whitespace and line ends
        if (not next_tk.is_word()) {
            throw compile_error{tz.position_of(next_tk),
                                "expected a field name after '.'"};
        }

        // e.g. the '.add' of 'lst.add(1)'
        if (is_method_name(tc, tz, prefix, next_tk)) {
            return method_tokens{
                .dot_tk{*dot_tk},
                .name_tk{next_tk},
            };
        }

        return path_field{
            .dot_tk{*dot_tk},
            .name_tk{next_tk},
        };
    }
};

// storage that a node names and that is read or passed in place, e.g. the
// buffer 'b' of 'read(0, b)'; 'ident' is the bare path of the node, 'info'
// resolves it to a variable
struct place {
    const stmt_identifier* ident{};
    ident_info info;
};

// the identifier the path names, e.g. one element for 'ps[1]'
[[nodiscard]] inline auto make_ident_info(const toc& tc,
                                          const stmt_identifier& ident)
    -> ident_info {

    // the name refers to the declared array, 'ps[1]' accesses one element
    const ident_info declared{
        tc.resolve_declared(ident.tok(), ident.dotted_path()),
    };

    ident_info info{
        ident.is_array_element() ? declared.as_element() : declared,
    };

    info.set_src_loc_tk(ident.last_token());

    return info;
}

inline auto assert_not_whole_array(const stmt_identifier& ident,
                                   const ident_info& info) -> void {

    if (not info.is_array()) {
        return;
    }

    throw compile_error{ident.tok(),
                        std::format("array {} must be indexed",
                                    in_quotes(ident.dotted_path()))};
}

// a whole array is not read as its first element
[[nodiscard]] inline auto make_scalar_ident_info(const toc& tc,
                                                 const stmt_identifier& ident)
    -> ident_info {

    ident_info info{make_ident_info(tc, ident)};
    assert_not_whole_array(ident, info);
    return info;
}

// see the declaration in 'statement.hpp'
inline auto statement::bare_path() const -> const stmt_identifier* {
    const stmt_identifier* const ident{operand_path()};

    if (ident == nullptr or not ident->get_unary_ops().is_empty()) {
        return nullptr;
    }

    return ident;
}

// the storage 'st' names in place, empty for a value: a node with operators
// or unary ops, a call, a method path, a literal, a constant or a name bound
// to a register
[[nodiscard]] inline auto as_place(const toc& tc, const statement& st)
    -> std::optional<place> {

    const stmt_identifier* const ident{st.bare_path()};

    if (ident == nullptr) {
        return std::nullopt;
    }

    ident_info info{make_ident_info(tc, *ident)};

    if (not info.is_var()) {
        return std::nullopt;
    }

    return place{
        .ident{ident},
        .info{std::move(info)},
    };
}
