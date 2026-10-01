#pragma once

#include <array>
#include <bit>
#include <fstream>
#include <limits>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

#include "assembler_rv32i.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "machine.hpp"
#include "type.hpp"

class machine_rv32i : public machine {
  protected:
    // buffered modes hold output from 'start' to 'finish' so jumps can
    // be optimized and grown to reach their targets
    using jump_mode = assembler::jump_mode;

  private:
    using op = assembler_rv32i::op;

    using section = assembler_rv32i::section;

    static constexpr size_t s0_register_index{8};
    static constexpr std::string_view variables_base_register_{"s0"};
    static constexpr size_t data_alignment_{16};
    static constexpr size_t copy_unroll_threshold_bytes_{16};
    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};
    static constexpr int syscall_read_{63};
    static constexpr int syscall_write_{64};
    static constexpr int syscall_exit_{93};
    // shifting a word right by this spreads its sign bit over all bits
    static constexpr int sign_shift_{31};
    // the return address slot keeps sp 16-byte aligned
    static constexpr int64_t frame_save_bytes_{16};
    static constexpr size_t word_size_bytes_{4};

    static constexpr const decltype(assembler_rv32i::register_names)&
        register_names_{assembler_rv32i::register_names};

    static constexpr std::array<size_t, 30> scratch_registers_{
        5,  6,  7,  28, 29, 30, 31, 8,  9,  18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 4,  3,  1,  11, 12, 13, 14, 15, 16, 17, 10,
    };

    // note: ascending t and s names keep generated code readable while argument
    //       registers stay late to avoid builtin conflicts and a0 stays last
    //       because syscalls overwrite it with their result

    struct allocation {
        size_t register_index;
        token source_location;
        size_t indent;
        bool named;
    };

    std::reference_wrapper<std::ostream> os_;
    std::string_view source_;
    jump_mode jump_mode_{};
    // empty when no binary image is written
    std::string binary_file_name_;
    // buffering output is no more logical state than writing to 'os_'
    mutable assembler_rv32i assembler_;
    const type* type_i32_{};
    uint32_t unavailable_registers_{};
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    bool multiply_helper_used_{};
    bool divide_helper_used_{};
    std::vector<allocation> allocations_;
    size_t usage_max_scratch_regs_{};
    std::vector<std::array<operand, 3>> bulk_registers_;

    // where unrolled accesses start: the address minus 'phase' is aligned to
    // 'alignment', which is at most a word
    struct access_start {
        size_t alignment{};
        size_t phase{};
    };

    // the addresses given to an active bulk operation, each start taken with
    // alignment 1 since the type alignment is given at the end
    struct bulk_addresses {
        std::array<access_start, 2> starts;
        // a missing address would leave the word alignment unproven
        size_t count{};
    };

    std::vector<bulk_addresses> bulk_addresses_;

    // 'address_scope' protects operand registers and memory base/index
    // registers from scratch allocation while lowering an operation, including
    // raw register operands on scope exit, drops temporaries allocated within
    // the scope and restores the previous unavailable mask, including during
    // exception unwinding this restores allocator bookkeeping only, not runtime
    // register values allocations that existed on entry must not be freed
    // within the scope

    class address_scope {
        machine_rv32i& backend_;
        uint32_t saved_mask_;
        size_t saved_count_;

      public:
        address_scope(machine_rv32i& backend, const operand& dst,
                      const operand& src)
            : backend_{backend}, saved_mask_{backend.unavailable_registers_},
              saved_count_{backend.allocations_.size()} {

            for (const operand* value : {&dst, &src}) {
                if (value->is_register() or value->is_memory()) {
                    backend_.unavailable_registers_ |=
                        register_mask(value->base_register());
                }
                if (value->is_memory()) {
                    backend_.unavailable_registers_ |=
                        register_mask(value->index_register());
                }
            }
        }

        address_scope(const address_scope&) = delete;
        address_scope(address_scope&&) = delete;
        auto operator=(const address_scope&) -> address_scope& = delete;
        auto operator=(address_scope&&) -> address_scope& = delete;

        ~address_scope() {
            while (backend_.allocations_.size() > saved_count_) {
                // implicit releases need the allocation context for a balanced
                // trace
                const allocation& entry{backend_.allocations_.back()};
                backend_.comment(entry.source_location, entry.indent,
                                 "free {} register {}",
                                 entry.named ? "named" : "scratch",
                                 register_names_.at(entry.register_index));
                backend_.allocations_.pop_back();
            }
            backend_.unavailable_registers_ = saved_mask_;
        }
    };

    struct address_offset_parts {
        uint32_t upper;
        int32_t low;
    };

    // a 32-bit multiplier can need one digit above bit 31 in non-adjacent form
    static constexpr size_t multiplier_digit_count{33};

    // 'value' is modified in place then written back by
    // 'store_operation_result' through 'address'
    struct loaded_destination {
        operand address;
        operand value;
    };

    // a multiplier as signed digits: a factor of 2^i is added or subtracted per
    // nonzero digit
    struct digit_sequence {
        std::array<int, multiplier_digit_count> digits;
        // the digits build the negated multiplier
        bool negate;
        // the highest and the lowest nonzero digit
        size_t top;
        size_t lowest;
    };

    // a store of constant bytes at 'offset' with 'size_bytes' of 1, 2 or 4
    struct byte_part {
        size_t offset{};
        size_t size_bytes{};
        int64_t value{};
        // zero is stored from register 'zero' and a repeated value is reused
        bool needs_load{};
    };

    // note: a bulk operation walks 'size_bytes' at one or two addresses and
    //       decides every access: unrolled or a loop, the loop width, a head
    //       up to it and a tail; a subclass only emits one access in
    //       'do_operation', may allow unrolling in 'unrolls' and may emit
    //       code after the walk in 'finish'
    //       * the variables base 's0' and a non-inline frame base 's1' are
    //         word aligned, so an unindexed address from them has a known
    //         position within a word; other addresses are only as aligned as
    //         their type
    //       * the pointers of a loop advance together, so the loop keeps one
    //         width that every address reaches after the same head of a byte
    //         and a halfword
    //       * a known size decides the head, loop and tail at compile time,
    //         a run-time count checks the head against the count and selects
    //         the tail by its low bits
    //       * registers a subclass allocates are freed with the operation

    struct bulk_operation {
      private:
        // a numeric local label as defined and as a branch refers to it
        struct local_label {
            std::string_view name;
            std::string_view reference;
        };

        // the addresses or pointers a walk advances together
        struct src_dst {
            operand src;
            operand dst;

            // 'to' applied to each operand present, zero has no source
            template <typename function_t>
            [[nodiscard]] auto map(const function_t& to) const -> src_dst {
                const auto present = [&to](const operand& value) -> operand {
                    if (value.is_empty()) {
                        return {};
                    }

                    return to(value);
                };

                return {
                    .src{present(src)},
                    .dst{present(dst)},
                };
            }
        };

        // what both walks share: one access, advancing the pointers, a loop of
        // equal chunks and the loop width for pointers
        struct walk {
            explicit walk(bulk_operation& operation_in)
                : operation{operation_in}, assembler{operation_in.assembler},
                  indent{operation_in.indent} {}

            walk(const walk&) = delete;
            walk(walk&&) = delete;
            auto operator=(const walk&) -> walk& = delete;
            auto operator=(walk&&) -> walk& = delete;
            ~walk() = default;

            bulk_operation& operation;
            assembler_rv32i& assembler;
            size_t indent;

          protected:
            // the loop runs 'width' accesses after 'head_size_bytes' of
            // narrower ones
            struct loop_start {
                size_t head_size_bytes{};
                size_t width{};
            };

          public:
            // one access 'offset' past the addresses
            auto access(const size_t width, const src_dst& addresses,
                        const size_t offset = 0) -> void {

                const src_dst moved{
                    addresses.map([offset](const operand& address) -> operand {
                        return offset_by(address, offset);
                    })};

                operation.do_operation(width, moved.src, moved.dst);
            }

            // the addresses the pointers hold
            [[nodiscard]] auto addresses_of(const src_dst& pointers) const
                -> src_dst {

                return pointers.map([this](const operand& pointer) -> operand {
                    return operand::mem(pointer.base_register(), {}, 1, 0,
                                        operation.backend.default_type());
                });
            }

            auto advance(const src_dst& pointers, const size_t size_bytes)
                -> void {

                for (const operand* pointer : {&pointers.src, &pointers.dst}) {
                    if (pointer->is_empty()) {
                        continue;
                    }

                    assembler.addi(indent, pointer->base_register(),
                                   pointer->base_register(), size_bytes);
                }
            }

            [[nodiscard]] auto describe_loop(const size_t width) const
                -> std::string {

                if (width == 1) {
                    return std::format("{} bytes", operation.verb);
                }

                return std::format("{} {}-byte {}", operation.verb, width,
                                   width == 4 ? "words" : "halfwords");
            }

            // repeats 'width' accesses 'chunks' times, 'chunks' must not be
            // zero
            auto emit_chunk_loop(const src_dst& pointers, const operand& chunks,
                                 const size_t width) -> void {

                assembler.label(indent, chunk_loop.name);
                access(width, addresses_of(pointers));
                advance(pointers, width);
                assembler.addi(indent, chunks.base_register(),
                               chunks.base_register(), -1);
                assembler.bnez(indent, chunks.base_register(),
                               chunk_loop.reference);
            }

            // unrolled accesses at offsets from the addresses
            auto emit_parts(const size_t size_bytes,
                            const std::span<const access_start> starts,
                            const src_dst& addresses) -> void {

                for_each_aligned_part(
                    size_bytes, starts,
                    [&](const size_t width, const size_t offset) -> void {
                        access(width, addresses, offset);
                    });
            }

            // names what decided the width of a pointer loop
            auto explain_width(const loop_start& start,
                               const std::array<access_start, 2>& starts,
                               const size_t alignment) const -> void {

                if (start.head_size_bytes != 0) {
                    operation.comment(
                        "{}-byte accesses after a {} B head: addresses {} and "
                        "{}",
                        start.width, start.head_size_bytes,
                        describe_start(starts.front()),
                        describe_start(starts.back()));

                    return;
                }

                if (start.width > alignment) {
                    operation.comment("{}-byte accesses: both addresses "
                                      "{}-byte aligned, type {}-byte aligned",
                                      start.width, start.width, alignment);

                    return;
                }

                operation.comment("{}-byte accesses: type {}-byte aligned, "
                                  "addresses not proven more aligned",
                                  start.width, alignment);
            }

            // pointers may hold any address, so their starts from 'start_of'
            // with alignment 1 gain the type 'alignment' here
            [[nodiscard]] auto
            plan_pointer_loop(const std::array<access_start, 2>& starts,
                              const size_t alignment,
                              const size_t max_head_size_bytes) const
                -> loop_start {

                std::array<access_start, 2> typed{starts};
                for (access_start& s : typed) {
                    s.alignment = std::max(s.alignment, bulk_width(alignment));
                }

                const loop_start start{
                    plan_loop_start(typed, max_head_size_bytes)};

                explain_width(start, typed, alignment);

                return start;
            }

            //
            // statics
            //

            [[nodiscard]] static auto offset_by(const operand& address,
                                                const size_t offset)
                -> operand {

                operand moved{address};
                moved.increment_offset(static_cast<int64_t>(offset));

                return moved;
            }

            // the widest width that every start reaches after the same head of
            // at most 'max_head_size_bytes'
            [[nodiscard]] static auto
            plan_loop_start(const std::span<const access_start> starts,
                            const size_t max_head_size_bytes) -> loop_start {

                for (size_t width{word_size_bytes_}; width > 1; width /= 2) {
                    const size_t phase{starts.front().phase % width};
                    const size_t head_size_bytes{(width - phase) % width};

                    const bool reaches_width{std::ranges::all_of(
                        starts, [width, phase](const access_start& s) -> bool {
                            return width <= s.alignment and
                                   s.phase % width == phase;
                        })};

                    if (reaches_width and
                        head_size_bytes <= max_head_size_bytes) {

                        return {
                            .head_size_bytes{head_size_bytes},
                            .width{width},
                        };
                    }
                }

                return {
                    .head_size_bytes{},
                    .width{1},
                };
            }
        };

        // the size is known at compile time, so the head, loop and tail are
        // decided here and a single chunk needs no loop
        struct known_size_walk final : walk {
            using walk::walk;

            // offsets that reach 'reach_bytes' past each address
            [[nodiscard]] auto direct(const src_dst& addresses,
                                      const size_t reach_bytes) -> src_dst {

                return addresses.map([&](const operand& address) -> operand {
                    return operation.backend.unrolled_address(
                        operation.src_loc_tk, indent, address, reach_bytes);
                });
            }

            // 'size_bytes' from pointers at a 'width' boundary
            auto emit_counted_loop(const src_dst& pointers,
                                   const operand& count,
                                   const size_t size_bytes, const size_t width)
                -> void {

                const access_start start{
                    .alignment{width},
                    .phase{},
                };

                const size_t chunk_count{size_bytes / width};
                if (chunk_count <= 1) {
                    emit_parts(size_bytes, std::span{&start, 1},
                               addresses_of(pointers));

                    return;
                }

                operation.comment("{}", describe_loop(width));
                assembler.li(indent, count.base_register(), chunk_count);
                emit_chunk_loop(pointers, count, width);

                const size_t tail_size_bytes{size_bytes % width};
                if (tail_size_bytes == 0) {
                    return;
                }

                operation.comment("{} {} B tail", operation.verb,
                                  tail_size_bytes);

                emit_parts(tail_size_bytes, std::span{&start, 1},
                           addresses_of(pointers));
            }

            // the head at direct offsets, then pointers from the boundary
            auto emit_loop(const src_dst& addresses, const size_t size_bytes,
                           const std::span<const access_start> starts) -> void {

                const loop_start loop{plan_loop_start(starts, size_bytes)};

                const std::string head{loop.head_size_bytes == 0
                                           ? std::string{}
                                           : std::format(" after a {} B head",
                                                         loop.head_size_bytes)};

                operation.comment("{} loop of {}-byte accesses{}: {}",
                                  operation.verb, loop.width, head,
                                  describe_starts(starts));

                if (loop.head_size_bytes != 0) {
                    // a head pointer for a far offset is not needed by the loop
                    const address_scope head_scope{
                        operation.backend, addresses.dst, addresses.src};

                    emit_parts(loop.head_size_bytes, starts,
                               direct(addresses, loop.head_size_bytes));
                }

                const src_dst pointers{
                    load_pointers(addresses, loop.head_size_bytes)};

                emit_counted_loop(pointers, operation.scratch_register(),
                                  size_bytes - loop.head_size_bytes,
                                  loop.width);
            }

            // the pointers are 'width - head' past a boundary, so the head
            // parts are at fixed offsets followed by one advance
            auto emit_pointer_head(const src_dst& pointers,
                                   const loop_start& start) -> void {

                if (start.head_size_bytes == 0) {
                    return;
                }

                const access_start head_start{
                    .alignment{start.width},
                    .phase{start.width - start.head_size_bytes},
                };

                operation.comment("{} {} B head", operation.verb,
                                  start.head_size_bytes);

                emit_parts(start.head_size_bytes, std::span{&head_start, 1},
                           addresses_of(pointers));

                advance(pointers, start.head_size_bytes);
            }

            auto emit_unrolled(const src_dst& addresses,
                               const size_t size_bytes,
                               const std::span<const access_start> starts)
                -> void {

                operation.backend.comment_aligned_parts(operation.src_loc_tk,
                                                        indent, operation.verb,
                                                        size_bytes, starts);

                emit_parts(size_bytes, starts, direct(addresses, size_bytes));
            }

            // unrolled accesses use direct offsets, a loop takes its head from
            // them and then walks pointers from the boundary
            auto from_addresses(const src_dst& addresses,
                                const size_t size_bytes,
                                const std::span<const access_start> starts)
                -> void {

                const address_scope scope{operation.backend, addresses.dst,
                                          addresses.src};

                const size_t part_count{aligned_part_count(size_bytes, starts)};

                if (operation.unrolls(size_bytes, part_count)) {
                    emit_unrolled(addresses, size_bytes, starts);
                    return;
                }

                emit_loop(addresses, size_bytes, starts);
            }

            auto from_pointers(const src_dst& pointers, const operand& count,
                               const size_t size_bytes,
                               const std::array<access_start, 2>& starts,
                               const size_t alignment) -> void {

                const address_scope scope{operation.backend, pointers.dst,
                                          pointers.src};

                // the known size limits the head
                const loop_start start{
                    plan_pointer_loop(starts, alignment, size_bytes)};

                emit_pointer_head(pointers, start);
                emit_counted_loop(pointers, count,
                                  size_bytes - start.head_size_bytes,
                                  start.width);
            }

            // pointer registers holding the addresses plus 'offset'
            [[nodiscard]] auto load_pointers(const src_dst& addresses,
                                             const size_t offset) -> src_dst {

                return addresses.map([&](const operand& address) -> operand {
                    const operand pointer{operation.scratch_register()};

                    operation.backend.address_of(operation.src_loc_tk, indent,
                                                 pointer,
                                                 offset_by(address, offset));

                    return pointer;
                });
            }

            //
            // statics
            //

            [[nodiscard]] static auto
            describe_starts(const std::span<const access_start> starts)
                -> std::string {

                if (starts.size() == 1) {
                    return std::format("start {}",
                                       describe_start(starts.front()));
                }

                return std::format("source {}, destination {}",
                                   describe_start(starts.front()),
                                   describe_start(starts.back()));
            }
        };

        // the byte count is known only at run time, so each head access checks
        // it and its low bits select the tail; the pointers stay aligned to
        // the loop width so each tail access is aligned too
        struct runtime_count_walk final : walk {
            using walk::walk;

            // 'count' may be zero
            auto emit_byte_loop(const src_dst& pointers, const operand& count)
                -> void {

                operation.comment("{}; skip if none", describe_loop(1));
                assembler.beqz(indent, count.base_register(),
                               walk_end.reference);
                emit_chunk_loop(pointers, count, 1);
                assembler.label(indent, walk_end.name);
            }

            // each head access first checks that the count still covers it,
            // otherwise the fewer bytes are left to the tail, which needs no
            // more than the current pointer alignment
            auto emit_checked_head(const src_dst& pointers,
                                   const operand& count, const operand& scratch,
                                   const size_t head_size_bytes) -> void {

                if (head_size_bytes == 0) {
                    return;
                }

                operation.comment("{} {} B head", operation.verb,
                                  head_size_bytes);

                for (const size_t width : {size_t{1}, size_t{2}}) {
                    if ((head_size_bytes & width) == 0) {
                        continue;
                    }

                    if (width == 1) {
                        assembler.beqz(indent, count.base_register(),
                                       after_head.reference);
                    }

                    if (width == 2) {
                        assembler.sltiu(indent, scratch.base_register(),
                                        count.base_register(), 2);
                        assembler.bnez(indent, scratch.base_register(),
                                       after_head.reference);
                    }

                    access(width, addresses_of(pointers));
                    advance(pointers, width);
                    assembler.addi(indent, count.base_register(),
                                   count.base_register(),
                                   -static_cast<int64_t>(width));
                }

                assembler.label(indent, after_head.name);
            }

            // the loop takes 'count' divided into chunks, 'count' keeps the
            // tail bytes
            auto emit_chunks(const src_dst& pointers, const operand& count,
                             const operand& chunks, const size_t width)
                -> void {

                operation.comment(
                    "split bytes into chunks and tail; skip loop if none");
                assembler.srli(indent, chunks.base_register(),
                               count.base_register(), std::countr_zero(width));
                assembler.andi(indent, count.base_register(),
                               count.base_register(), width - 1);
                assembler.beqz(indent, chunks.base_register(),
                               after_chunks.reference);
                operation.comment("{}", describe_loop(width));
                emit_chunk_loop(pointers, chunks, width);
                assembler.label(indent, after_chunks.name);
            }

            // after words at most 3 bytes remain, bit 1 selects a halfword and
            // bit 0 the final byte
            auto emit_tail(const src_dst& pointers, const operand& count,
                           const operand& scratch, const size_t width) -> void {

                if (width == 4) {
                    operation.comment("{} optional 2-byte tail",
                                      operation.verb);
                    assembler.andi(indent, scratch.base_register(),
                                   count.base_register(), 2);
                    assembler.beqz(indent, scratch.base_register(),
                                   after_halfword.reference);
                    access(2, addresses_of(pointers));
                    advance(pointers, 2);
                    assembler.label(indent, after_halfword.name);
                    assembler.andi(indent, count.base_register(),
                                   count.base_register(), 1);
                }

                operation.comment("{} optional final byte", operation.verb);
                assembler.beqz(indent, count.base_register(),
                               walk_end.reference);
                access(1, addresses_of(pointers));
                assembler.label(indent, walk_end.name);
            }

            auto from_pointers(const src_dst& pointers, const operand& count,
                               const std::array<access_start, 2>& starts,
                               const size_t alignment) -> void {

                const address_scope scope{operation.backend, pointers.dst,
                                          pointers.src};

                // the run-time count may be smaller than any head
                const loop_start start{plan_pointer_loop(
                    starts, alignment, std::numeric_limits<size_t>::max())};

                // without a known alignment every access is a byte
                if (start.width == 1) {
                    emit_byte_loop(pointers, count);
                    return;
                }

                // also the scratch of the head check and the halfword tail test
                const operand chunks{operation.scratch_register()};

                operation.comment("{}: {}, {}: tail bytes",
                                  chunks.base_register(),
                                  start.width == 4 ? "words" : "halfwords",
                                  count.base_register());

                emit_checked_head(pointers, count, chunks,
                                  start.head_size_bytes);

                emit_chunks(pointers, count, chunks, start.width);
                emit_tail(pointers, count, chunks, start.width);
            }
        };

      public:
        // parameters differ from the members they initialize
        bulk_operation(machine_rv32i& backend_in, const token& src_loc_tk_in,
                       const size_t indent_in, const std::string_view verb_in)
            : backend{backend_in}, assembler{backend_in.assembler_},
              src_loc_tk{src_loc_tk_in}, indent{indent_in}, verb{verb_in},
              allocations{backend_in, operand{}, operand{}} {}

        bulk_operation(const bulk_operation&) = delete;
        bulk_operation(bulk_operation&&) = delete;
        auto operator=(const bulk_operation&) -> bulk_operation& = delete;
        auto operator=(bulk_operation&&) -> bulk_operation& = delete;

        virtual ~bulk_operation() = default;

        machine_rv32i& backend;
        assembler_rv32i& assembler;
        token src_loc_tk;
        size_t indent;
        std::string_view verb;
        // frees the registers allocated with the operation
        address_scope allocations;

        // every local label of the walks and of a subclass 'finish', listed
        // together so none is used twice
        static constexpr local_label chunk_loop{.name{"1"}, .reference{"1b"}};
        static constexpr local_label after_chunks{.name{"2"}, .reference{"2f"}};
        static constexpr local_label after_halfword{.name{"3"},
                                                    .reference{"3f"}};
        static constexpr local_label walk_end{.name{"4"}, .reference{"4f"}};
        static constexpr local_label false_exit{.name{"5"}, .reference{"5f"}};
        static constexpr local_label result_end{.name{"6"}, .reference{"6f"}};
        static constexpr local_label after_head{.name{"7"}, .reference{"7f"}};

        //
        // virtual methods
        //

        // one access of 'width' bytes at the memory operands 'src' and 'dst',
        // 'src' is empty for zero
        virtual auto do_operation(size_t width, const operand& src,
                                  const operand& dst) -> void = 0;

        virtual auto finish() -> void {}

        // whether a known size is unrolled into 'part_count' accesses instead
        // of a loop
        [[nodiscard]] virtual auto
        unrolls([[maybe_unused]] size_t size_bytes,
                [[maybe_unused]] size_t part_count) const -> bool {

            return false;
        }

        //
        // class methods
        //

        template <typename... args_t>
        auto comment(const std::format_string<args_t...> format,
                     args_t&&... args) const -> void {

            backend.comment(src_loc_tk, indent, format,
                            std::forward<args_t>(args)...);
        }

        [[nodiscard]] auto scratch_register() const -> operand {
            return backend.alloc_scratch_register(src_loc_tk, indent,
                                                  backend.default_type());
        }

        // pointers the walk may advance, 'alignment' is the type alignment
        auto walk_from_pointers(const operand& src_pointer,
                                const operand& dst_pointer,
                                const operand& count, const size_t size_bytes,
                                const std::array<access_start, 2>& starts,
                                const size_t alignment) -> void {

            known_size_walk{*this}.from_pointers(
                {.src{src_pointer}, .dst{dst_pointer}}, count, size_bytes,
                starts, alignment);

            finish();
        }

        // 'src' is empty for zero
        auto walk_known_size(const operand& src, const operand& dst,
                             const size_t size_bytes,
                             const std::span<const access_start> starts)
            -> void {

            known_size_walk{*this}.from_addresses({.src{src}, .dst{dst}},
                                                  size_bytes, starts);
            finish();
        }

        // a byte 'count' known at run time, 'alignment' is the type alignment
        auto walk_runtime_count(const operand& src_pointer,
                                const operand& dst_pointer,
                                const operand& count,
                                const std::array<access_start, 2>& starts,
                                const size_t alignment) -> void {

            runtime_count_walk{*this}.from_pointers(
                {.src{src_pointer}, .dst{dst_pointer}}, count, starts,
                alignment);

            finish();
        }
    };

    struct bulk_zero final : bulk_operation {
        bulk_zero(machine_rv32i& backend_in, const token& src_loc_tk_in,
                  const size_t indent_in)
            : bulk_operation{backend_in, src_loc_tk_in, indent_in, "zero"} {}

        //
        // overridden methods
        //

        auto do_operation(const size_t width,
                          [[maybe_unused]] const operand& src,
                          const operand& dst) -> void override {

            assembler.store(indent, store_op(width), "zero", dst.displacement(),
                            dst.base_register());
        }

        // stores run faster than the loop's 4 instructions per word, the cap
        // only bounds the code size
        [[nodiscard]] auto unrolls([[maybe_unused]] const size_t size_bytes,
                                   const size_t part_count) const
            -> bool override {

            constexpr size_t max_unrolled_stores{16};
            return part_count <= max_unrolled_stores;
        }
    };

    struct bulk_copy final : bulk_operation {
        bulk_copy(machine_rv32i& backend_in, const token& src_loc_tk_in,
                  const size_t indent_in)
            : bulk_operation{backend_in, src_loc_tk_in, indent_in, "copy"},
              value{scratch_register()} {}

        operand value;

        //
        // overridden methods
        //

        auto do_operation(const size_t width, const operand& src,
                          const operand& dst) -> void override {

            assembler.load(indent, unsigned_load_op(width),
                           value.base_register(), src.displacement(),
                           src.base_register());

            assembler.store(indent, store_op(width), value.base_register(),
                            dst.displacement(), dst.base_register());
        }

        // direct offsets avoid two pointer temporaries for ordinary small
        // copies
        [[nodiscard]] auto
        unrolls(const size_t size_bytes,
                [[maybe_unused]] const size_t part_count) const
            -> bool override {

            return size_bytes <= copy_unroll_threshold_bytes_;
        }
    };

    // stores 1 in 'result' when every access matched and 0 at the first
    // mismatch, 'inverted' swaps them
    struct bulk_compare final : bulk_operation {
        bulk_compare(machine_rv32i& backend_in, const token& src_loc_tk_in,
                     const size_t indent_in, const operand& result_in,
                     const bool inverted_in)
            : bulk_operation{backend_in, src_loc_tk_in, indent_in, "compare"},
              result{result_in}, inverted{inverted_in},
              result_scope{backend_in, result_in, operand{}},
              reuse_result{can_hold_left_value()},
              left{reuse_result ? result : scratch_register()},
              right{scratch_register()} {

            comment("{}: left value/result, {}: right value",
                    left.base_register(), right.base_register());

            comment("stop at first mismatch");
        }

        operand result;
        bool inverted;
        // keeps the result registers from being picked for scratch
        address_scope result_scope;
        // a register result can hold the left value and saves the final copy
        bool reuse_result;
        operand left;
        operand right;

        //
        // overridden methods
        //

        auto do_operation(const size_t width, const operand& src,
                          const operand& dst) -> void override {

            assembler.load(indent, unsigned_load_op(width),
                           left.base_register(), src.displacement(),
                           src.base_register());

            assembler.load(indent, unsigned_load_op(width),
                           right.base_register(), dst.displacement(),
                           dst.base_register());

            assembler.bne(indent, left.base_register(), right.base_register(),
                          false_exit.reference);
        }

        // every access matched or the range was empty, unless a mismatch
        // branched to the false exit
        auto finish() -> void override {
            comment("all matched or empty: {}", inverted ? "false" : "true");
            assembler.li(indent, left.base_register(), inverted ? 0 : 1);
            assembler.j(indent, result_end.reference);
            assembler.label(indent, false_exit.name);
            comment("mismatch: {}", inverted ? "true" : "false");
            assembler.li(indent, left.base_register(), inverted ? 1 : 0);
            assembler.label(indent, result_end.name);
            if (not reuse_result) {
                backend.copy_value(src_loc_tk, indent, result, left);
            }
        }

        //
        // class methods
        //

        [[nodiscard]] auto can_hold_left_value() const -> bool {
            if (not result.is_register()) {
                return false;
            }

            const size_t index{register_index(result.base_register())};

            // 'zero' discards writes and 'sp' holds the stack
            if (index == register_index("zero") or
                index == register_index("sp")) {
                return false;
            }

            // the walk advances the bulk registers
            return std::ranges::none_of(
                backend.bulk_registers_.back(),
                [index](const operand& r) -> bool {
                    return register_index(r.base_register()) == index;
                });
        }
    };

  protected:
    //
    // virtual methods
    //

    // devices with a fixed memory size reject images that do not fit, an
    // operating system loads the program where it has room
    virtual auto
    check_memory_end([[maybe_unused]] const size_t memory_end_address) const
        -> void {}

    // a0 is the descriptor and receives the byte count, a1 is the address,
    // a2 the count and a7 is reserved, other registers keep their values
    virtual auto emit_read_call(const size_t indent) -> void {
        assembler_.li(indent, "a7", syscall_read_);
        assembler_.ecall(indent);
    }

    // same registers as 'emit_read_call'
    virtual auto emit_write_call(const size_t indent) -> void {
        assembler_.li(indent, "a7", syscall_write_);
        assembler_.ecall(indent);
    }

    //
    // class methods
    //

    [[nodiscard]] auto assembler() const -> assembler_rv32i& {
        return assembler_;
    }

    // i/o routines replacing system calls return through a7 and change only
    // 'clobbered' besides a0, so the call keeps just the live ones like a
    // system call does
    auto call_io_routine(const size_t indent, const std::string_view label,
                         const std::span<const std::string_view> clobbered)
        -> void {

        std::vector<std::string_view> saved;
        for (const std::string_view name : clobbered) {
            if (is_register_allocated(name)) {
                saved.push_back(name);
            }
        }

        if (saved.empty()) {
            assembler_.call(indent, label, "a7");
            return;
        }

        const size_t stack_bytes{save_registers(indent, saved)};
        assembler_.call(indent, label, "a7");
        restore_saved_registers(indent, saved, stack_bytes);
    }

  public:
    // 'binary_file_name' receives the image of the resolved output, backend
    // tests without complete programs leave it empty
    explicit machine_rv32i(std::ostream& os_ref,
                           const std::string_view source = {},
                           const jump_mode jumps = jump_mode::resolved,
                           const std::string_view binary_file_name = {})
        : os_{os_ref}, source_{source}, jump_mode_{jumps},
          binary_file_name_{binary_file_name} {

        // output before 'start' is written as emitted in every mode
        assembler_.set_direct_output(&os_.get());
    }

    using machine::comment;
    using machine::emit_data_array;

    //
    // overridden methods
    //

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const char operation, const operand& dst,
                      const operand& src) -> void override {

        assert(operation == '+' or operation == '-');

        binary_operation(src_loc_tk, indent,
                         operation == '+' ? op::add : op::sub, dst, src);
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        if (not(dst.is_register() or dst.is_memory()) or
            dst.type_ref().size_bytes() != 4) {
            throw compiler_exception{
                src_loc_tk, "RV32I address destination must be 32-bit storage"};
        }
        const address_scope scope{*this, dst, address};
        const operand value{working_register(src_loc_tk, indent, dst)};

        const operand lowered{
            lower_address(src_loc_tk, indent, address, value)};
        // a distinct base or nonzero residual offset still needs an add
        if (register_index(value.base_register()) !=
                register_index(lowered.base_register()) or
            lowered.displacement() != 0) {

            assembler_.addi(indent, value.base_register(),
                            lowered.base_register(), lowered.displacement());
        }
        if (dst.is_memory()) {
            copy_value(src_loc_tk, indent, dst, value);
        }
    }

    [[nodiscard]] auto address_size_bytes() const -> size_t override {
        return 4;
    }

    auto advance_array_iteration(const size_t indent, const operand& iterator,
                                 const operand& counter,
                                 const size_t element_size_bytes,
                                 const operand& limit,
                                 const std::string_view loop_label)
        -> void override {

        if (element_size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{token{},
                                     "array iteration exceeds RV32I range"};
        }

        const address_scope scope{*this, iterator, counter};

        add_subtract(token{}, indent, '+', iterator,
                     operand::imm(std::format("{}", element_size_bytes),
                                  default_type()));

        add_subtract(token{}, indent, '+', counter,
                     operand::imm("1", default_type()));

        emit_comparison(token{}, indent, counter, limit,
                        {
                            .operation{"!="},
                            .inverted{},
                            .destination{},
                            .target{loop_label},
                            .branch_on_true{true},
                        });
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        const size_t index{register_index(register_name)};
        const uint32_t mask{register_mask(register_name)};
        if (mask == 0 or index == 0 or index == 2 or
            (unavailable_registers_ & mask) != 0) {
            throw compiler_exception{
                src_loc_tk,
                std::format("cannot allocate register {}", register_name)};
        }
        operand result{make_register_operand(register_name, type_ref)};
        result.set_allocation_register(register_names_.at(index));
        record_allocation(src_loc_tk, indent, index, true);

        comment(src_loc_tk, indent, "allocate named register {}",
                register_names_.at(index));

        return result;
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref)
        -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        for (const size_t index : scratch_registers_) {
            if ((unavailable_registers_ & (uint32_t{1} << index)) != 0) {
                continue;
            }

            record_allocation(src_loc_tk, indent, index, false);

            usage_max_scratch_regs_ =
                std::max(scratch_count(), usage_max_scratch_regs_);

            comment(src_loc_tk, indent, "allocate scratch register -> {}",
                    register_names_.at(index));

            operand result{
                make_register_operand(register_names_.at(index), type_ref)};

            result.set_allocation_register(register_names_.at(index));

            return result;
        }

        throw compiler_exception{src_loc_tk, "out of RV32I scratch registers"};
    }

    [[nodiscard]] auto array_copy_destination_register() const
        -> operand override {

        return bulk_registers_.back().at(1);
    }

    [[nodiscard]] auto array_copy_source_register() const -> operand override {
        return bulk_registers_.back().at(0);
    }

    [[nodiscard]] auto begin_array_copy(const token& src_loc_tk,
                                        const size_t indent)
        -> operand override {

        const operand count{begin_bulk(src_loc_tk, indent)};

        const std::array<operand, 3>& registers{bulk_registers_.back()};

        comment(src_loc_tk, indent, "{}: source, {}: destination, {}: count",
                registers.at(0).base_register(),
                registers.at(1).base_register(), count.base_register());

        return count;
    }

    auto begin_data(const size_t alignment) -> void override {
        emit_arithmetic_helpers();
        assembler_.switch_section(section::data);
        assembler_.align(alignment);
        label(0, "dat");
    }

    auto begin_memory_equal(const token& src_loc_tk, const size_t indent)
        -> operand override {

        return begin_array_copy(src_loc_tk, indent);
    }

    auto bitwise(const token& src_loc_tk, const size_t indent,
                 const char operation, const operand& dst, const operand& src)
        -> void override {

        assert(operation == '&' or operation == '|' or operation == '^');

        op instruction{op::xor_op};
        if (operation == '&') {
            instruction = op::and_op;
        } else if (operation == '|') {
            instruction = op::or_op;
        }
        binary_operation(src_loc_tk, indent, instruction, dst, src);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {

        emit_jump(indent, op::j, {}, {}, target);
    }

    auto call_function(const token& src_loc_tk, const size_t indent,
                       const std::string_view label,
                       const operand& frame_address) -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(register_index(frame_address.base_register()) !=
               register_index("sp"));

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        std::vector<std::string_view> saved;
        for (const allocation& allocated : allocations_) {
            if (allocated.register_index ==
                register_index(variables_base_register_)) {

                continue;
            }

            saved.push_back(register_names_.at(allocated.register_index));
        }

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        const size_t stack_bytes{save_registers(indent, saved)};

        comment(src_loc_tk, indent, "set function frame base");
        address_of(src_loc_tk, indent,
                   make_register_operand(frame_base_register(), default_type()),
                   frame_address);

        assembler_.call(indent, label);
        if (stack_bytes != 0) {
            comment(src_loc_tk, indent, "after call: restore saved registers");
        }

        restore_saved_registers(indent, saved, stack_bytes);
    }

    [[nodiscard]] auto can_lower_index_scale(const size_t size_bytes) const
        -> bool override {

        return std::has_single_bit(size_bytes) and size_bytes <= UINT32_MAX;
    }

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        if (array_count > std::numeric_limits<uint32_t>::max() or
            src_loc_tk.at_line() > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{src_loc_tk,
                                     "bounds check exceeds RV32I range"};
        }

        comment(src_loc_tk, indent, "bounds check begin");
        emit_bounds_check(src_loc_tk, indent, reg_to_check, array_count,
                          allow_end, reg_count, options);
        comment(src_loc_tk, indent, "bounds check end");
    }

    auto check_frame_capacity(const token& src_loc_tk, const size_t indent,
                              const operand& frame_address,
                              const operand& frame_size_bytes,
                              const std::string_view failure_label,
                              const bool enabled = {}) -> void override {

        if (not enabled) {
            return;
        }

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(frame_size_bytes.is_immediate());

        comment(src_loc_tk, indent, "frame capacity check begin");

        const address_scope scope{*this, frame_address, frame_size_bytes};
        const operand start{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        address_of(src_loc_tk, indent, start, frame_address);
        if (register_mask(frame_address.base_register()) != 0 and
            frame_address.displacement() != 0) {
            assembler_.branch(
                indent, frame_address.displacement() > 0 ? op::bltu : op::bgtu,
                start.base_register(), frame_address.base_register(), "1f");
        }
        assembler_.la(indent, remaining.base_register(), "vars");
        assembler_.bltu(indent, start.base_register(),
                        remaining.base_register(), "1f");
        assembler_.la(indent, remaining.base_register(), "vars.end");
        assembler_.bltu(indent, remaining.base_register(),
                        start.base_register(), "1f");
        assembler_.sub(indent, remaining.base_register(),
                       remaining.base_register(), start.base_register());
        assembler_.lui(indent, start.base_register(),
                       assembler_rv32i::immediate::of_symbol(
                           frame_size_bytes.immediate(),
                           assembler_rv32i::immediate::part::high));
        assembler_.addi(indent, start.base_register(), start.base_register(),
                        assembler_rv32i::immediate::of_symbol(
                            frame_size_bytes.immediate(),
                            assembler_rv32i::immediate::part::low));
        assembler_.bgeu(indent, remaining.base_register(),
                        start.base_register(), "2f");
        assembler_.label(indent, "1");
        branch(indent, failure_label);
        assembler_.label(indent, "2");
        comment(src_loc_tk, indent, "frame capacity check end");
    }

    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void override {

        // synthetic tokens and standalone backend calls have no source location
        if (src_loc_tk.at_line() == 0 or source_.empty()) {
            assembler_.comment(indent, text);
            return;
        }

        const auto [line, column]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        assembler_.comment(indent, line, column, text);
    }

    auto comment_alias(const token& src_loc_tk, const size_t indent,
                       const std::string_view from, const std::string_view to,
                       [[maybe_unused]] const operand& address)
        -> void override {

        comment(src_loc_tk, indent, "alias {} -> {}", from, to);
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                format_address(address));
    }

    auto
    compare_and_branch(const token& src_loc_tk, const size_t indent,
                       const operand& lhs, const operand& rhs,
                       const comparison_action& action,
                       const std::span<const operand> scratch_registers_to_free)
        -> void override {

        // the address scopes in 'emit_comparison' end before these are freed
        emit_comparison(src_loc_tk, indent, lhs, rhs, action);
        free_scratch_registers(src_loc_tk, indent, scratch_registers_to_free);
    }

    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t size_bytes,
              const size_t alignment) -> void override {

        if (size_bytes == 0) {
            return;
        }
        if (size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{src_loc_tk,
                                     "copy size exceeds RV32I address range"};
        }
        const address_scope scope{*this, dst, src};

        // the pointers of a loop advance together, unrolled accesses can
        // follow where each address is within its word
        const std::array<access_start, 2> starts{
            start_of(src, alignment),
            start_of(dst, alignment),
        };

        bulk_copy{*this, src_loc_tk, indent}.walk_known_size(
            src, dst, size_bytes, starts);
    }

    // few bytes are stored with immediates when that takes no more code than
    // loading them from read-only data
    auto copy_bytes(const token& src_loc_tk, const size_t indent,
                    const std::string_view bytes, const operand& dst,
                    const size_t alignment,
                    const std::function_ref<std::string()> add_constant)
        -> void override {

        // the scope keeps 'dst' registers from being picked for scratch
        const address_scope scope{*this, dst, operand{}};

        const size_t width{bulk_width(access_alignment(dst, alignment))};

        const access_start dst_start{start_of(dst, alignment)};

        const std::vector<byte_part> parts{split_bytes(bytes, dst_start)};

        // the word aligned constant is copied with the alignment 'width', so
        // its parts can be narrower than the stored immediates
        const std::array<access_start, 2> copy_starts{
            access_start{
                .alignment{width},
                .phase{},
            },
            dst_start,
        };

        const size_t copy_part_count{
            aligned_part_count(bytes.size(), copy_starts)};

        if (are_immediates_smaller(parts, bytes.size(), copy_part_count)) {
            comment_aligned_parts(src_loc_tk, indent, "store", bytes.size(),
                                  std::span{&dst_start, 1});

            store_byte_parts(src_loc_tk, indent, parts, dst, bytes.size());
            return;
        }

        const operand pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        assembler_.la(indent, pointer.base_register(), add_constant());

        // read-only constants are word aligned so 'dst' limits the width
        if (bytes.size() <= copy_unroll_threshold_bytes_) {
            copy(
                src_loc_tk, indent,
                operand::mem(pointer.base_register(), {}, 1, 0, dst.type_ref()),
                dst, bytes.size(), width);

            return;
        }

        // the loop may advance the label pointer since it is private
        const operand dst_pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        // a distinct name keeps the argument order check from flagging 'dst'
        const operand& dst_address{dst};
        address_of(src_loc_tk, indent, dst_pointer, dst_address);

        const operand count{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        bulk_copy{*this, src_loc_tk, indent}.walk_from_pointers(
            pointer, dst_pointer, count, bytes.size(), copy_starts, alignment);
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());

        if (not(dst.is_register() or dst.is_memory()) or src.is_empty()) {
            throw compiler_exception{src_loc_tk, "invalid RV32I copy operands"};
        }

        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }

        if (src.is_memory()) {
            validate_address(src_loc_tk, src);
        }

        // skip self assignment
        if (dst.is_memory() and src.is_memory() and
            dst.type_ref().name() == src.type_ref().name() and
            dst.base_register() == src.base_register() and
            dst.index_register() == src.index_register() and
            dst.scale() == src.scale() and
            dst.displacement() == src.displacement()) {

            return;
        }

        const address_scope scope{*this, dst, src};

        const std::optional<int32_t> constant{
            narrowed_immediate(src, dst.type_ref())};

        // known constants are truncated and extended before emission
        if (constant.has_value()) {
            store_constant_result(src_loc_tk, indent, dst, *constant);
            return;
        }

        operand value{dst};
        if (not dst.is_register()) {
            value = src.is_register() ? src
                                      : alloc_scratch_register(
                                            src_loc_tk, indent, default_type());
        }

        load_into(src_loc_tk, indent, value, src);

        if (dst.is_memory()) {
            const operand lowered{lower_address(src_loc_tk, indent, dst)};

            // store the low 32, 16, or 8 bits at base + displacement
            assembler_.store(indent, store_op(dst.type_ref().size_bytes()),
                             value.base_register(), lowered.displacement(),
                             lowered.base_register());

            return;
        }

        if (not register_needs_extension(dst.type_ref(), src)) {
            return;
        }

        const size_t shift{32 - (dst.type_ref().size_bytes() * 8)};

        // discard high bits, then sign-extend integers or zero-extend bool
        assembler_.slli(indent, value.base_register(), value.base_register(),
                        shift);

        assembler_.immediate_op(indent, extend_shift_op(dst.type_ref()),
                                value.base_register(), value.base_register(),
                                shift);
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    [[nodiscard]] auto default_type() const -> const type& override {
        assert(type_i32_ != nullptr);

        return *type_i32_;
    }

    auto define_constant(const std::string_view name, const size_t value)
        -> void override {

        if (value > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{token{}, "constant exceeds RV32I range"};
        }
        assembler_.define_constant(name, static_cast<int64_t>(value));
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const char operation, const operand& dst,
                const operand& divisor) -> void override {

        assert(operation == '/' or operation == '%');

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_division_operand(src_loc_tk, divisor);
        // division requires a writable quotient or remainder destination
        if (not(dst.is_register() or dst.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I division destination"};
        }
        divide_helper_used_ = true;
        call_arithmetic_helper(src_loc_tk, indent, dst, divisor, true,
                               operation == '%');
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        constexpr std::string_view message{"panic: bounds at line "};
        // room for the ten digits of a 32-bit line number and a newline
        constexpr int64_t digits_bytes{16};
        constexpr int digit_zero{'0'};
        constexpr int newline{'\n'};

        label(0, "baz_bounds_panic");
        if (with_line) {
            assembler_.mv(1, "s2", "a0");
            assembler_.li(1, "a0", 2);
            assembler_.la(1, "a1", ".Lbaz_bounds_message");
            assembler_.li(1, "a2", message.size());
            emit_write_call(1);
            assembler_.addi(1, "sp", "sp", -digits_bytes);
            assembler_.mv(1, "a1", "sp");
            assembler_.li(1, "a2", 0);
            assembler_.la(1, "t0", ".Lbaz_decimal_places");
            assembler_.label(0, "1");
            assembler_.lw(1, "t1", 0, "t0");
            assembler_.li(1, "t2", 0);
            assembler_.label(0, "2");
            assembler_.bltu(1, "s2", "t1", "3f");
            assembler_.sub(1, "s2", "s2", "t1");
            assembler_.addi(1, "t2", "t2", 1);
            assembler_.j(1, "2b");
            assembler_.label(0, "3");
            assembler_.or_op(1, "t3", "a2", "t2");
            assembler_.bnez(1, "t3", "4f");
            assembler_.li(1, "t3", 1);
            assembler_.bne(1, "t1", "t3", "5f");
            assembler_.label(0, "4");
            assembler_.addi(1, "t2", "t2", digit_zero);
            assembler_.sb(1, "t2", 0, "a1");
            assembler_.addi(1, "a1", "a1", 1);
            assembler_.addi(1, "a2", "a2", 1);
            assembler_.label(0, "5");
            assembler_.addi(1, "t0", "t0", 4);
            assembler_.li(1, "t3", 1);
            assembler_.bne(1, "t1", "t3", "1b");
            assembler_.li(1, "t2", newline);
            assembler_.sb(1, "t2", 0, "a1");
            assembler_.addi(1, "a2", "a2", 1);
            assembler_.mv(1, "a1", "sp");
            assembler_.li(1, "a0", 2);
            emit_write_call(1);
        }
        exit(token{}, 1, operand::imm("255", default_type()));
        if (with_line) {
            constexpr std::array<int64_t, 10> decimal_places{
                1000000000, 100000000, 10000000, 1000000, 100000,
                10000,      1000,      100,      10,      1,
            };

            assembler_.switch_section(section::rodata);
            assembler_.label(0, ".Lbaz_bounds_message");
            assembler_.ascii(message);
            assembler_.align(4);
            assembler_.label(0, ".Lbaz_decimal_places");
            assembler_.data(4, decimal_places);
            assembler_.switch_section(section::text);
        }
    }

    auto emit_data(const size_t element_size_bytes,
                   const data_initializer& value) -> void override {

        emit_repeated_data(element_size_bytes, 1, value);
    }

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {

        data_initializer value;
        while (next(value)) {
            emit_data(element_size_bytes, value);
        }
    }

    auto emit_frame_overflow_handler() -> void override {
        constexpr std::string_view message{"panic: frame overflow"};
        constexpr std::array<int64_t, 1> newline{'\n'};

        label(0, "baz_frame_overflow");
        assembler_.li(1, "a0", 2);
        assembler_.la(1, "a1", ".Lbaz_frame_message");
        // the newline follows the message text
        assembler_.li(1, "a2", message.size() + 1);
        emit_write_call(1);
        exit(token{}, 1, operand::imm("255", default_type()));
        assembler_.switch_section(section::rodata);
        assembler_.label(0, ".Lbaz_frame_message");
        assembler_.ascii(message);
        assembler_.data(1, newline);
        // the bounds handler may follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto
    emit_most_efficient([[maybe_unused]] const token& src_loc_tk,
                        [[maybe_unused]] const size_t indent,
                        const std::function_ref<void()> emit_without_scratch,
                        const std::function_ref<void()> emit_with_scratch)
        -> void override {

        // both versions are buffered to compare sizes, even when output is
        // otherwise written as emitted
        assembler_.emit_buffered([&] -> void {
            assembler_.emit_smaller(emit_without_scratch, emit_with_scratch);
        });
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const data_initializer& value) const
        -> void override {

        if (element_size_bytes != 1 and element_size_bytes != 2 and
            element_size_bytes != 4) {

            throw compiler_exception{
                token{}, "RV32I data elements must be 1, 2, or 4 bytes"};
        }

        assembler_.repeated_data(element_size_bytes, count, value.uops,
                                 value.value);
    }

    auto emit_string_constants(const std::span<const string_constant> strings)
        -> void override {

        assembler_.switch_section(section::rodata);
        for (const string_constant& s : strings) {
            // word alignment lets copies to aligned destinations use words
            assembler_.align(word_size_bytes_);
            assembler_.label(0, s.label);
            emit_string_data(s.text);
        }
        // the arithmetic helpers follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto emit_string_data(const std::string_view value) -> void override {
        const std::optional<std::string> bytes{token::decode_string(value)};
        if (not bytes) {
            throw compiler_exception{token{},
                                     "unsupported RV32I string escape"};
        }

        assembler_.ascii(*bytes);
    }

    auto emit_zero_data(const size_t size_bytes) const -> void override {
        assembler_.zero(size_bytes);
    }

    auto end_array_copy(const token& src_loc_tk, const size_t indent,
                        const size_t element_size_bytes, const size_t alignment)
        -> void override {

        const std::array<operand, 3>& registers{bulk_registers_.back()};
        comment(src_loc_tk, indent, "{}: elements to bytes ({} bytes/element)",
                registers.at(2).base_register(), element_size_bytes);
        scale_index(src_loc_tk, indent, registers.at(2), element_size_bytes);

        bulk_copy{*this, src_loc_tk, indent}.walk_runtime_count(
            registers.at(0), registers.at(1), registers.at(2), bulk_starts(),
            alignment);

        release_bulk(src_loc_tk, indent);
    }

    auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                          const size_t element_size_bytes,
                          const size_t alignment, const operand& dst,
                          const bool inverted = false) -> void override {

        const std::array<operand, 3>& registers{bulk_registers_.back()};
        {
            // scaling the count must not pick the result registers
            const address_scope scope{*this, dst, operand{}};
            comment(src_loc_tk, indent,
                    "{}: elements to bytes ({} bytes/element)",
                    registers.at(2).base_register(), element_size_bytes);
            scale_index(src_loc_tk, indent, registers.at(2),
                        element_size_bytes);
        }

        bulk_compare{*this, src_loc_tk, indent, dst, inverted}
            .walk_runtime_count(registers.at(0), registers.at(1),
                                registers.at(2), bulk_starts(), alignment);

        release_bulk(src_loc_tk, indent);
    }

    auto end_main() -> void override {
        exit(token{}, 1, operand::imm("0", default_type()));
    }

    auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                          const size_t size_bytes, const size_t alignment,
                          const operand& dst, const bool inverted = false)
        -> void override {

        if (size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{
                src_loc_tk, "comparison size exceeds RV32I address range"};
        }
        const std::array<operand, 3>& registers{bulk_registers_.back()};

        bulk_compare{*this, src_loc_tk, indent, dst, inverted}
            .walk_from_pointers(registers.at(0), registers.at(1),
                                registers.at(2), size_bytes, bulk_starts(),
                                alignment);

        release_bulk(src_loc_tk, indent);
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);
        assembler_.li(indent, "a7", syscall_exit_);
        assembler_.ecall(indent);
    }

    auto finish() -> void override {
        // reserved by 'start', not set in backend testing mode
        if (variables_base_reserved_) {
            release_variables_base();
        }

        assert(bulk_registers_.empty());
        assert(allocations_.empty());
        assert(unavailable_registers_ == 0);
        assert(not variables_base_reserved_);
        assert(not frame_base_reserved_);

        if (assembler_.is_buffering()) {
            if (jump_mode_ == jump_mode::optimized) {
                assembler_.optimize_jumps();
            }
            assembler_.add_optimization_counts();
        }

        // otherwise the optimization counts open the statistics block
        if (not assembler_.is_buffering()) {
            assembler_.add_separator_newline();
        }

        assembler_.comment(0, std::format("max scratch registers in use: {}",
                                          usage_max_scratch_regs_));

        usage_max_scratch_regs_ = 0;
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {

        return "s1";
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void override {

        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void override {

        assert(not allocations_.empty());

        const size_t index{register_index(reg.allocation_register())};

        assert(allocations_.back().register_index == index);

        // named and scratch allocations share the same lifo pool
        comment(src_loc_tk, indent, "free {} register {}",
                allocations_.back().named ? "named" : "scratch",
                register_names_.at(index));
        unavailable_registers_ &= ~(uint32_t{1} << index);
        allocations_.pop_back();
    }

    auto label(const size_t indent, const std::string_view label)
        -> void override {

        assembler_.label(indent, label);
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {

        validate_scalar(token{}, value_type);
        const size_t index{register_index(name)};
        if (index == register_names_.size()) {
            throw compiler_exception{token{}, "invalid RV32I register"};
        }

        return operand::reg(register_names_.at(index), value_type);
    }

    // borrowed pointers avoid a temporary address and final move for indexing
    [[nodiscard]] auto memory_equal_left_register() const -> operand override {
        return bulk_registers_.back().at(0);
    }

    [[nodiscard]] auto memory_equal_right_register() const -> operand override {
        return bulk_registers_.back().at(1);
    }

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  [[maybe_unused]] const bool reuse_source = false)
        -> void override {

        validate_scalar(src_loc_tk, product.type_ref());
        validate_scalar(src_loc_tk, factor.type_ref());
        validate_destination_storage(src_loc_tk, product, "multiply");

        // validate memory operands even when a constant eliminates the
        // operation

        if (factor.is_memory()) {
            validate_address(src_loc_tk, factor);
        }

        const std::optional<int32_t> constant{immediate_value(factor)};

        // variable factors use the shared runtime helper
        if (not constant.has_value()) {
            multiply_helper_used_ = true;
            call_arithmetic_helper(src_loc_tk, indent, product, factor, false);
            return;
        }

        multiply_by_constant(src_loc_tk, indent, product, factor, *constant);
    }

    auto read(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& descriptor, const operand& address,
              const operand& count) -> void override {

        const operand call_register{reserve_io_call_register(
            src_loc_tk, indent, dst, descriptor, address, count)};

        emit_read_call(indent);
        free_named_register(src_loc_tk, indent, call_register);
    }

    [[nodiscard]] auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers override {

        static constexpr std::array<std::string_view, 3> io_args{"a0", "a1",
                                                                 "a2"};

        static constexpr std::array<std::string_view, 1> exit_args{"a0"};

        if (function == builtin_function::exit) {
            return {
                .arguments{exit_args},
                .result{},
            };
        }

        return {
            .arguments{io_args},
            .result{"a0"},
        };
    }

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);

        operand base{
            make_register_operand(frame_base_register(), default_type())};

        base.set_allocation_register(frame_base_register());
        free_named_register(token{}, 0, base);
        frame_base_reserved_ = false;
    }

    auto release_variables_base() -> void override {
        assert(variables_base_reserved_);

        operand base{
            make_register_operand(variables_base_register(), default_type())};

        base.set_allocation_register(variables_base_register());

        free_named_register(token{}, 0, base);

        variables_base_reserved_ = false;
    }

    auto reserve_frame_base() -> void override {
        assert(not frame_base_reserved_);

        static_cast<void>(alloc_named_register(
            token{}, 0, frame_base_register(), default_type()));

        frame_base_reserved_ = true;

        assembler_.addi(1, "sp", "sp", -frame_save_bytes_);
        assembler_.sw(1, "ra", 0, "sp");
    }

    auto reserve_variables(const size_t alignment, const size_t size_bytes)
        -> void override {

        label(0, "dat.end");
        // variables are zeroed when defined, so the image does not hold them
        assembler_.switch_section(section::bss);
        assembler_.align(alignment);
        label(0, "vars");
        assembler_.zero(size_bytes);
        label(0, "vars.end");
    }

    auto reserve_variables_base() -> void override {
        assert(not variables_base_reserved_);

        static_cast<void>(alloc_named_register(
            token{}, 0, variables_base_register(), default_type()));

        variables_base_reserved_ = true;
    }

    auto return_function(const size_t indent) -> void override {
        assembler_.lw(indent, "ra", 0, "sp");
        assembler_.addi(indent, "sp", "sp", frame_save_bytes_);
        assembler_.ret(indent);
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void override {

        // index scaling uses the target's address width
        if (index.type_ref().size_bytes() != address_size_bytes() or
            element_size_bytes > std::numeric_limits<uint32_t>::max()) {

            throw compiler_exception{src_loc_tk,
                                     "index scale exceeds RV32I address range"};
        }

        multiply(src_loc_tk, indent, index,
                 operand::imm(std::format("{}", element_size_bytes),
                              default_type()));
    }

    auto set_array_copy_destination(const size_t indent, const operand& address)
        -> void override {

        record_bulk_address_start(address);
        address_of(token{}, indent, bulk_registers_.back().at(1), address);
    }

    auto set_array_copy_source(const size_t indent, const operand& address)
        -> void override {

        record_bulk_address_start(address);
        address_of(token{}, indent, bulk_registers_.back().at(0), address);
    }

    auto set_builtin_types([[maybe_unused]] const type& t_i64,
                           const type& t_i32,
                           [[maybe_unused]] const type& t_i16,
                           [[maybe_unused]] const type& t_i8,
                           [[maybe_unused]] const type& t_bool,
                           [[maybe_unused]] const type& t_void)
        -> void override {

        type_i32_ = &t_i32;
    }

    auto set_memory_equal_left(const size_t indent, const operand& address)
        -> void override {

        set_array_copy_source(indent, address);
    }

    auto set_memory_equal_right(const size_t indent, const operand& address)
        -> void override {

        set_array_copy_destination(indent, address);
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const char operation, const operand& dst, const operand& count)
        -> void override {

        assert(operation == '<' or operation == '>');

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_shift_operand(src_loc_tk, count);
        validate_destination_storage(src_loc_tk, dst, "shift");

        const std::optional<int32_t> constant{immediate_value(count)};

        // immediate shifts must be resolved here rather than by the assembler
        if (count.is_immediate() and not constant.has_value()) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I immediate shift count"};
        }

        const uint32_t shift_count{static_cast<uint32_t>(constant.value_or(0)) &
                                   31U};
        const size_t bits{dst.type_ref().size_bytes() * 8};

        if (constant.has_value() and shift_count == 0) {
            return;
        }

        // every bit is shifted out
        if (constant.has_value() and shift_count >= bits and
            (operation == '<' or dst.type_ref().name() == "bool")) {

            store_constant_result(src_loc_tk, indent, dst, 0);
            return;
        }

        const address_scope scope{*this, dst, count};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, dst)};

        if (constant.has_value()) {
            shift_by_constant(indent, operation, dst, loaded, shift_count,
                              bits);

            return;
        }

        shift_by_register(src_loc_tk, indent, operation, dst, count, loaded);
    }

    auto start() -> void override {
        multiply_helper_used_ = false;
        divide_helper_used_ = false;

        // resolved and optimized jumps need every line before writing
        assembler_.set_direct_output(
            jump_mode_ == jump_mode::as_emitted ? &os_.get() : nullptr);

        assembler_.comment(0, "");
        assembler_.comment(0, "generated by baz");
        assembler_.comment(0, "");
        assembler_.add_separator_newline();

        assembler_.option_norvc();
        assembler_.option_norelax();
        assembler_.add_separator_newline();
        assembler_.switch_section(section::text);
        assembler_.globl("_start");
        label(0, "_start");
        assembler_.add_separator_newline();
        reserve_variables_base();
        assembler_.la(0, variables_base_register_, "dat");
        assembler_.add_separator_newline();
    }

    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void override {

        copy_value(src_loc_tk, indent, dst,
                   operand::imm(value ? "1" : "0", default_type()));
    }

    auto unary(const size_t indent, const char operation,
               const operand& destination) -> void override {

        assert(operation == '-' or operation == '~');

        validate_scalar(token{}, destination.type_ref());
        if (not(destination.is_register() or destination.is_memory())) {
            throw compiler_exception{token{},
                                     "invalid RV32I unary destination"};
        }
        const address_scope scope{*this, destination, operand{}};

        const loaded_destination loaded{
            load_destination(token{}, indent, destination)};

        if (operation == '-') {
            assembler_.sub(indent, loaded.value.base_register(), "zero",
                           loaded.value.base_register());

            store_operation_result(indent, destination, loaded.address,
                                   loaded.value, true);

            return;
        }

        // 'not' of a bool flips the stored byte only
        const int mask{destination.type_ref().name() == "bool"
                           ? std::numeric_limits<uint8_t>::max()
                           : -1};

        assembler_.xori(indent, loaded.value.base_register(),
                        loaded.value.base_register(), mask);

        store_operation_result(indent, destination, loaded.address,
                               loaded.value, false);
    }

    auto validate_division_operand(const token& src_loc_tk,
                                   const operand& divisor) const
        -> void override {

        validate_scalar(src_loc_tk, divisor.type_ref());
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        validate_scalar(src_loc_tk, count.type_ref());
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return variables_base_register_;
    }

    auto write(const token& src_loc_tk, const size_t indent, const operand& dst,
               const operand& descriptor, const operand& address,
               const operand& count) -> void override {

        const operand call_register{reserve_io_call_register(
            src_loc_tk, indent, dst, descriptor, address, count)};

        emit_write_call(indent);
        free_named_register(src_loc_tk, indent, call_register);
    }

    // a named binary image is written together with the assembly source
    auto write_assembly(std::ostream& os) -> void override {
        // written output was not kept to assemble
        if (not assembler_.is_buffering()) {
            return;
        }

        // the check precedes any output so a failing build writes nothing
        assembler_.resolve_jumps();
        check_memory_end(assembler_.memory_end_address());

        // counted after resolving because grown jumps take more instructions,
        // and added while still buffering so it ends the written lines
        assembler_.comment(0, std::format("{:>28}: {}", "instructions",
                                          assembler_.instruction_count()));

        assembler_.set_direct_output(&os_.get());

        if (binary_file_name_.empty()) {
            assembler_.write_resolved(os);
            return;
        }

        std::ofstream binary{binary_file_name_, std::ios::binary};
        if (not binary) {
            throw panic_exception{
                std::format("cannot write '{}'", binary_file_name_)};
        }

        assembler_.write_resolved(os, binary);
    }

    auto zero(const token& src_loc_tk, const size_t indent,
              const operand& destination, const size_t size_bytes,
              const size_t alignment) -> void override {

        if (size_bytes == 0) {
            return;
        }
        const address_scope scope{*this, destination, operand{}};

        const access_start start{start_of(destination, alignment)};

        bulk_zero{*this, src_loc_tk, indent}.walk_known_size(
            operand{}, destination, size_bytes, std::span{&start, 1});
    }

  private:
    // a direct offset from a word aligned base can prove more alignment than
    // the type does
    [[nodiscard]] auto access_alignment(const operand& address,
                                        const size_t alignment) const
        -> size_t {

        if (not is_word_based(address)) {
            return alignment;
        }

        // the lowest set bit of the displacement, capped at a word; the low
        // bits of a negative displacement give the same alignment
        const size_t displacement_alignment{offset_alignment(
            static_cast<size_t>(address.displacement()), word_size_bytes_)};

        return std::max(alignment, displacement_alignment);
    }

    // the destination holds the computed address when that keeps its inputs
    [[nodiscard]] auto
    address_result_register(const token& src_loc_tk, const size_t indent,
                            const operand& address, const operand& destination)
        -> operand {

        if (can_reuse_address_destination(address, destination)) {
            return destination;
        }

        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    auto begin_bulk(const token& src_loc_tk, const size_t indent) -> operand {
        // argument-order allocation keeps pointer and count names easy to
        // follow
        std::array<operand, 3> registers;
        for (operand& reg : registers) {
            reg = alloc_scratch_register(src_loc_tk, indent, default_type());
        }
        bulk_registers_.push_back(registers);
        bulk_addresses_.emplace_back();

        return registers.back();
    }

    auto binary_operation(const token& src_loc_tk, const size_t indent,
                          const op instruction, const operand& destination,
                          const operand& src) -> void {

        validate_scalar(src_loc_tk, destination.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());
        if (not(destination.is_register() or destination.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I operation destination"};
        }
        if (destination.is_memory()) {
            validate_address(src_loc_tk, destination);
        }
        if (src.is_memory()) {
            validate_address(src_loc_tk, src);
        }

        const size_t width{destination.type_ref().size_bytes()};

        const std::optional<int32_t> constant{
            narrowed_immediate(src, destination.type_ref())};

        if (constant.has_value() and
            keeps_destination(instruction, *constant, width)) {
            return;
        }

        if (constant.has_value() and
            yields_constant(instruction, *constant, width)) {

            store_constant_result(src_loc_tk, indent, destination, *constant);
            return;
        }

        const bool identical{
            same_memory(destination, src) or
            (destination.is_register() and src.is_register() and
             register_index(destination.base_register()) ==
                 register_index(src.base_register()) and
             destination.type_ref().name() == src.type_ref().name())};

        // 'x & x' and 'x | x' are 'x'
        if (identical and
            (instruction == op::and_op or instruction == op::or_op)) {
            return;
        }

        // 'x - x' and 'x ^ x' are 0
        if (identical and
            (instruction == op::sub or instruction == op::xor_op)) {

            store_constant_result(src_loc_tk, indent, destination, 0);
            return;
        }

        const address_scope scope{*this, destination, src};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, destination)};

        emit_binary_instruction(src_loc_tk, indent, instruction, destination,
                                src, loaded.value, constant);

        store_operation_result(
            indent, destination, loaded.address, loaded.value,
            needs_normalize(instruction, destination, src, constant));
    }

    // a missing address would leave the loop width unproven
    [[nodiscard]] auto bulk_starts() const -> std::array<access_start, 2> {
        const bulk_addresses& addresses{bulk_addresses_.back()};

        assert(addresses.count == 2);

        return addresses.starts;
    }

    auto call_arithmetic_helper(const token& src_loc_tk, const size_t indent,
                                const operand& destination,
                                const operand& source, const bool division,
                                const bool remainder = {}) -> void {

        const address_scope scope{*this, destination, source};

        const uint32_t live{unavailable_registers_};

        // argument registers are allocated last, so the helpers rarely
        // clobber a live scratch register that would need saving
        constexpr std::array<std::string_view, 8> clobbers{
            "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6"};

        const size_t clobber_count{division ? clobbers.size() : 5};
        std::vector<std::string_view> saved;
        for (const std::string_view name :
             std::span{clobbers}.first(clobber_count)) {

            // restoring a register destination would discard the result
            if ((live & register_mask(name)) != 0 and
                (not destination.is_register() or
                 register_index(destination.base_register()) !=
                     register_index(name))) {

                saved.push_back(name);
            }

            // staging registers must survive the helper call
            unavailable_registers_ |= register_mask(name);
        }

        // stack operands must be read before the save area changes sp
        const bool stack_operands{uses_register(destination, "sp") or
                                  uses_register(source, "sp")};
        operand left;
        operand right;
        if (stack_operands) {
            left = alloc_scratch_register(src_loc_tk, indent, default_type());
            right = alloc_scratch_register(src_loc_tk, indent, default_type());
            copy_value(src_loc_tk, indent, left, destination);
            copy_value(src_loc_tk, indent, right, source);
        }

        // save caller values before argument setup overwrites a0 or a1
        const size_t stack_bytes{save_registers(indent, saved)};

        load_helper_arguments(src_loc_tk, indent, destination, source, left,
                              right);

        assembler_.call(indent, division ? ".Lbaz_divide" : ".Lbaz_multiply");

        const operand result{
            operand::reg(remainder ? "a1" : "a0", default_type())};

        // the destination is not restored so it can retain the result
        if (destination.is_register() and not stack_operands) {
            copy_value(src_loc_tk, indent, destination, result);
            restore_saved_registers(indent, saved, stack_bytes);

            return;
        }

        const operand kept{
            preserved_helper_result(src_loc_tk, indent, result, left, saved)};

        restore_saved_registers(indent, saved, stack_bytes);

        // memory destinations need their original address registers back
        copy_value(src_loc_tk, indent, destination, kept);
    }

    // passing checks branch to '2f' and failing ones to '1f', the last check
    // branches past the handler on success so failures fall through to it
    auto check_lower_bounds(const size_t indent, const std::string_view index,
                            const operand& reg_count, const bool is_last)
        -> void {

        const auto check_negative = [&](const std::string_view reg,
                                        const bool last) -> void {
            assembler_.branch_zero(indent, last ? op::bgez : op::bltz, reg,
                                   last ? "2f" : "1f");
        };

        if (reg_count.is_empty()) {
            check_negative(index, is_last);
            return;
        }

        check_negative(index, false);

        // a negative count passes 'start + count' but spans the address space
        check_negative(reg_count.base_register(), is_last);
    }

    auto check_upper_bound(const token& src_loc_tk, const size_t indent,
                           const std::string_view index,
                           const size_t array_count, const bool allow_end,
                           const operand& reg_count, const bool lower_checked)
        -> void {

        const operand limit{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const std::string top{upper_bound_top(src_loc_tk, indent, index,
                                              reg_count, limit, lower_checked)};

        assembler_.li(indent, limit.base_register(), array_count);
        if (allow_end) {
            assembler_.bgeu(indent, limit.base_register(), top, "2f");
            return;
        }

        assembler_.bltu(indent, top, limit.base_register(), "2f");
    }

    // accesses widen after the first only when a start inside a word was
    // peeled, other sequences keep the uncommented widths of the alignment
    auto comment_aligned_parts(const token& src_loc_tk, const size_t indent,
                               const std::string_view verb,
                               const size_t size_bytes,
                               const std::span<const access_start> starts)
        -> void {

        size_t first_width{};
        size_t widest{};
        for_each_aligned_part(size_bytes, starts,
                              [&](const size_t width, const size_t) -> void {
                                  if (first_width == 0) {
                                      first_width = width;
                                  }

                                  widest = std::max(widest, width);
                              });

        if (widest == first_width) {
            return;
        }

        const std::string parts{describe_parts(size_bytes, starts)};

        if (starts.size() == 1) {
            comment(src_loc_tk, indent,
                    "{} {}: start {}, widest aligned access at each offset",
                    verb, parts, describe_start(starts.front()));

            return;
        }

        comment(src_loc_tk, indent,
                "{} {}: source {}, destination {}, widest access aligned for "
                "both at each offset",
                verb, parts, describe_start(starts.front()),
                describe_start(starts.back()));
    }

    // 'source' in a register of the comparison width
    auto comparison_operand(const token& src_loc_tk, const size_t indent,
                            const operand& source, const operand& other,
                            const type& width_type, const operand& destination)
        -> operand {

        // matching register representations need no conversion
        if (source.is_register() and
            source.type_ref().name() == width_type.name()) {
            return source;
        }

        // zero has the same representation at every supported width
        if (immediate_value(source) == 0) {
            return operand::reg("zero", width_type);
        }

        // the output can hold an input unless doing so destroys the other
        // value or an address still needed to load it
        const size_t output_register{
            register_index(destination.base_register())};

        const bool reuse_destination{
            destination.is_register() and output_register != 0 and
            (not(other.is_register() or other.is_memory()) or
             output_register != register_index(other.base_register())) and
            (not other.is_memory() or
             output_register != register_index(other.index_register()))};

        // preserve the comparison width rather than narrowing to bool
        const operand value{
            reuse_destination
                ? operand::reg(destination.base_register(), width_type)
                : alloc_scratch_register(src_loc_tk, indent, width_type)};

        copy_value(src_loc_tk, indent, value, source);

        return value;
    }

    // prefer the output register or a temporary the comparison owns
    auto comparison_result_register(const token& src_loc_tk,
                                    const size_t indent,
                                    const operand& destination,
                                    const operand& lhs, const operand& rhs,
                                    const operand& left, const operand& right)
        -> operand {

        if (destination.is_register()) {
            return destination;
        }

        // materialized operands are ours to overwrite after the comparison
        if (not left.allocation_register().empty() and not lhs.is_register()) {
            return left;
        }

        if (not right.allocation_register().empty() and not rhs.is_register()) {
            return right;
        }

        // both operands are live inputs or zero
        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    auto emit_arithmetic_helpers() const -> void {
        // unused helpers contribute no code
        if (multiply_helper_used_) {
            assembler_.label(0, ".Lbaz_multiply");
            assembler_.mv(1, "a2", "a0");
            assembler_.li(1, "a0", 0);
            assembler_.beqz(1, "a1", "3f");
            assembler_.label(0, "1");
            assembler_.andi(1, "a3", "a1", 1);
            assembler_.beqz(1, "a3", "2f");
            assembler_.add(1, "a0", "a0", "a2");
            assembler_.label(0, "2");
            assembler_.slli(1, "a2", "a2", 1);
            assembler_.srli(1, "a1", "a1", 1);
            assembler_.bnez(1, "a1", "1b");
            assembler_.label(0, "3");
            assembler_.ret(1);
        }
        // divide and remainder share magnitude division and sign restoration
        if (divide_helper_used_) {
            // one quotient bit per step
            constexpr int divide_steps{32};

            assembler_.label(0, ".Lbaz_divide");
            assembler_.beqz(1, "a1", "5f");
            assembler_.srai(1, "a4", "a0", sign_shift_);
            assembler_.srai(1, "a3", "a1", sign_shift_);
            assembler_.xor_op(1, "a0", "a0", "a4");
            assembler_.sub(1, "a0", "a0", "a4");
            assembler_.xor_op(1, "a1", "a1", "a3");
            assembler_.sub(1, "a1", "a1", "a3");
            assembler_.xor_op(1, "a3", "a3", "a4");
            assembler_.li(1, "a2", 0);
            assembler_.li(1, "a5", divide_steps);
            assembler_.label(0, "1");
            assembler_.srli(1, "a6", "a0", sign_shift_);
            assembler_.slli(1, "a2", "a2", 1);
            assembler_.or_op(1, "a2", "a2", "a6");
            assembler_.slli(1, "a0", "a0", 1);
            assembler_.bltu(1, "a2", "a1", "2f");
            assembler_.sub(1, "a2", "a2", "a1");
            assembler_.ori(1, "a0", "a0", 1);
            assembler_.label(0, "2");
            assembler_.addi(1, "a5", "a5", -1);
            assembler_.bnez(1, "a5", "1b");
            assembler_.xor_op(1, "a0", "a0", "a3");
            assembler_.sub(1, "a0", "a0", "a3");
            assembler_.xor_op(1, "a1", "a2", "a4");
            assembler_.sub(1, "a1", "a1", "a4");
            assembler_.ret(1);
            assembler_.label(0, "5");
            assembler_.ebreak(1);
            assembler_.j(1, "5b");
        }
    }

    // applies 'instruction' to 'left' with an immediate when it fits
    auto emit_binary_instruction(const token& src_loc_tk, const size_t indent,
                                 const op instruction,
                                 const operand& destination, const operand& src,
                                 const operand& left,
                                 const std::optional<int32_t> constant)
        -> void {

        const bool arithmetic{instruction == op::add or instruction == op::sub};

        const int64_t immediate{instruction == op::sub
                                    ? -int64_t{constant.value_or(0)}
                                    : int64_t{constant.value_or(0)}};

        if (constant.has_value() and immediate >= immediate_min and
            immediate <= immediate_max) {

            assembler_.immediate_op(indent, immediate_form(instruction),
                                    left.base_register(), left.base_register(),
                                    immediate);

            return;
        }

        // two addi are shorter than li and add
        if (constant.has_value() and arithmetic and
            immediate >= 2 * immediate_min and immediate <= 2 * immediate_max) {

            const int64_t first{immediate < 0 ? immediate_min : immediate_max};

            assembler_.addi(indent, left.base_register(), left.base_register(),
                            first);

            assembler_.addi(indent, left.base_register(), left.base_register(),
                            immediate - first);

            return;
        }

        const operand right{source_register(src_loc_tk, indent, destination,
                                            src, left, constant)};

        assembler_.register_op(indent, instruction, left.base_register(),
                               left.base_register(), right.base_register());
    }

    // sets 'result' to 1 when the comparison holds and to 0 otherwise
    auto emit_boolean_result(const size_t indent,
                             const std::string_view operation,
                             const std::string_view result, const operand& left,
                             const operand& right,
                             const std::optional<int64_t> immediate,
                             const bool inverted) -> void {

        if (operation == "==" or operation == "!=") {
            emit_equality_result(indent, result, left, right, immediate,
                                 inverted != (operation == "!="));

            return;
        }

        emit_less_than(indent, operation, result, left, right, immediate);

        // 'slt' answers '<', against a threshold '>' and '>=' are its
        // complement, with swapped registers '>=' and '<=' are
        const bool complement{immediate.has_value()
                                  ? (operation == ">=" or operation == ">")
                                  : (operation == ">=" or operation == "<=")};

        if (inverted != complement) {
            assembler_.xori(indent, result, result, 1);
        }
    }

    // the address scope frees its temporaries before the end comment
    auto emit_bounds_check(const token& src_loc_tk, const size_t indent,
                           const operand& reg_to_check,
                           const size_t array_count, const bool allow_end,
                           const operand& reg_count,
                           const bounds_check_options& options) -> void {

        const address_scope scope{*this, reg_to_check, reg_count};

        const std::string_view index{reg_to_check.base_register()};

        if (options.lower) {
            comment(src_loc_tk, indent, "lower bound");
            check_lower_bounds(indent, index, reg_count, not options.upper);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");
            check_upper_bound(src_loc_tk, indent, index, array_count, allow_end,
                              reg_count, options.lower);
        }

        assembler_.label(indent, "1");
        if (options.with_line) {
            comment(src_loc_tk, indent, "source line");
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }
        branch(indent, "baz_bounds_panic");
        assembler_.label(indent, "2");
    }

    // the address scopes end on return, before the caller frees its registers
    auto emit_comparison(const token& src_loc_tk, const size_t indent,
                         const operand& lhs, const operand& rhs,
                         const comparison_action& action) -> void {

        const address_scope destination_scope{*this, action.destination,
                                              operand{}};

        const address_scope scope{*this, lhs, rhs};

        validate_scalar(src_loc_tk, lhs.type_ref());
        validate_scalar(src_loc_tk, rhs.type_ref());

        const std::string_view operation{action.operation};

        const std::optional<int32_t> constant{
            narrowed_immediate(rhs, lhs.type_ref())};

        // x > c and x <= c use the signed threshold c + 1
        const bool inclusive_threshold{operation == ">" or operation == "<="};
        const int64_t threshold{int64_t{constant.value_or(0)} +
                                (inclusive_threshold ? 1 : 0)};

        // a branch compares registers, a boolean result can use an immediate
        const bool use_immediate{
            not action.destination.is_empty() and constant.has_value() and
            threshold >= immediate_min and threshold <= immediate_max};

        const std::optional<int64_t> immediate{
            use_immediate ? std::optional<int64_t>{threshold} : std::nullopt};

        const operand left{comparison_operand(
            src_loc_tk, indent, lhs, rhs, lhs.type_ref(), action.destination)};

        const operand right{use_immediate
                                ? operand::reg("zero", lhs.type_ref())
                                : comparison_operand(src_loc_tk, indent, rhs,
                                                     left, lhs.type_ref(),
                                                     action.destination)};

        // branch-only comparisons do not need a materialized boolean
        if (action.destination.is_empty()) {
            emit_comparison_branch(indent, operation, left, right, action);
            return;
        }

        const operand value{comparison_result_register(
            src_loc_tk, indent, action.destination, lhs, rhs, left, right)};

        const std::string_view result{value.base_register()};

        emit_boolean_result(indent, operation, result, left, right, immediate,
                            action.inverted);

        // memory results require a store, register results are in place
        if (action.destination.is_memory()) {
            copy_value(src_loc_tk, indent, action.destination, value);
        }

        // some callers request both a stored boolean and a branch
        if (not action.target.empty()) {
            emit_jump(indent, action.branch_on_true ? op::bne : op::beq, result,
                      "zero", action.target);
        }
    }

    auto emit_comparison_branch(const size_t indent,
                                const std::string_view operation,
                                const operand& left, const operand& right,
                                const comparison_action& action) -> void {

        // no target means the comparison result is discarded
        if (action.target.empty()) {
            return;
        }

        const bool inverted{action.inverted != not action.branch_on_true};

        // equality and inequality share one branch pair
        if (operation == "==" or operation == "!=") {
            const bool branch_on_unequal{inverted != (operation == "!=")};

            emit_jump(indent, branch_on_unequal ? op::bne : op::beq,
                      left.base_register(), right.base_register(),
                      action.target);

            return;
        }

        // ordered comparisons use signed blt/bge, swapping for > and <=
        const bool swapped{operation == ">" or operation == "<="};
        const bool branch_on_greater_equal{
            inverted != (operation == ">=" or operation == "<=")};

        emit_jump(indent, branch_on_greater_equal ? op::bge : op::blt,
                  swapped ? right.base_register() : left.base_register(),
                  swapped ? left.base_register() : right.base_register(),
                  action.target);
    }

    auto emit_equality_result(const size_t indent,
                              const std::string_view result,
                              const operand& left, const operand& right,
                              const std::optional<int64_t> immediate,
                              const bool unequal) -> void {

        const std::string_view tested{
            equality_tested_register(indent, result, left, right, immediate)};

        // inequality is a nonzero test, not a second boolean inversion
        if (unequal) {
            assembler_.sltu(indent, result, "zero", tested);
            return;
        }

        assembler_.sltiu(indent, result, tested, 1);
    }

    // 'j' or a conditional branch comparing 'first' and 'second'
    auto emit_jump(const size_t indent, const op code,
                   const std::string_view first, const std::string_view second,
                   const std::string_view target) -> void {

        if (code == op::j) {
            assembler_.resolved_jump(indent, target, far_jump_register());
            return;
        }

        assembler_.resolved_branch(indent, code, first, second, target,
                                   far_jump_register());
    }

    auto emit_less_than(const size_t indent, const std::string_view operation,
                        const std::string_view result, const operand& left,
                        const operand& right,
                        const std::optional<int64_t> immediate) -> void {

        // encodable thresholds avoid materializing a constant register
        if (immediate.has_value()) {
            assembler_.slti(indent, result, left.base_register(), *immediate);
            return;
        }

        // 'x > y' is 'y < x' and 'x <= y' is the complement of 'y < x'
        const bool swapped{operation == ">" or operation == "<="};

        assembler_.slt(indent, result,
                       swapped ? right.base_register() : left.base_register(),
                       swapped ? left.base_register() : right.base_register());
    }

    // the result replaces the value in 'loaded', a partial sum goes through a
    // scratch register
    //   x * 10  =>  t = x << 2; x = t + x; x = x << 1
    auto emit_shift_add_sequence(const token& src_loc_tk, const size_t indent,
                                 const loaded_destination& loaded,
                                 const digit_sequence& sequence) -> void {

        // a single digit needs only the final shift
        const operand partial{
            sequence.lowest == sequence.top
                ? operand{}
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        std::string_view shifted{loaded.value.base_register()};
        int pending_shift{};

        for (size_t bit{sequence.top}; bit != sequence.lowest;) {
            --bit;
            ++pending_shift;

            if (sequence.digits.at(bit) == 0) {
                continue;
            }

            assembler_.slli(indent, partial.base_register(), shifted,
                            pending_shift);

            const std::string_view sum{bit == sequence.lowest
                                           ? loaded.value.base_register()
                                           : partial.base_register()};

            assembler_.register_op(
                indent, sequence.digits.at(bit) < 0 ? op::sub : op::add, sum,
                partial.base_register(), loaded.value.base_register());

            shifted = partial.base_register();
            pending_shift = 0;
        }

        // the zero bits below the lowest digit
        if (sequence.lowest != 0) {
            assembler_.slli(indent, loaded.value.base_register(),
                            loaded.value.base_register(), sequence.lowest);
        }

        if (sequence.negate) {
            assembler_.sub(indent, loaded.value.base_register(), "zero",
                           loaded.value.base_register());
        }
    }

    // a register that is zero exactly when the operands are equal
    auto equality_tested_register(const size_t indent,
                                  const std::string_view result,
                                  const operand& left, const operand& right,
                                  const std::optional<int64_t> immediate)
        -> std::string_view {

        // an immediate zero can be tested without transforming the input
        if (immediate.has_value() and *immediate == 0) {
            return left.base_register();
        }

        // nonzero small constants fit directly in xori
        if (immediate.has_value()) {
            assembler_.xori(indent, result, left.base_register(), *immediate);
            return result;
        }

        if (register_index(right.base_register()) == 0) {
            return left.base_register();
        }

        if (register_index(left.base_register()) == 0) {
            return right.base_register();
        }

        // neither operand is zero and the constant did not fit
        assembler_.xor_op(indent, result, left.base_register(),
                          right.base_register());

        return result;
    }

    // a jump grown beyond 1 MiB needs a register without a live value
    [[nodiscard]] auto far_jump_register() const -> std::string_view {
        for (const size_t index : scratch_registers_) {
            if ((unavailable_registers_ & (uint32_t{1} << index)) == 0) {
                return register_names_.at(index);
            }
        }

        return {};
    }

    [[nodiscard]] auto is_register_allocated(const std::string_view name) const
        -> bool {

        return (unavailable_registers_ & register_mask(name)) != 0;
    }

    // variables and frames start word aligned so a direct offset from their
    // base has a known position within a word
    [[nodiscard]] auto is_word_based(const operand& address) const -> bool {

        // an index register holds a value unknown at compile time
        if (not address.is_memory() or not address.index_register().empty()) {
            return false;
        }

        const size_t base{register_index(address.base_register())};

        const bool is_variables_base{variables_base_reserved_ and
                                     base == s0_register_index};

        const bool is_frame_base{frame_base_reserved_ and
                                 base == register_index(frame_base_register())};

        // other bases such as loaded pointers or bulk registers may hold any
        // address
        return is_variables_base or is_frame_base;
    }

    [[nodiscard]] auto load_destination(const token& src_loc_tk,
                                        const size_t indent,
                                        const operand& destination)
        -> loaded_destination {

        if (destination.is_register()) {
            return {
                .address{},
                .value{destination},
            };
        }

        // lowering before allocating keeps the scratch register order
        const operand address{lower_address(src_loc_tk, indent, destination)};

        const operand value{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        copy_value(src_loc_tk, indent, value, address);

        return {
            .address{address},
            .value{value},
        };
    }

    // writes 'destination' to a0 and 'source' to a1 without losing an input
    auto load_helper_arguments(const token& src_loc_tk, const size_t indent,
                               const operand& destination,
                               const operand& source,
                               const operand& staged_destination,
                               const operand& staged_source) -> void {

        const operand first_argument{operand::reg("a0", default_type())};
        const operand second_argument{operand::reg("a1", default_type())};

        // stack operands were staged before the save area moved sp
        if (not staged_destination.is_empty()) {
            copy_value(src_loc_tk, indent, first_argument, staged_destination);
            copy_value(src_loc_tk, indent, second_argument, staged_source);

            return;
        }

        // the source does not need the old a0 so no staging is necessary
        if (not uses_register(source, "a0")) {
            copy_value(src_loc_tk, indent, first_argument, destination);
            copy_value(src_loc_tk, indent, second_argument, source);

            return;
        }

        // consume the source before loading the destination into a0
        if (not uses_register(destination, "a1")) {
            copy_value(src_loc_tk, indent, second_argument, source);
            copy_value(src_loc_tk, indent, first_argument, destination);

            return;
        }

        // neither argument can be written first without losing an input
        const operand staged{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        copy_value(src_loc_tk, indent, staged, source);
        copy_value(src_loc_tk, indent, first_argument, destination);
        copy_value(src_loc_tk, indent, second_argument, staged);
        free_scratch_register(src_loc_tk, indent, staged);
    }

    auto load_into(const token& src_loc_tk, const size_t indent,
                   const operand& value, const operand& src) -> void {

        if (src.is_memory()) {
            // a load may build its address in the register it will overwrite
            const operand lowered{
                lower_address(src_loc_tk, indent, src, value)};

            assembler_.load(indent, load_op(src.type_ref()),
                            value.base_register(), lowered.displacement(),
                            lowered.base_register());

            return;
        }

        if (src.is_register() and register_index(value.base_register()) ==
                                      register_index(src.base_register())) {
            return;
        }

        // adding zero copies a register without changing its bits
        if (src.is_register()) {
            assembler_.addi(indent, value.base_register(), src.base_register(),
                            0);

            return;
        }

        // li materializes a constant using one or more RV32I instructions
        if (src.is_immediate()) {
            assembler_.li(
                indent, value.base_register(),
                assembler_rv32i::immediate::of_symbol(src.immediate()));

            return;
        }

        throw compiler_exception{src_loc_tk, "invalid RV32I copy source"};
    }

    // lower base + index * scale + displacement to register + signed 12-bit
    // offset
    [[nodiscard]] auto
    lower_address(const token& src_loc_tk, const size_t indent,
                  const operand& address, const operand& destination = {})
        -> operand {

        validate_address(src_loc_tk, address);

        // no index: encode the offset directly or materialize base + offset
        if (address.index_register().empty()) {
            return lower_address_without_index(src_loc_tk, indent, address,
                                               destination);
        }

        // indexed memory operand: [base + index * scale + displacement];
        // combine the register terms before applying the displacement
        const operand result{
            address_result_register(src_loc_tk, indent, address, destination)};

        // unit scale: combine the base and index without multiplication
        if (address.scale() == 1) {
            return lower_address_unscaled_index(src_loc_tk, indent, address,
                                                result);
        }

        // scaled index: form the product before adding the base and offset
        return lower_address_scaled_index(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto lower_address_offset(const token& src_loc_tk,
                                            const size_t indent,
                                            const operand& address,
                                            const operand& result) -> operand {

        const int64_t offset{address.displacement()};

        const std::string& result_name{result.base_register()};

        const address_offset_parts parts{split_address_offset(offset)};

        // no upper part: the memory instruction handles the entire offset
        if (parts.upper == 0) {
            return operand::mem(result_name, {}, 1, parts.low,
                                address.type_ref());
        }

        const operand displacement{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        assembler_.lui(indent, displacement.base_register(), parts.upper);

        assembler_.add(indent, result_name, result_name,
                       displacement.base_register());

        free_scratch_register(src_loc_tk, indent, displacement);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    [[nodiscard]] auto
    lower_address_scaled_index(const token& src_loc_tk, const size_t indent,
                               const operand& address, const operand& result)
        -> operand {

        const std::string& base{address.base_register()};
        const std::string& result_name{result.base_register()};

        assembler_.slli(indent, result_name, address.index_register(),
                        std::countr_zero(address.scale()));

        // the preserved base can now be added to the scaled index
        assembler_.add(indent, result_name, result_name, base);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto
    lower_address_unscaled_index(const token& src_loc_tk, const size_t indent,
                                 const operand& address, const operand& result)
        -> operand {

        const std::string& base{address.base_register()};
        const std::string& index{address.index_register()};
        const std::string& result_name{result.base_register()};

        // one add combines register inputs, even when result aliases either
        assembler_.add(indent, result_name, base, index);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto lower_address_without_index(const token& src_loc_tk,
                                                   const size_t indent,
                                                   const operand& address,
                                                   const operand& destination)
        -> operand {

        const std::string& base{address.base_register()};
        const int64_t offset{address.displacement()};

        const address_offset_parts parts{split_address_offset(offset)};

        // no upper part: the memory instruction handles the entire offset
        if (parts.upper == 0) {
            return operand::mem(base, {}, 1, parts.low, address.type_ref());
        }

        const operand result{
            address_result_register(src_loc_tk, indent, address, destination)};

        const std::string& result_name{result.base_register()};

        // only the upper part; the memory instruction adds the low

        assembler_.lui(indent, result_name, parts.upper);

        assembler_.add(indent, result_name, result_name, base);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    // the factor is a known constant, keep only the bits that fit in the
    // product's type before choosing how to multiply
    auto multiply_by_constant(const token& src_loc_tk, const size_t indent,
                              const operand& product, const operand& factor,
                              const int32_t constant) -> void {

        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};

        const size_t bits{product.type_ref().size_bytes() * 8};

        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            (register_bits - bits)};

        const uint32_t multiplier{static_cast<uint32_t>(constant) & mask};

        // constant zero and one need no multiplication machinery
        if (multiplier == 0) {
            store_constant_result(src_loc_tk, indent, product, 0);
            return;
        }

        if (multiplier == 1) {
            return;
        }

        // all low bits set is multiplication by minus one at this width
        if (multiplier == mask) {
            unary(indent, '-', product);
            return;
        }

        // a power of two requires only a shift
        if (std::has_single_bit(multiplier)) {
            shift(src_loc_tk, indent, '<', product,
                  operand::imm(std::format("{}", std::countr_zero(multiplier)),
                               default_type()));

            return;
        }

        multiply_by_shifts_and_adds(src_loc_tk, indent, product, factor,
                                    multiplier, bits);
    }

    // the remaining constant needs shifts and adds; keep the original value
    // for the additions while the result changes
    auto
    multiply_by_shifts_and_adds(const token& src_loc_tk, const size_t indent,
                                const operand& product, const operand& factor,
                                const uint32_t multiplier, const size_t bits)
        -> void {

        const address_scope scope{*this, product, factor};

        // the original value stays here until the last add or sub, which
        // writes the result in its place so no copy is needed
        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, product)};

        const digit_sequence sequence{make_digit_sequence(multiplier, bits)};

        emit_shift_add_sequence(src_loc_tk, indent, loaded, sequence);

        store_operation_result(indent, product, loaded.address, loaded.value,
                               true);
    }

    // a register holding the result after the saved registers are restored
    auto preserved_helper_result(const token& src_loc_tk, const size_t indent,
                                 const operand& result, const operand& staged,
                                 const std::span<const std::string_view> saved)
        -> operand {

        const bool restored{std::ranges::find(saved, result.base_register()) !=
                            saved.end()};

        if (staged.is_empty() and not restored) {
            return result;
        }

        // reuse stack staging or protect a result register being restored
        const operand kept{
            staged.is_empty()
                ? alloc_scratch_register(src_loc_tk, indent, default_type())
                : staged};

        copy_value(src_loc_tk, indent, kept, result);

        return kept;
    }

    // named and scratch allocations share one lifo stack
    auto record_allocation(const token& src_loc_tk, const size_t indent,
                           const size_t index, const bool named) -> void {

        unavailable_registers_ |= uint32_t{1} << index;
        allocations_.push_back({
            .register_index{index},
            .source_location{src_loc_tk},
            .indent{indent},
            .named{named},
        });
    }

    // the pointers advance together so both starts decide the loop width
    auto record_bulk_address_start(const operand& address) -> void {
        bulk_addresses& addresses{bulk_addresses_.back()};
        addresses.starts.at(addresses.count) = start_of(address, 1);
        ++addresses.count;
    }

    auto release_bulk(const token& src_loc_tk, const size_t indent) -> void {
        for (const operand& reg :
             bulk_registers_.back() | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
        bulk_registers_.pop_back();
        bulk_addresses_.pop_back();
    }

    auto reserve_io_call_register(const token& src_loc_tk, const size_t indent,
                                  const operand& dst, const operand& descriptor,
                                  const operand& address, const operand& count)
        -> operand {

        constexpr size_t word_size{4};
        for (const operand* value : {&dst, &descriptor, &address, &count}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == word_size);
        }

        assert(register_index(dst.base_register()) == register_index("a0"));
        assert(register_index(descriptor.base_register()) ==
               register_index("a0"));
        assert(register_index(address.base_register()) == register_index("a1"));
        assert(register_index(count.base_register()) == register_index("a2"));

        // selects the system call or receives the return address
        return alloc_named_register(src_loc_tk, indent, "a7", default_type());
    }

    auto restore_saved_registers(const size_t indent,
                                 const std::span<const std::string_view> saved,
                                 const size_t stack_bytes) const -> void {

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.lw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }

        // restore sp before writing a possibly stack-relative destination
        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp", stack_bytes);
        }
    }

    // an aligned stack area is allocated only when a register is saved, the
    // returned size lets 'restore_saved_registers' free it
    auto save_registers(const size_t indent,
                        const std::span<const std::string_view> saved) const
        -> size_t {

        constexpr size_t stack_alignment{16};
        const size_t stack_bytes{align_storage_size(
            saved.size() * word_size_bytes_, stack_alignment)};

        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp",
                            -static_cast<int64_t>(stack_bytes));
        }

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.sw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }

        return stack_bytes;
    }

    [[nodiscard]] auto scratch_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count(allocations_, false, &allocation::named));
    }

    // a narrow register shifts to the top and back, extending in one pair
    auto shift_by_constant(const size_t indent, const char operation,
                           const operand& dst, const loaded_destination& loaded,
                           const uint32_t shift_count, const size_t bits)
        -> void {

        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};

        if (operation == '<' and bits < register_bits and dst.is_register()) {
            assembler_.slli(indent, loaded.value.base_register(),
                            loaded.value.base_register(),
                            register_bits - bits + shift_count);

            assembler_.immediate_op(indent, extend_shift_op(dst.type_ref()),
                                    loaded.value.base_register(),
                                    loaded.value.base_register(),
                                    register_bits - bits);

            store_operation_result(indent, dst, loaded.address, loaded.value,
                                   false);

            return;
        }

        // known counts already have rv32's five-bit shift semantics applied
        assembler_.immediate_op(indent, operation == '<' ? op::slli : op::srai,
                                loaded.value.base_register(),
                                loaded.value.base_register(), shift_count);

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               operation == '<');
    }

    auto shift_by_register(const token& src_loc_tk, const size_t indent,
                           const char operation, const operand& dst,
                           const operand& count,
                           const loaded_destination& loaded) -> void {

        const operand amount{source_register(src_loc_tk, indent, dst, count,
                                             loaded.value, std::nullopt)};

        assembler_.register_op(indent, operation == '<' ? op::sll : op::sra,
                               loaded.value.base_register(),
                               loaded.value.base_register(),
                               amount.base_register());

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               operation == '<');
    }

    // 'left' already holds the source when both name the same memory
    auto source_register(const token& src_loc_tk, const size_t indent,
                         const operand& destination, const operand& src,
                         const operand& left,
                         const std::optional<int32_t> constant) -> operand {

        if (same_memory(destination, src)) {
            return left;
        }

        if (src.is_register()) {
            return src;
        }

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        if (constant.has_value()) {
            assembler_.li(indent, right.base_register(), *constant);
            return right;
        }

        copy_value(src_loc_tk, indent, right, src);

        return right;
    }

    [[nodiscard]] auto start_of(const operand& address,
                                const size_t alignment) const -> access_start {

        // without a known base only the type alignment holds, from offset 0
        if (not is_word_based(address)) {
            return {
                .alignment{bulk_width(alignment)},
                .phase{},
            };
        }

        // the low bits of a negative displacement give the same phase
        return {
            .alignment{word_size_bytes_},
            .phase{static_cast<size_t>(address.displacement()) %
                   word_size_bytes_},
        };
    }

    auto store_byte_parts(const token& src_loc_tk, const size_t indent,
                          const std::span<const byte_part> parts,
                          const operand& dst, const size_t size_bytes) -> void {

        const operand address{
            unrolled_address(src_loc_tk, indent, dst, size_bytes)};

        const bool has_load{std::ranges::any_of(
            parts, [](const byte_part& p) -> bool { return p.needs_load; })};

        const operand value{has_load ? alloc_scratch_register(
                                           src_loc_tk, indent, default_type())
                                     : operand{}};

        for (const byte_part& p : parts) {
            const int64_t displacement{address.displacement() +
                                       static_cast<int64_t>(p.offset)};

            if (p.value == 0) {
                assembler_.store(indent, store_op(p.size_bytes), "zero",
                                 displacement, address.base_register());
                continue;
            }

            if (p.needs_load) {
                assembler_.li(indent, value.base_register(), p.value);
            }

            assembler_.store(indent, store_op(p.size_bytes),
                             value.base_register(), displacement,
                             address.base_register());
        }
    }

    auto store_constant_result(const token& src_loc_tk, const size_t indent,
                               const operand& destination,
                               const int32_t constant) -> void {

        const address_scope scope{*this, destination, operand{}};

        const operand address{
            destination.is_memory()
                ? lower_address(src_loc_tk, indent, destination)
                : operand{}};

        if (destination.is_memory() and constant == 0) {

            store_operation_result(indent, destination, address,
                                   operand::reg("zero", default_type()), false);

            return;
        }

        const operand value{working_register(src_loc_tk, indent, destination)};

        assembler_.li(indent, value.base_register(), constant);
        store_operation_result(indent, destination, address, value, false);
    }

    auto store_operation_result(const size_t indent, const operand& destination,
                                const operand& address, const operand& value,
                                const bool normalize) const -> void {

        const size_t width{destination.type_ref().size_bytes()};
        if (destination.is_memory()) {
            assembler_.store(indent, store_op(width), value.base_register(),
                             address.displacement(), address.base_register());

        } else if (width < 4 and normalize) {
            const size_t shift{32 - (width * 8)};

            assembler_.slli(indent, value.base_register(),
                            value.base_register(), shift);

            assembler_.immediate_op(
                indent, extend_shift_op(destination.type_ref()),
                value.base_register(), value.base_register(), shift);
        }
    }

    // keeps an unrolled access of 'size_bytes' within the load/store offset
    // range
    [[nodiscard]] auto unrolled_address(const token& src_loc_tk,
                                        const size_t indent,
                                        const operand& address,
                                        const size_t size_bytes) -> operand {

        operand lowered{lower_address(src_loc_tk, indent, address)};
        if (lowered.displacement() + static_cast<int64_t>(size_bytes) - 1 >
            immediate_max) {

            const operand pointer{
                alloc_scratch_register(src_loc_tk, indent, default_type())};

            address_of(src_loc_tk, indent, pointer, lowered);
            lowered = operand::mem(pointer.base_register(), {}, 1, 0,
                                   address.type_ref());
        }

        // one return keeps the copy elided
        return lowered;
    }

    // the value compared with the limit, the index or the end 'index + count'
    [[nodiscard]] auto
    upper_bound_top(const token& src_loc_tk, const size_t indent,
                    const std::string_view index, const operand& reg_count,
                    const operand& limit, const bool lower_checked)
        -> std::string {

        // a negative index is left to the lower check, which has already
        // failed it when enabled
        if (reg_count.is_empty() and not lower_checked) {
            assembler_.bltz(indent, index, "2f");
            return std::string{index};
        }

        if (reg_count.is_empty()) {
            return std::string{index};
        }

        const operand sum{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const std::string& top{sum.base_register()};

        // after the lower checks both are below 2^31, so the sum cannot wrap
        if (lower_checked) {
            assembler_.add(indent, top, index, reg_count.base_register());
            return top;
        }

        // the sign and carry bits form the high word of the widened sum, which
        // decides ends outside the 32-bit range
        const operand high{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        assembler_.srai(indent, high.base_register(), index, sign_shift_);
        assembler_.srai(indent, limit.base_register(),
                        reg_count.base_register(), sign_shift_);
        assembler_.add(indent, high.base_register(), high.base_register(),
                       limit.base_register());
        assembler_.add(indent, top, index, reg_count.base_register());
        assembler_.sltu(indent, limit.base_register(), top, index);
        assembler_.add(indent, high.base_register(), high.base_register(),
                       limit.base_register());
        assembler_.bltz(indent, high.base_register(), "2f");
        assembler_.bgtz(indent, high.base_register(), "1f");

        return top;
    }

    // a register destination holds the result directly, memory needs a scratch
    [[nodiscard]] auto working_register(const token& src_loc_tk,
                                        const size_t indent,
                                        const operand& destination) -> operand {

        if (destination.is_register()) {
            return destination;
        }

        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    //
    // statics
    //

    [[nodiscard]] static auto
    aligned_part_count(const size_t size_bytes,
                       const std::span<const access_start> starts) -> size_t {

        size_t count{};
        for_each_aligned_part(
            size_bytes, starts,
            [&count](const size_t, const size_t) -> void { ++count; });

        return count;
    }

    // 'copy' of a constant takes 'la' and a load and a store for each of its
    // 'copy_part_count' parts, above the unroll threshold it loops
    [[nodiscard]] static auto
    are_immediates_smaller(const std::span<const byte_part> parts,
                           const size_t size_bytes,
                           const size_t copy_part_count) -> bool {

        if (size_bytes > copy_unroll_threshold_bytes_) {
            return false;
        }

        const size_t copy_size_bytes{
            assembler_rv32i::two_instructions_bytes +
            (copy_part_count * assembler_rv32i::two_instructions_bytes)};

        size_t immediates_size_bytes{};
        for (const byte_part& p : parts) {
            immediates_size_bytes += assembler_rv32i::one_instruction_bytes;
            if (p.needs_load) {
                immediates_size_bytes +=
                    assembler_rv32i::li_value_size_bytes(p.value);
            }
        }

        return immediates_size_bytes <= copy_size_bytes;
    }

    // aligned hardware requires every access to be within the known alignment:
    // 1 selects 'lbu'/'sb', 2 'lhu'/'sh', 4 and above 'lw'/'sw'
    [[nodiscard]] static auto bulk_width(const size_t alignment) -> size_t {
        return std::min(alignment, word_size_bytes_);
    }

    [[nodiscard]] static auto
    can_reuse_address_destination(const operand& address,
                                  const operand& destination) -> bool {

        // no destination register is available to reuse
        if (not destination.is_register()) {
            return false;
        }

        const size_t dst_index{register_index(destination.base_register())};

        // zero discards writes, so it cannot hold the computed address
        if (dst_index == 0) {
            return false;
        }

        const size_t base_index{register_index(address.base_register())};

        // if the destination is the base register, loading the offset would
        // erase the base value before the add
        if (address.index_register().empty()) {
            return dst_index != base_index;
        }

        // if the destination is the base register, shifting the index into it
        // would erase the base value before the add
        if (dst_index == base_index and address.scale() != 1) {
            return false;
        }

        return true;
    }

    // runs of equal widths, e.g. '1 + 2 + 4 x 4 + 1 B'
    [[nodiscard]] static auto
    describe_parts(const size_t size_bytes,
                   const std::span<const access_start> starts) -> std::string {

        std::vector<std::pair<size_t, size_t>> runs;
        for_each_aligned_part(
            size_bytes, starts,
            [&runs](const size_t width, const size_t) -> void {
                if (not runs.empty() and runs.back().first == width) {
                    ++runs.back().second;
                    return;
                }

                runs.emplace_back(width, 1);
            });

        std::string text;
        for (const auto& [width, count] : runs) {
            if (not text.empty()) {
                text += " + ";
            }

            text += count == 1 ? std::format("{}", width)
                               : std::format("{} x {}", width, count);
        }

        return text + " B";
    }

    [[nodiscard]] static auto describe_start(const access_start& start)
        -> std::string {

        if (start.phase != 0) {
            return std::format("{} B past a word boundary", start.phase);
        }

        if (start.alignment == word_size_bytes_) {
            return "word aligned";
        }

        return std::format("{}-byte aligned", start.alignment);
    }

    // the shift that extends the high bits of a narrow value, zero for bool
    [[nodiscard]] static auto extend_shift_op(const type& value_type) -> op {
        return value_type.name() == "bool" ? op::srli : op::srai;
    }

    // each access is the widest that fits the remaining bytes and is aligned
    // for every address, so an unaligned start takes a byte and a halfword up
    // to the word boundary, then words, then a halfword and byte tail; from a
    // phase of 0 this gives the same parts as 'for_each_part'
    static auto for_each_aligned_part(
        const size_t size_bytes, const std::span<const access_start> starts,
        const std::function_ref<void(size_t part_size_bytes, size_t offset)>
            emit_part) -> void {

        size_t offset{};
        while (offset < size_bytes) {
            size_t width{word_size_bytes_};
            while (width > 1 and (width > size_bytes - offset or
                                  not is_aligned_at(starts, offset, width))) {
                width /= 2;
            }

            emit_part(width, offset);
            offset += width;
        }
    }

    [[nodiscard]] static auto format_address(const operand& address)
        -> std::string {

        std::string text{address.base_register()};
        if (not address.index_register().empty()) {
            if (not text.empty()) {
                text += " + ";
            }
            text += address.index_register();
            if (address.scale() > 1) {
                text += std::format(" * {}", address.scale());
            }
        }

        if (text.empty()) {
            text = std::format("{}", address.displacement());
        } else if (address.displacement() < 0) {
            const uint64_t magnitude{
                uint64_t{} - static_cast<uint64_t>(address.displacement())};

            text += std::format(" - {}", magnitude);
        } else if (address.displacement() > 0) {
            text += std::format(" + {}", address.displacement());
        }

        return text;
    }

    [[nodiscard]] static auto has_all_bits(const int32_t constant,
                                           const size_t width) -> bool {

        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            ((4 - width) * 8)};

        return (static_cast<uint32_t>(constant) & mask) == mask;
    }

    // the register-immediate form of 'add', 'and', 'or' and 'xor', 'sub' adds
    // the negated immediate
    [[nodiscard]] static auto immediate_form(const op operation) -> op {
        if (operation == op::and_op) {
            return op::andi;
        }

        if (operation == op::or_op) {
            return op::ori;
        }

        if (operation == op::xor_op) {
            return op::xori;
        }

        return op::addi;
    }

    [[nodiscard]] static auto immediate_value(const operand& value)
        -> std::optional<int32_t> {

        const std::optional<uint64_t> bits{immediate_bits(value)};
        if (not bits) {
            return std::nullopt;
        }

        return std::bit_cast<int32_t>(static_cast<uint32_t>(*bits));
    }

    [[nodiscard]] static auto
    is_aligned_at(const std::span<const access_start> starts,
                  const size_t offset, const size_t width) -> bool {

        return std::ranges::all_of(
            starts, [offset, width](const access_start& s) -> bool {
                return width <= s.alignment and (s.phase + offset) % width == 0;
            });
    }

    [[nodiscard]] static auto is_register(const std::string_view name) -> bool {
        return register_index(name) != register_names_.size();
    }

    // 'x op constant' is 'x': all bits for 'and', zero for the others
    [[nodiscard]] static auto keeps_destination(const op instruction,
                                                const int32_t constant,
                                                const size_t width) -> bool {

        if (instruction == op::and_op) {
            return has_all_bits(constant, width);
        }

        return constant == 0;
    }

    // lb and lh sign-extend, a bool is zero-extended
    [[nodiscard]] static auto load_op(const type& value_type) -> op {
        if (value_type.size_bytes() == 4) {
            return op::lw;
        }

        if (value_type.size_bytes() == 2) {
            return op::lh;
        }

        if (value_type.name() == "bool") {
            return op::lbu;
        }

        return op::lb;
    }

    [[nodiscard]] static auto make_digit_sequence(const uint32_t multiplier,
                                                  const size_t bits)
        -> digit_sequence {

        std::array<int, multiplier_digit_count> digits{
            multiplier_digits(multiplier)};

        // a digit at the product width vanishes modulo the width, leaving a
        // negative multiplier that is cheaper to build positive then negate
        const bool negate{digits.at(bits) != 0};
        if (negate) {
            digits.at(bits) = 0;
            for (int& digit : digits) {
                digit = -digit;
            }
        }

        // the leading nonzero digit is now plus one and starts the result
        size_t top{digits.size() - 1};
        while (digits.at(top) == 0) {
            --top;
        }

        size_t lowest{};
        while (digits.at(lowest) == 0) {
            ++lowest;
        }

        return {
            .digits{digits},
            .negate{negate},
            .top{top},
            .lowest{lowest},
        };
    }

    // non-adjacent form turns a run of set bits into one subtraction, e.g. 7
    // as 8 - 1, so each run costs one shift and add instead of one per bit
    [[nodiscard]] static auto multiplier_digits(const uint32_t multiplier)
        -> std::array<int, multiplier_digit_count> {

        std::array<int, multiplier_digit_count> digits{};
        uint64_t remaining{multiplier};
        for (size_t i{}; remaining != 0; ++i) {
            if ((remaining & 1U) == 0) {
                remaining >>= 1U;
                continue;
            }

            // remainder 3 modulo 4 is inside a run of set bits
            const bool in_run{(remaining & 3U) == 3};
            digits.at(i) = in_run ? -1 : 1;
            remaining = in_run ? remaining + 1 : remaining - 1;
            remaining >>= 1U;
        }

        return digits;
    }

    // the value as a register of 'value_type' holds it: truncated to the width,
    // then sign-extended unless it is a bool
    [[nodiscard]] static auto narrow_constant(const int32_t constant,
                                              const type& value_type)
        -> int32_t {

        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};
        const size_t bits{value_type.size_bytes() * 8};

        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            (register_bits - bits)};

        uint32_t value{static_cast<uint32_t>(constant) & mask};

        if (value_type.name() != "bool" and
            (value & (uint32_t{1} << (bits - 1))) != 0) {

            value |= ~mask;
        }

        return std::bit_cast<int32_t>(value);
    }

    // comparisons convert the right operand to the left operand's width
    [[nodiscard]] static auto narrowed_immediate(const operand& value,
                                                 const type& width_type)
        -> std::optional<int32_t> {

        const std::optional<int32_t> constant{immediate_value(value)};
        if (not constant.has_value()) {
            return std::nullopt;
        }

        return narrow_constant(*constant, width_type);
    }

    // whether the result may have bits outside the destination width
    [[nodiscard]] static auto
    needs_normalize(const op instruction, const operand& destination,
                    const operand& src, const std::optional<int32_t> constant)
        -> bool {

        // sums can carry out of the width
        if (instruction == op::add or instruction == op::sub) {
            return true;
        }

        const type& dst_type{destination.type_ref()};

        // bitwise results stay in range when the source representation does
        if (not constant.has_value()) {
            return src.type_ref().size_bytes() > dst_type.size_bytes() or
                   (src.type_ref().name() == "bool") !=
                       (dst_type.name() == "bool");
        }

        if (dst_type.name() == "bool") {
            return *constant < 0 or
                   std::cmp_greater(*constant,
                                    std::numeric_limits<uint8_t>::max());
        }

        const int64_t limit{static_cast<int64_t>(
            uint64_t{1} << ((dst_type.size_bytes() * 8) - 1))};

        return *constant < -limit or *constant >= limit;
    }

    [[nodiscard]] static auto register_index(const std::string_view name)
        -> size_t {

        return assembler_rv32i::register_number(name).value_or(
            register_names_.size());
    }

    [[nodiscard]] static auto register_mask(const std::string_view name)
        -> uint32_t {

        const size_t index{register_index(name)};
        return index == register_names_.size() ? 0 : uint32_t{1} << index;
    }

    // a narrow register gets high bits from a wider, immediate or
    // differently extended source
    [[nodiscard]] static auto register_needs_extension(const type& dst_type,
                                                       const operand& src)
        -> bool {

        const size_t dst_size_bytes{dst_type.size_bytes()};
        const size_t src_size_bytes{src.type_ref().size_bytes()};

        const bool extension_differs{(src.type_ref().name() == "bool") !=
                                     (dst_type.name() == "bool")};

        return dst_size_bytes < 4 and
               (src.is_immediate() or src_size_bytes > dst_size_bytes or
                (extension_differs and src_size_bytes == dst_size_bytes));
    }

    [[nodiscard]] static auto same_memory(const operand& left,
                                          const operand& right) -> bool {

        const auto same_register = [](const std::string_view first,
                                      const std::string_view second) -> bool {
            return first == second or
                   (is_register(first) and
                    register_index(first) == register_index(second));
        };

        return left.is_memory() and right.is_memory() and
               left.type_ref().name() == right.type_ref().name() and
               same_register(left.base_register(), right.base_register()) and
               same_register(left.index_register(), right.index_register()) and
               left.scale() == right.scale() and
               left.displacement() == right.displacement();
    }

    [[nodiscard]] static auto split_address_offset(const int64_t offset)
        -> address_offset_parts {

        constexpr unsigned low_bits{12};
        constexpr uint32_t low_mask{0xfff};
        constexpr int32_t low_range{4096};

        // keep the signed low 12 bits in the memory operand; subtracting
        // them from the offset leaves the upper part to load with lui
        int32_t low{
            static_cast<int32_t>(static_cast<uint32_t>(offset) & low_mask)};

        if (low > immediate_max) {
            low -= low_range;
        }

        return {
            .upper{static_cast<uint32_t>(offset - low) >> low_bits},
            .low{low},
        };
    }

    // the widest aligned parts like the unrolled 'copy'
    [[nodiscard]] static auto split_bytes(const std::string_view bytes,
                                          const access_start& start)
        -> std::vector<byte_part> {

        std::vector<byte_part> parts;
        int64_t loaded_value{};
        for_each_aligned_part(
            bytes.size(), std::span{&start, 1},
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const int64_t value{
                    little_endian_value(bytes.substr(offset, part_size_bytes))};

                const bool needs_load{value != 0 and value != loaded_value};
                if (needs_load) {
                    loaded_value = value;
                }

                parts.push_back({
                    .offset{offset},
                    .size_bytes{part_size_bytes},
                    .value{value},
                    .needs_load{needs_load},
                });
            });

        return parts;
    }

    // stores the low 8, 16 or 32 bits
    [[nodiscard]] static auto store_op(const size_t width) -> op {
        if (width == 4) {
            return op::sw;
        }

        if (width == 2) {
            return op::sh;
        }

        return op::sb;
    }

    // loads without sign extension, for copying bytes unchanged
    [[nodiscard]] static auto unsigned_load_op(const size_t width) -> op {
        if (width == 4) {
            return op::lw;
        }

        if (width == 2) {
            return op::lhu;
        }

        return op::lbu;
    }

    // aliases and address registers carry the same argument dependencies
    [[nodiscard]] static auto uses_register(const operand& value,
                                            const std::string_view name)
        -> bool {

        return (value.is_register() or value.is_memory()) and
               (register_index(value.base_register()) == register_index(name) or
                (value.is_memory() and register_index(value.index_register()) ==
                                           register_index(name)));
    }

    static auto validate_address(const token& src_loc_tk,
                                 const operand& address) -> void {

        if (not address.is_memory()) {
            throw compiler_exception{src_loc_tk,
                                     "RV32I requires a memory address"};
        }

        constexpr int64_t limit{std::numeric_limits<uint32_t>::max()};
        if (address.displacement() < -limit or address.displacement() > limit) {
            throw compiler_exception{
                src_loc_tk, "address offset exceeds RV32I address range"};
        }

        if (not is_register(address.base_register())) {
            throw compiler_exception{src_loc_tk, "invalid RV32I base register"};
        }

        if (not address.index_register().empty() and
            not is_register(address.index_register())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I index register"};
        }

        if (not address.index_register().empty() and
            address.scale() > UINT32_MAX) {
            throw compiler_exception{src_loc_tk,
                                     "index scale exceeds RV32I address range"};
        }
    }

    // the result is written in place
    static auto validate_destination_storage(const token& src_loc_tk,
                                             const operand& dst,
                                             const std::string_view operation)
        -> void {

        if (not(dst.is_register() or dst.is_memory())) {
            throw compiler_exception{
                src_loc_tk,
                std::format("invalid RV32I {} destination", operation)};
        }

        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }
    }

    static auto validate_scalar(const token& src_loc_tk, const type& value_type)
        -> void {

        if (not value_type.is_builtin() or
            (value_type.size_bytes() != 1 and value_type.size_bytes() != 2 and
             value_type.size_bytes() != 4)) {
            throw compiler_exception{
                src_loc_tk, "RV32I requires an 8-, 16-, or 32-bit scalar"};
        }
    }

    // 'x op constant' is 'constant': and-ing zero or or-ing all bits
    [[nodiscard]] static auto yields_constant(const op instruction,
                                              const int32_t constant,
                                              const size_t width) -> bool {

        return (instruction == op::and_op and constant == 0) or
               (instruction == op::or_op and has_all_bits(constant, width));
    }
};
