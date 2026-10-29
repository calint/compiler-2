#pragma once

// the source files of a program and where a token is in them

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <string>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "token.hpp"

// a line of the source, 'begin' is the index of its first character
struct source_line {
    size_t begin{};
    std::string_view text;
};

// the line of the character at 'index' without its line end, the character can
// be the line end itself
[[nodiscard]] inline auto line_at(const std::string_view src,
                                  const size_t index) -> source_line {

    const size_t before_index{
        index == 0 ? std::string_view::npos : src.rfind('\n', index - 1),
    };

    const size_t begin{
        before_index == std::string_view::npos ? 0 : before_index + 1,
    };
    // note: +1 because the line starts after the line end

    const size_t end{std::min(src.find('\n', index), src.size())};

    return {
        .begin{begin},
        .text{src.substr(begin, end - begin)},
    };
}

// the column is 1-based, at a line end the one after its last character
[[nodiscard]] inline auto
column_for_char_index(const size_t char_index_in_source,
                      const std::string_view src) -> size_t {

    assert(char_index_in_source <= src.size());

    return char_index_in_source - line_at(src, char_index_in_source).begin + 1;
    // note: +1 because the columns start at 1
}

// the source files of a program, the first is the main file; the text stays
// where it is when files are added, tokens refer to it
class source_files final {
    struct file {
        std::string name;
        std::string text;
    };

    std::deque<file> files_;

  public:
    // returns the index of the file
    auto add(std::string name, std::string text) -> size_t {
        files_.push_back({
            .name{std::move(name)},
            .text{std::move(text)},
        });

        return files_.size() - 1;
    }

    [[nodiscard]] auto count() const -> size_t { return files_.size(); }

    [[nodiscard]] auto name(const size_t file_ix) const -> std::string_view {
        return files_.at(file_ix).name;
    }

    [[nodiscard]] auto text(const size_t file_ix) const -> std::string_view {
        return files_.at(file_ix).text;
    }

    //
    // statics
    //

    [[nodiscard]] static auto read_file(const std::string_view file_name)
        -> std::string {

        // a source is far smaller, the limit stops a file that never ends, e.g.
        // a device, from using all the memory
        constexpr size_t max_source_size_bytes{size_t{256} * 1024 * 1024};
        constexpr size_t read_chunk_size_bytes{0x10000};

        std::ifstream fs{std::string{file_name}};

        if (not fs.is_open()) {
            throw unlocated_error{
                std::format("cannot open file {}", in_quotes(file_name))};
        }

        std::string contents;
        std::array<char, read_chunk_size_bytes> chunk{};

        while (
            fs.read(chunk.data(), static_cast<std::streamsize>(chunk.size())) or
            fs.gcount() > 0) {

            contents.append(chunk.data(), static_cast<size_t>(fs.gcount()));

            if (contents.size() > max_source_size_bytes) {
                throw unlocated_error{std::format("file {} is larger than {} B",
                                                  in_quotes(file_name),
                                                  max_source_size_bytes)};
            }
        }

        // a directory opens but cannot be read
        if (fs.bad()) {
            throw unlocated_error{
                std::format("cannot read file {}", in_quotes(file_name))};
        }

        return contents;
    }
};

// where a token is in the source, as numbers or as text
class source_locations final {
    std::reference_wrapper<const source_files> files_;

  public:
    explicit source_locations(const source_files& files) : files_{files} {}

    [[nodiscard]] auto for_comment(const token& src_loc_tk) const
        -> std::string {

        // note: a line end in a file name would end the comment
        return std::format("{}:{}",
                           printable(relative_name(src_loc_tk.file_index())),
                           text(src_loc_tk, ':'));
    }

    // where an error is, 'file:line:column', always with the file name
    [[nodiscard]] auto for_error(const source_span& where) const
        -> std::string {

        const std::string_view src{files_.get().text(where.file_ix)};

        return std::format(
            "{}:{}:{}", printable(files_.get().name(where.file_ix)), where.line,
            column_for_char_index(where.start_index, src));
    }

    // the location in the main file has no file name, a label stays the same
    // for a program of one file
    [[nodiscard]] auto for_label(const token& src_loc_tk) const -> std::string {
        if (src_loc_tk.file_index() == 0) {
            return text(src_loc_tk, '.');
        }

        return std::format("f{}.{}", src_loc_tk.file_index(),
                           text(src_loc_tk, '.'));
    }

    // like 'for_comment', without the file name for a program of one file,
    // e.g. in the register report
    [[nodiscard]] auto for_listing(const token& src_loc_tk) const
        -> std::string {

        if (files_.get().count() == 1) {
            return text(src_loc_tk, ':');
        }

        return for_comment(src_loc_tk);
    }

    // a source location for a message, with the file name when the program
    // has more than one file
    [[nodiscard]] auto for_message(const token& src_loc_tk) const
        -> std::string {

        if (files_.get().count() == 1) {
            return text(src_loc_tk, ':');
        }

        return std::format(
            "{}:{}", printable(files_.get().name(src_loc_tk.file_index())),
            text(src_loc_tk, ':'));
    }

    // the file relative to the directory of the main file, e.g.
    // 'include/x.baz', so the text does not depend on where the compiler was
    // run from or how the main file was named
    [[nodiscard]] auto relative_name(const size_t file_ix) const
        -> std::string {

        const std::filesystem::path main_dir{
            absolute_path(files_.get().name(0)).parent_path(),
        };

        const std::filesystem::path src_path{
            absolute_path(files_.get().name(file_ix)),
        };

        return src_path.lexically_relative(main_dir).string();
    }

    [[nodiscard]] auto source_of(const token& src_loc_tk) const
        -> std::string_view {

        return files_.get().text(src_loc_tk.file_index());
    }

  private:
    [[nodiscard]] auto column(const token& src_loc_tk) const -> size_t {
        return column_for_char_index(
            src_loc_tk.start_index(),
            files_.get().text(src_loc_tk.file_index()));
    }

    // 'line' and 'column' of the token, 'separator' between them
    [[nodiscard]] auto text(const token& src_loc_tk, const char separator) const
        -> std::string {

        return std::format("{}{}{}", src_loc_tk.at_line(), separator,
                           column(src_loc_tk));
    }

    //
    // statics
    //

    // e.g. '/w/include/x.baz' of 'lib/../include/x.baz' run from '/w'
    [[nodiscard]] static auto absolute_path(const std::string_view name)
        -> std::filesystem::path {

        return std::filesystem::absolute(std::filesystem::path{name})
            .lexically_normal();
    }
};
