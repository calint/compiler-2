#pragma once
// reviewed: 2025-09-28

#include "stmt_def_type_field.hpp"
#include "type.hpp"

class stmt_def_type final : public statement {
    token name_tk_;
    token ws_; // whitespace
    std::vector<stmt_def_type_field> fields_;
    type type_;

  public:
    stmt_def_type(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (not tz.is_next_char('{')) {
            throw compiler_exception{
                name_tk_, "expected '{' to begin declaration of type"};
        }

        while (true) {
            // read field definition with the next token being the name
            fields_.emplace_back(tc, tz.next_token(), tz);
            if (tz.is_next_char('}')) {
                break;
            }
            if (not tz.is_next_char(',')) {
                throw compiler_exception{
                    tz, std::format("expected ',' and more fields in type '{}'",
                                    name_tk_.text())};
            }
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

    ~stmt_def_type() override = default;

    stmt_def_type() = default;
    stmt_def_type(const stmt_def_type&) = default;
    stmt_def_type(stmt_def_type&&) = default;
    auto operator=(const stmt_def_type&) -> stmt_def_type& = default;
    auto operator=(stmt_def_type&&) -> stmt_def_type& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        std::print(os, "{{");
        ws_.source_to(os);
        size_t i{};
        for (const stmt_def_type_field& fld : fields_) {
            if (i++) {
                std::print(os, ",");
            }
            fld.source_to(os);
        }
        std::print(os, "}}");
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        const type& tp{tc.get_type_or_throw(tok(), name_tk_.text())};

        tc.comment_start(tok(), os, indent);
        std::println(os, "{} : {} B    fields: ", name_tk_.text(), tp.size());

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
