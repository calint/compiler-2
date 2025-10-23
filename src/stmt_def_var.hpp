#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>

#include "compiler_exception.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_identifier.hpp"
#include "type.hpp"

class null_stream final : public std::ostream {
    class null_buffer : public std::streambuf {
      public:
        auto overflow(int c) -> int override { return c; }
    } nb_{};

  public:
    null_stream() : std::ostream(&nb_) {}
};

class stmt_def_var final : public statement {
    static constexpr size_t threshold_for_rep_stos{32};

    token name_tk_;
    token type_tk_;
    token array_size_tk_;
    std::unique_ptr<stmt_assign_var> assign_var_;
    size_t array_size_{};
    token ws1_;
    token ws2_;
    bool is_array_{};

  public:
    stmt_def_var(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        // check if type declared
        if (tz.is_next_char(':')) {
            type_tk_ = tz.next_token();
            if (tz.is_next_char('[')) {
                array_size_tk_ = tz.next_token();
                is_array_ = true;
                if (const std::optional<int64_t> value{
                        toc::parse_to_constant(array_size_tk_.text())};
                    value) {
                    array_size_ = static_cast<size_t>(*value);
                    if (array_size_ <= 0) {
                        throw compiler_exception{
                            array_size_tk_,
                            "array sizes must be greater than 0"};
                    }
                } else {
                    throw compiler_exception{
                        array_size_tk_,
                        "expected array size as a positive constant"};
                }
                if (not tz.is_next_char(']')) {
                    throw compiler_exception{type_tk_,
                                             "expected array size and ']'"};
                }
                ws2_ = tz.next_whitespace_token();
            }
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};
        set_type(tp);

        // expect initialization
        const bool init_required{tz.is_next_char('=')};

        ws1_ = tz.next_whitespace_token();

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr = &tp,
            .declared_at_tk{name_tk_},
            .is_array = is_array_,
            .array_size = array_size_,
        };
        tc.add_var(name_tk_, null_strm, 0, var);

        if (init_required) {
            stmt_identifier si{tc, {}, name_tk_, tz};
            assign_var_ =
                std::make_unique<stmt_assign_var>(tc, tz, std::move(si));
        }

        assert_var_not_used(name_tk_.text());
        // note: asserts that the newly defined variable isn't used in the
        //       initialization
    }

    ~stmt_def_var() override = default;

    stmt_def_var() = default;
    stmt_def_var(const stmt_def_var&) = delete;
    stmt_def_var(stmt_def_var&&) = default;
    auto operator=(const stmt_def_var&) -> stmt_def_var& = delete;
    auto operator=(stmt_def_var&&) -> stmt_def_var& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (not type_tk_.is_empty()) {
            std::print(os, ":");
            type_tk_.source_to(os);
            if (not array_size_tk_.is_empty()) {
                std::print(os, "[");
                array_size_tk_.source_to(os);
                std::print(os, "]");
                ws2_.source_to(os);
            }
        }
        if (assign_var_) {
            std::print(os, "=");
            ws1_.source_to(os);
            assign_var_->expression().source_to(os);
        }
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr = &get_type(),
            .declared_at_tk{name_tk_},
            .is_array = is_array_,
            .array_size = array_size_,
        };
        tc.add_var(name_tk_, os, indent, var);

        if (assign_var_) {
            assign_var_->compile(tc, os, indent, name_tk_.text());
            return;
        }

        // zero the variable data

        // zero out the array
        // ; RDI = destination pointer
        // ; RCX = number of bytes
        // xor eax, eax        ; Zero out RAX (value to store)
        // rep stosb           ; Repeat store byte [RDI] = AL, RCX times

        const ident_info& dst_info{
            tc.make_ident_info(name_tk_, name_tk_.text())};

        const size_t instance_count{array_size_ ? array_size_ : 1};
        const size_t bytes_count{instance_count * dst_info.type_ptr->size()};

        tc.comment_start(name_tk_, os, indent);
        std::println(os, "clear {} * {} B = {} B", instance_count,
                     dst_info.type_ptr->size(), bytes_count);

        if (bytes_count > threshold_for_rep_stos) {
            // try storing qwords
            const size_t qword_count{bytes_count / toc::size_qword};
            const size_t rest_bytes_count{bytes_count -
                                          (qword_count * toc::size_qword)};
            const size_t reps{rest_bytes_count == 0 ? qword_count
                                                    : bytes_count};
            const char rep_size{rest_bytes_count ? 'b' : 'q'};

            tc.alloc_named_register_or_throw(tok(), os, indent, "rcx");
            tc.alloc_named_register_or_throw(tok(), os, indent, "rdi");
            tc.alloc_named_register_or_throw(tok(), os, indent, "rax");

            tc.asm_cmd(tok(), os, indent, "mov", "rcx", std::to_string(reps));
            toc::asm_lea(tok(), os, indent, "rdi",
                         std::format("rsp - {}", -dst_info.stack_ix));
            // note: -dst_info.stack_ix_rel_rsp for nicer source formatting; is
            //       always negative
            tc.asm_cmd(name_tk_, os, indent, "xor", "rax", "rax");
            toc::asm_rep_stos(name_tk_, os, indent, rep_size);

            tc.free_named_register(tok(), os, indent, "rax");
            tc.free_named_register(tok(), os, indent, "rdi");
            tc.free_named_register(tok(), os, indent, "rcx");
            return;
        }

        // mov less than threshold for rep stos

        tc.comment_start(tok(), os, indent);
        std::println(os, "size < 32 B, use mov");

        size_t rest{bytes_count};
        int32_t displacement{dst_info.stack_ix};
        const size_t qword_movs{rest / toc::size_qword};
        const std::string size_qword{toc::get_size_specifier(toc::size_qword)};

        for (size_t i{}; i < qword_movs; i++) {
            tc.asm_cmd(tok(), os, indent, "mov",
                       std::format("{} [rsp - {}]", size_qword, -displacement),
                       "0");
            displacement += toc::size_qword;
            rest -= toc::size_qword;
        }

        // mov the reminder
        if ((rest / toc::size_dword) != 0) {
            tc.asm_cmd(tok(), os, indent, "mov",
                       std::format("{} [rsp - {}]",
                                   toc::get_size_specifier(toc::size_dword),
                                   -displacement),
                       "0");
            displacement += toc::size_dword;
            rest -= toc::size_dword;
        }

        if ((rest / toc::size_word) != 0) {
            tc.asm_cmd(tok(), os, indent, "mov",
                       std::format("{} [rsp - {}]",
                                   toc::get_size_specifier(toc::size_word),
                                   -displacement),
                       "0");
            displacement += toc::size_word;
            rest -= toc::size_word;
        }

        if (rest) {
            tc.asm_cmd(tok(), os, indent, "mov",
                       std::format("{} [rsp - {}]",
                                   toc::get_size_specifier(toc::size_byte),
                                   -displacement),
                       "0");
        }
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        if (assign_var_) {
            assign_var_->assert_var_not_used(var);
        }
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return name_tk_.text();
    }
};
