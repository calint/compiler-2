#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <ranges>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_order.hpp"
#include "ub_reads.hpp"

// the arguments '(first, second, count)' of a built-in function that works on
// 'count' elements of two arrays, e.g. 'array_copy(a, b, 4)' and
// 'arrays_equal(a[1], b, 3)'; the messages name the arrays by the roles the
// built-in gives them, e.g. 'source' and 'destination'
// invariants: after parsing, each array is an array or an element of one where
// the operation starts; the element types match and a constant count is in the
// bounds of both once 'check' has run, which is at compile time because the
// types are resolved then
class bulk_operands final {
    // the two arrays and the count
    static constexpr size_t argument_count_{3};
    static constexpr std::array<size_t, 2> array_indexes_{0, 1};

    builtin_arguments args_;
    // how the messages name the arrays, e.g. 'source' and 'destination'
    std::string_view first_role_;
    std::string_view second_role_;
    // the destination of a copy is written, only its indexes are read
    bool is_second_written_{};

  public:
    // what the operation does to the second array
    enum class second_use : uint8_t { read, written };

    // the addresses of the two arrays and the count, emitted in the order of
    // the arguments: 'copy_elements' and 'equal_elements' of the machine call
    // them, a known size gets both addresses from 'both_addresses' before
    // 'copy_block' or 'memory_equal'; a range is checked after the count,
    // or after the second address when the count is constant, so a later
    // argument runs first, e.g. the call of 'array_copy(a[k], b[f()], 4)'
    class argument_emitters final {
        // a run-time start, held in a register from before the walk of its
        // address until the walk has returned and the ranges are checked
        struct held_start {
            // holds the register of the start, none for a constant start and
            // once the start is freed
            std::optional<machine::held_registers> holder;
            bool is_walked{};

            [[nodiscard]] auto start() const -> std::optional<operand> {
                if (not holder) {
                    return std::nullopt;
                }

                return holder->registers().front();
            }
        };

        const bulk_operands& operands_;
        compile_toc& tc_;
        const token& src_loc_tk_;
        // none for a run-time count
        std::optional<int64_t> constant_count_;
        // the start of the first and of the second array
        std::array<held_start, 2> starts_{};
        bool are_ranges_checked_{};

      public:
        argument_emitters(const bulk_operands& operands, compile_toc& tc,
                          const token& src_loc_tk)
            : operands_{operands}, tc_{tc}, src_loc_tk_{src_loc_tk},
              constant_count_{operands.count().constant_value(tc)} {}

        argument_emitters(const argument_emitters&) = delete;
        argument_emitters(argument_emitters&&) = delete;
        auto operator=(const argument_emitters&) -> argument_emitters& = delete;
        auto operator=(argument_emitters&&) -> argument_emitters& = delete;
        ~argument_emitters() = default;

        // a known size: 'use' gets the two addresses, built in the order of
        // the arguments, e.g. for 'copy_block' or 'memory_equal'
        auto both_addresses(
            const std::function_ref<void(const operand& first_address,
                                         const operand& second_address)>
                use) -> void {

            first()(std::nullopt, [&](const operand& first_address) -> void {
                second()(std::nullopt,
                         [&](const operand& second_address) -> void {
                             use(first_address, second_address);
                         });
            });
        }

        // compiles the count into the register the machine asks for, then
        // checks the ranges of a run-time count
        // note: the compile of the count comments its source, as the
        //       arguments of 'read' and 'exit' do
        [[nodiscard]] auto count() -> std::function<void(const operand&)> {
            return [this](const operand& count_register) -> void {
                operands_.count().compile(
                    tc_, destination::of_register(count_register));

                // note: a constant count checked the ranges after the second
                //       address, e.g. for the overlap check of 'array_copy'

                if (constant_count_) {
                    return;
                }

                check_ranges(count_register);
                free_done_starts();
            };
        }

        [[nodiscard]] auto first() -> machine::address_emitter {
            return address_of(operands_.first(), starts_.at(0), false);
        }

        // a constant count checks the ranges once the second address is
        // built, where a run-time count is computed
        [[nodiscard]] auto second() -> machine::address_emitter {
            return address_of(operands_.second(), starts_.at(1),
                              constant_count_.has_value());
        }

      private:
        // a run-time start gets its register before the walk of the address,
        // which frees its own registers first; 'checks_ranges' checks the
        // ranges before the address is used
        [[nodiscard]] auto address_of(const stmt_identifier& array,
                                      held_start& held,
                                      const bool checks_ranges)
            -> machine::address_emitter {

            return [this, &array, &held, checks_ranges](
                       const std::optional<operand>& address_register,
                       const machine::address_use use) -> void {
                if (tc_.checks().is_bounds() and
                    not array.constant_start_index(tc_)) {

                    machine& x{tc_.machine()};

                    held.holder.emplace(x,
                                        emit_site{src_loc_tk_, tc_.indent()});

                    std::ignore = held.holder->alloc_scratch_register(
                        {array.tok(), tc_.indent()},
                        tc_.builtins().default_type());
                }

                walk_address(array, held.start(), address_register,
                             checks_ranges, use);

                held.is_walked = true;

                free_done_starts();
            };
        }

        // the range from the start of 'array': 'start' holds a run-time
        // start, which the walk of the address checked up to the length,
        // 'count_register' a run-time count, none for a constant one
        auto check_range(const stmt_identifier& array,
                         const std::optional<operand>& start,
                         const std::optional<operand>& count_register,
                         const bool count_lower_checked) const -> void {

            if (not tc_.checks().is_bounds()) {
                return;
            }

            const array_shape shape{array.resolve_path(tc_).shape()};

            // note: an unknown length is the check of a function without its
            //       calls or a body it checks, its code is discarded

            if (shape.is_length_unknown()) {
                return;
            }

            const std::optional<machine::bounds_range> range{
                range_of(array, shape.element_count(), start, count_register,
                         count_lower_checked),
            };

            if (not range) {
                return;
            }

            // a range past the end names the count, as the check of a
            // constant count does ('assert_count_in_bounds')
            const token& count_tk{operands_.count().tok()};

            machine& x{tc_.machine()};

            x.check_bounds({count_tk, tc_.indent()}, *range);
        }

        // the ranges of both arrays after the later arguments,
        // 'count_register' holds a run-time count, none for a constant one
        auto check_ranges(const std::optional<operand>& count_register)
            -> void {

            check_range(operands_.first(), starts_.at(0).start(),
                        count_register, false);

            // note: the check of the first range tested the sign of a
            //       run-time count where '--checks=lower' asks for it

            check_range(operands_.second(), starts_.at(1).start(),
                        count_register, true);

            are_ranges_checked_ = true;
        }

        // frees the starts that are walked and checked from the top, outside
        // the walk of an address; when the machine nests the walks, the
        // first start is under the registers of the first address and is
        // freed after it
        auto free_done_starts() -> void {
            if (not are_ranges_checked_) {
                return;
            }

            for (held_start& held : std::views::reverse(starts_)) {
                if (not held.holder) {
                    continue;
                }

                if (not held.is_walked) {
                    return;
                }

                held.holder.reset();
            }
        }

        // the range 'check_range' checks, none when the start and the count
        // are constant
        [[nodiscard]] auto
        range_of(const stmt_identifier& array, const size_t array_count,
                 const std::optional<operand>& start,
                 const std::optional<operand>& count_register,
                 const bool count_lower_checked) const
            -> std::optional<machine::bounds_range> {

            // note: a constant start with a constant count was checked at
            //       compile time

            if (not count_register and not start) {
                return std::nullopt;
            }

            if (not count_register) {
                // note: a constant count is at most the length, rejected at
                //       compile time otherwise

                assert(constant_count_ and *constant_count_ >= 0 and
                       std::cmp_less_equal(*constant_count_, array_count));

                machine::bounds_range range{
                    machine::bounds_range::up_to(
                        *start,
                        array_count - static_cast<size_t>(*constant_count_)),
                };

                range.start_lower_checked = true;

                return range;
            }

            if (start) {
                machine::bounds_range range{
                    machine::bounds_range::range(*start, *count_register,
                                                 array_count),
                };

                range.start_lower_checked = true;
                range.count_lower_checked = count_lower_checked;

                return range;
            }

            const std::optional<int64_t> constant_start{
                array.constant_start_index(tc_),
            };

            // note: a constant start is at most the length, rejected at
            //       compile time otherwise

            assert(constant_start and *constant_start >= 0 and
                   std::cmp_less_equal(*constant_start, array_count));

            return machine::bounds_range::up_to(
                *count_register,
                array_count - static_cast<size_t>(*constant_start));
        }

        // the address of 'array' passed to 'use' while the registers of its
        // walk are held, then they are freed; 'checks_ranges' checks the
        // ranges first
        auto walk_address(const stmt_identifier& array,
                          const std::optional<operand>& start,
                          const std::optional<operand>& address_register,
                          const bool checks_ranges,
                          const machine::address_use use) -> void {

            machine& x{tc_.machine()};

            x.comment({array.tok(), tc_.indent()},
                      statement::trimmed_source(array));

            machine::held_registers walk{x, {src_loc_tk_, tc_.indent()}};

            // note: the walk checks a start up to the length, its range is
            //       checked after the later arguments

            const operand address{
                array.compile_lea(tc_, walk,
                                  {
                                      .is_range{true},
                                      .start_register{start},
                                      .address_register{address_register},
                                  }),
            };

            if (checks_ranges) {
                check_ranges(std::nullopt);
            }

            use(address);
        }
    };

    // 'name_tk' is the name of the built-in, e.g. 'array_copy'
    bulk_operands(parse_toc& tc, tokenizer& tz, const token& name_tk,
                  const std::string_view first_role,
                  const std::string_view second_role, const second_use use)
        : args_{tc,
                tz,
                name_tk,
                array_indexes_,
                argument_count_,
                argument_count_},
          first_role_{first_role}, second_role_{second_role},
          is_second_written_{use == second_use::written} {}

    // the checks of the arguments in the order the statement reads: no
    // argument is written by a later one, the element types match, a constant
    // count is in bounds; returns the element type of both arrays
    [[nodiscard]] auto check(const toc& tc) const -> const type& {
        // e.g. the 'a' of 'array_copy(a, b, f())' with 'f' writing 'a'
        assert_not_written_later(tc, *this, "argument");

        const type& element_type{resolve_element_type(tc)};

        assert_constant_count_in_bounds(tc);

        return element_type;
    }

    // the bytes of the elements a constant count covers, none for a run-time
    // count; 'check' put a constant count in the bounds of both arrays
    [[nodiscard]] auto constant_size_bytes(const toc& tc,
                                           const type& element_type) const
        -> std::optional<size_t> {

        const std::optional<int64_t> constant_count{count().constant_value(tc)};

        if (not constant_count) {
            return std::nullopt;
        }

        return product_storage_size(element_type.size_bytes(),
                                    static_cast<size_t>(*constant_count));
    }

    [[nodiscard]] auto first() const -> const stmt_identifier& {
        return *args_.at(0).bare_path();
    }

    [[nodiscard]] auto second() const -> const stmt_identifier& {
        return *args_.at(1).bare_path();
    }

    auto source_to(std::ostream& os) const -> void { args_.source_to(os); }

    // the arrays and the count in their order, the second array written by
    // a copy
    auto visit_operands(const operand_visitor visitor) const -> void {
        args_.visit_operands(visitor, is_second_written_ ? std::optional{1UZ}
                                                         : std::nullopt);
    }

  private:
    // a run-time count is left to the bounds check at run time
    auto assert_constant_count_in_bounds(const toc& tc) const -> void {
        const std::optional<int64_t> constant_count{count().constant_value(tc)};

        if (not constant_count) {
            return;
        }

        first().assert_count_in_bounds(tc, count().tok(), *constant_count);
        second().assert_count_in_bounds(tc, count().tok(), *constant_count);
    }

    [[nodiscard]] auto count() const -> const expr_any& { return args_.at(2); }

    // the elements are compared or copied as a whole, so their types match
    [[nodiscard]] auto resolve_element_type(const toc& tc) const
        -> const type& {

        const ident_info first_info{make_ident_info(tc, first())};
        const ident_info second_info{make_ident_info(tc, second())};

        if (not first_info.type_ref().is_same(second_info.type_ref())) {
            throw compile_error{
                second().tok(),
                std::format("{} type {} does not match {} type {}", first_role_,
                            in_quotes(first_info.type_ref().name()),
                            second_role_,
                            in_quotes(second_info.type_ref().name()))};
        }

        return first_info.type_ref();
    }
};
