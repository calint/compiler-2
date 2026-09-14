#pragma once
// reviewed: 2025-09-28

#include "stmt_def_type_field.hpp"
#include "type.hpp"
#include <vector>

class stmt_def_type final : public statement {
    token name_tk_;
    token open_brace_tk_;
    std::vector<stmt_def_type_field> fields_;
    std::vector<token> fields_delim_tk_;
    token close_brace_tk_;
    type type_;

  public:
    stmt_def_type(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()},
          open_brace_tk_{tz.is_next_char_token('{')} {

        if (open_brace_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '{' to begin declaration of type"};
        }

        while (true) {
            // read field definition with the next token being the name
            fields_.emplace_back(tc, tz.next_token(), tz);
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                break;
            }
            const token t{tz.is_next_char_token(',')};
            if (t.is_empty()) {
                throw compiler_exception{
                    tz, std::format("expected ',' followed by another field "
                                    "in type '{}'",
                                    name_tk_.text())};
            }
            fields_delim_tk_.emplace_back(t);
        }
        // initiate the type definitions
        type_.set_name(name_tk_.text());

        // add the fields
        for (const stmt_def_type_field& fld : fields_) {
            // get the type of field. no type name means default
            const type& tp{
                fld.type_str().empty()
                    ? tc.get_type_default()
                    : tc.get_type_or_throw(fld.type_token(), fld.type_str())};

            type_.add_field(fld.tok(), fld.name(), tp, fld.is_array(),
                            fld.array_size());
        }

        tc.add_type(name_tk_, type_);
    }

    stmt_def_type() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        open_brace_tk_.source_to(os);
        if (not fields_.empty()) {
            fields_.front().source_to(os);
            for (const auto [d, e] : std::views::zip(
                     fields_delim_tk_, fields_ | std::views::drop(1))) {
                d.source_to(os);
                e.source_to(os);
            }
        }
        close_brace_tk_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        const type& tp{tc.get_type_or_throw(tok(), name_tk_.text())};

        tc.comment_start(tok(), os, indent);
        std::println(os, "{} : {} B    fields:", name_tk_.text(), tp.size());

        tc.comment_start(tok(), os, indent);
        std::println(os, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}", "name",
                     "offset", "size", "array?", "array size");

        for (const type_field& f : tp.fields()) {
            tc.comment_start(tok(), os, indent);
            std::println(os, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}", f.name,
                         f.offset, f.size, f.is_array ? "yes" : "no",
                         f.is_array ? std::to_string(f.array_size) : "");
        }
        std::println(os);
    }
};
