#pragma once

#include "stmt_def_type_field.hpp"

class stmt_def_type final : public statement {
    token name_tk_;
    token ws_; // whitespace
    std::vector<stmt_def_type_field> fields_;
    type type_;

  public:
    stmt_def_type(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, name_tk_{tz.next_token()} {

        if (not tz.is_next_char('{')) {
            throw compiler_exception(
                name_tk_, "expected '{' to begin declaration of type");
        }

        while (true) {
            // read field definition with next token being name
            fields_.emplace_back(tc, tz.next_token(), tz);
            if (tz.is_next_char('}')) {
                break;
            }
            if (not tz.is_next_char(',')) {
                throw compiler_exception(tz, "expected ',' and more fields");
            }
        }
        // initiate the type definitions
        type_.set_name(name_tk_.name());
        // add the fields
        for (const stmt_def_type_field& fld : fields_) {
            // get type of field. no type name means default
            const type& tp{
                fld.type_str().empty()
                    ? tc.get_type_default()
                    : tc.get_type_or_throw(fld.tok(), fld.type_str())};
            type_.add_field(fld.tok(), fld.name(), tp);
        }
        tc.add_type(name_tk_, type_);
    }

    stmt_def_type() = default;
    stmt_def_type(const stmt_def_type&) = default;
    stmt_def_type(stmt_def_type&&) = default;
    auto operator=(const stmt_def_type&) -> stmt_def_type& = default;
    auto operator=(stmt_def_type&&) -> stmt_def_type& = default;

    ~stmt_def_type() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        os << '{';
        ws_.source_to(os);
        size_t i{};
        for (const stmt_def_type_field& fld : fields_) {
            if (i++) {
                os << ',';
            }
            fld.source_to(os);
        }
        os << '}';
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {}
};
