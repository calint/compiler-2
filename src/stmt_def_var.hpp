#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_identifier.hpp"
#include "type.hpp"
#include <format>

class null_stream : public std::ostream {
    class null_buffer : public std::streambuf {
      public:
        auto overflow(int c) -> int override { return c; }
    } nb_{};

  public:
    null_stream() : std::ostream(&nb_) {}
};

class stmt_def_var final : public statement {
    token name_tk_;
    token type_tk_;
    token array_size_tk_;
    stmt_assign_var assign_var_;
    size_t array_size_{};
    token ws1_;
    token ws2_;
    bool is_array_{};

  public:
    stmt_def_var(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, name_tk_{tz.next_token()} {

        // check if type declared
        if (tz.is_next_char(':')) {
            type_tk_ = tz.next_token();
            if (tz.is_next_char('[')) {
                array_size_tk_ = tz.next_token();
                is_array_ = true;
                if (const std::optional<int64_t> value{
                        toc::parse_to_constant(array_size_tk_.name())};
                    value) {
                    array_size_ = static_cast<size_t>(*value);
                } else {
                    throw compiler_exception{array_size_tk_,
                                             "expected array size as constant"};
                }
                if (not tz.is_next_char(']')) {
                    throw compiler_exception{type_tk_,
                                             "expected array size and ']'"};
                }
                ws2_ = tz.next_whitespace_token();
            }
        }

        // get type reference from token
        const type& tp{type_tk_.name().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.name())};
        set_type(tp);

        const bool init_required{array_size_tk_.is_empty()};

        // expect initialization
        if (init_required and not tz.is_next_char('=')) {
            throw compiler_exception(tz,
                                     "expected '=' and initializer after type");
        }

        ws1_ = tz.next_whitespace_token();

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        tc.add_var(name_tk_, null_strm, 0, name_tk_.name(), tp, is_array_,
                   array_size_);

        if (init_required) {
            stmt_identifier si{tc, tz, name_tk_};
            assign_var_ = {tc, tz, std::move(si)};
        }
    }

    stmt_def_var() = default;
    stmt_def_var(const stmt_def_var&) = default;
    stmt_def_var(stmt_def_var&&) = default;
    auto operator=(const stmt_def_var&) -> stmt_def_var& = default;
    auto operator=(stmt_def_var&&) -> stmt_def_var& = default;

    ~stmt_def_var() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (not type_tk_.is_empty()) {
            os << ':';
            type_tk_.source_to(os);
            if (not array_size_tk_.is_empty()) {
                os << '[';
                array_size_tk_.source_to(os);
                os << ']';
                ws2_.source_to(os);
            }
        }
        if (array_size_tk_.is_empty()) {
            os << '=';
            ws1_.source_to(os);
            assign_var_.expression().source_to(os);
        }
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.add_var(name_tk_, os, indent, name_tk_.name(), get_type(), is_array_,
                   array_size_);

        tc.comment_source(*this, os, indent);
        if (not is_array_) {
            assign_var_.compile(tc, os, indent, name_tk_.name());
            return;
        }

        // zero out the array
        // ; RDI = destination pointer
        // ; RCX = number of bytes
        // xor eax, eax        ; Zero out RAX (value to store)
        // rep stosb           ; Repeat store byte [RDI] = AL, RCX times

        const ident_info& dst_info{
            tc.make_ident_info(name_tk_, name_tk_.name(), false)};

        toc::indent(os, indent, true);
        os << "clear array " << array_size_ << " * " << dst_info.type_ref.size()
           << " B = " << array_size_ * dst_info.type_ref.size() << " B\n";

        tc.alloc_named_register_or_throw(*this, os, indent, "rdi");
        tc.alloc_named_register_or_throw(*this, os, indent, "rcx");
        tc.alloc_named_register_or_throw(*this, os, indent, "rax");

        tc.asm_cmd(tok(), os, indent, "lea", "rdi",
                   std::format("[rsp - {}]", -dst_info.stack_ix));
        // note: -dst_info.stack_ix_rel_rsp for nicer source formatting, is
        //       always negative
        tc.asm_cmd(tok(), os, indent, "mov", "rcx",
                   std::format("{}", array_size_ * dst_info.type_ref.size()));
        tc.asm_cmd(name_tk_, os, indent, "xor", "rax", "rax");
        toc::asm_rep_stosb(name_tk_, os, indent);

        tc.free_named_register(os, indent, "rax");
        tc.free_named_register(os, indent, "rcx");
        tc.free_named_register(os, indent, "rdi");
    }
};
