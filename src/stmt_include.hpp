#pragma once

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>

#include "compile_error.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// 'include "lib.baz"': it parses the name of the file, 'program' loads the
// file and parses its definitions before the definitions of the file with
// the 'include'; the statement only keeps the source
class stmt_include final : public statement {
    token path_tk_;
    // the name as the file system takes it, its escapes decoded
    std::string file_name_;

  public:
    // 'tz' is after the 'include'
    stmt_include(const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, path_tk_{tz.next_token()} {

        if (not path_tk_.is_string()) {
            throw compile_error{
                path_tk_, "expected the file name in quotes after 'include'"};
        }

        // escapes are decoded like in any other string, e.g. '\x20'
        file_name_ = path_tk_.string_bytes();

        // e.g. a NUL would end the path early and a line end split the
        // messages that name the file
        if (std::ranges::any_of(file_name_, is_control_char)) {
            throw compile_error{path_tk_,
                                "the file name has a control character"};
        }
    }

    stmt_include() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        path_tk_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

    //
    // class methods
    //

    [[nodiscard]] auto file_name() const -> const std::string& {
        return file_name_;
    }

    [[nodiscard]] auto path_token() const -> const token& { return path_tk_; }
};
