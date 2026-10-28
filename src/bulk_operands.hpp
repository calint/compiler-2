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

    // the address of each array and the run-time count, for the element
    // operations of the machine, which emit them in the order of the
    // arguments; the range of each start is checked after the count, a
    // run-time start is held in a register from its address to its check
    class run_time_emitters final {
        const bulk_operands& operands_;
        toc& tc_;
        size_t indent_{};
        const token& src_loc_tk_;
        // the run-time start of the first and of the second array
        std::array<std::optional<operand>, 2> starts_;

      public:
        run_time_emitters(const bulk_operands& operands, toc& tc,
                          const size_t indent, const token& src_loc_tk)
            : operands_{operands}, tc_{tc}, indent_{indent},
              src_loc_tk_{src_loc_tk} {}

        run_time_emitters(const run_time_emitters&) = delete;
        run_time_emitters(run_time_emitters&&) = delete;
        auto operator=(const run_time_emitters&) -> run_time_emitters& = delete;
        auto operator=(run_time_emitters&&) -> run_time_emitters& = delete;
        ~run_time_emitters() = default;

        // compiles the count into the register the machine asks for, then
        // checks the ranges and frees the start registers
        [[nodiscard]] auto count() -> std::function<void(const operand&)> {
            return [this](const operand& count_register) -> void {
                machine& x{tc_.machine()};

                const expr_any& count_expr{operands_.count()};

                x.comment(count_expr.tok(), indent_,
                          statement::trimmed_source(count_expr));

                count_expr.compile(tc_, indent_,
                                   destination::of_register(count_register));

                check_range(operands_.first(), starts_.at(0), count_register);
                check_range(operands_.second(), starts_.at(1), count_register);

                for (const std::optional<operand>& start :
                     std::views::reverse(starts_)) {

                    if (start) {
                        x.free_scratch_register(src_loc_tk_, indent_, *start);
                    }
                }
            };
        }

        [[nodiscard]] auto first() -> machine::address_emitter {
            return address_of(operands_.first(), starts_.at(0));
        }

        [[nodiscard]] auto second() -> machine::address_emitter {
            return address_of(operands_.second(), starts_.at(1));
        }

      private:
        // a run-time start gets its register before the walk of the address,
        // which frees its own registers first
        [[nodiscard]] auto address_of(const stmt_identifier& array,
                                      std::optional<operand>& start)
            -> machine::address_emitter {

            return [this, &array,
                    &start](const std::optional<operand>& address_register,
                            const machine::address_use use) -> void {
                const std::optional<int64_t> constant_count{
                    operands_.count().constant_value(tc_),
                };

                if (is_count_checked() and
                    not array.constant_start_index(tc_)) {

                    machine& x{tc_.machine()};

                    start = x.alloc_scratch_register(
                        array.tok(), indent_, tc_.builtins().default_type());
                }

                // note: the count is not negative once 'check' has run
                const size_t range_count{
                    constant_count ? static_cast<size_t>(*constant_count) : 0,
                };

                array.compile_address(tc_, indent_, src_loc_tk_,
                                      {
                                          .range_count{range_count},
                                          .start_register{start},
                                          .address_register{address_register},
                                      },
                                      use);
            };
        }

        // the range from the start of 'array' against the count in
        // 'count_register', 'start' holds a run-time start
        auto check_range(const stmt_identifier& array,
                         const std::optional<operand>& start,
                         const operand& count_register) const -> void {

            if (not is_count_checked()) {
                return;
            }

            const array_shape shape{array.resolve_path(tc_).shape()};

            // note: an unknown length is the check of a function without its
            //       calls, its code is discarded

            if (shape.is_length_unknown()) {
                return;
            }

            const size_t array_count{shape.element_count()};

            machine& x{tc_.machine()};

            if (start) {
                x.check_bounds(array.tok(), indent_,
                               machine::bounds_range::range(
                                   *start, count_register, array_count),
                               tc_.checks().bounds_options());

                return;
            }

            const std::optional<int64_t> constant_start{
                array.constant_start_index(tc_),
            };

            // note: a constant start is at most the length, rejected at
            //       compile time otherwise

            assert(constant_start and *constant_start >= 0 and
                   std::cmp_less_equal(*constant_start, array_count));

            x.check_bounds(
                array.tok(), indent_,
                machine::bounds_range::count_alone(
                    count_register,
                    array_count - static_cast<size_t>(*constant_start)),
                tc_.checks().bounds_options());
        }

        // a constant count is checked with the start, before the count
        [[nodiscard]] auto is_count_checked() const -> bool {
            return tc_.checks().is_bounds() and
                   not operands_.count().constant_value(tc_);
        }
    };

    // 'name_tk' is the name of the built-in, e.g. 'array_copy'
    bulk_operands(toc& tc, tokenizer& tz, const token& name_tk,
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
    [[nodiscard]] auto check(toc& tc) const -> const type& {
        assert_not_written_later(tc);

        const type& element_type{resolve_element_type(tc)};

        assert_constant_count_in_bounds(tc);

        return element_type;
    }

    [[nodiscard]] auto count() const -> const expr_any& { return args_.at(2); }

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
        visitor({
            .node{&args_.at(0)},
            .written_place{},
        });

        visitor({
            .node{&args_.at(1)},
            .written_place{is_second_written_ ? &second() : nullptr},
        });

        visitor({
            .node{&count()},
            .written_place{},
        });
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

    // e.g. the 'a' of 'array_copy(a, b, f())' with 'f' writing 'a'
    auto assert_not_written_later(const toc& tc) const -> void {
        ::assert_not_written_later(tc, operands_of(*this), "argument");
    }

    // the elements are compared or copied as a whole, so their types match
    [[nodiscard]] auto resolve_element_type(toc& tc) const -> const type& {
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
