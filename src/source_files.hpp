#pragma once

// the source files of a program and where a token is in them

#include <array>
#include <cstddef>
#include <deque>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "token.hpp"

// the column is 1-based, 0 when the index is at a line end or past the source
[[nodiscard]] inline auto
column_for_char_index(const size_t char_index_in_source,
                      const std::string_view src) -> size_t {

    if (char_index_in_source >= src.size() or
        src.at(char_index_in_source) == '\n') {

        return 0;
    }

    const size_t line_end_before{
        char_index_in_source == 0 ? std::string_view::npos
                                  : src.rfind('\n', char_index_in_source - 1),
    };

    if (line_end_before == std::string_view::npos) {
        return char_index_in_source + 1;
        // note: +1 because the columns start at 1
    }

    return char_index_in_source - line_end_before;
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
        // note: -1 because the file is the last one
    }

    [[nodiscard]] auto count() const -> size_t { return files_.size(); }

    [[nodiscard]] auto is_empty() const -> bool {
        return files_.empty() or files_.front().text.empty();
    }

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
            throw std::runtime_error{
                std::format("cannot open file '{}'", file_name)};
        }

        std::string contents;
        std::array<char, read_chunk_size_bytes> chunk{};

        while (
            fs.read(chunk.data(), static_cast<std::streamsize>(chunk.size())) or
            fs.gcount() > 0) {

            contents.append(chunk.data(), static_cast<size_t>(fs.gcount()));

            if (contents.size() > max_source_size_bytes) {
                throw std::runtime_error{
                    std::format("file '{}' is larger than {} B", file_name,
                                max_source_size_bytes)};
            }
        }

        // a directory opens but cannot be read
        if (fs.bad()) {
            throw std::runtime_error{
                std::format("cannot read file '{}'", file_name)};
        }

        return contents;
    }
};

// where a token is in the source, as numbers or as text
class source_locations final {
    const source_files* files_{};

  public:
    // without files the tokens have no location
    explicit source_locations(const source_files* const files)
        : files_{files} {}

    // the location in the main file has no file name, a label stays the same
    // for a program of one file
    [[nodiscard]] auto for_label(const token& src_loc_tk) const -> std::string {
        if (src_loc_tk.file_index() == 0) {
            return text(src_loc_tk, '.');
        }

        return std::format("f{}.{}", src_loc_tk.file_index(),
                           text(src_loc_tk, '.'));
    }

    [[nodiscard]] auto has_source() const -> bool {
        return files_ != nullptr and not files_->is_empty();
    }

    // human-readable source location, with the file name unless the file is
    // the main one
    [[nodiscard]] auto human_readable(const token& src_loc_tk) const
        -> std::string {

        if (src_loc_tk.file_index() == 0) {
            return text(src_loc_tk, ':');
        }

        return std::format("{}:{}", files_->name(src_loc_tk.file_index()),
                           text(src_loc_tk, ':'));
    }

    [[nodiscard]] auto line_and_column(const token& src_loc_tk) const
        -> std::pair<size_t, size_t> {

        const size_t column{
            column_for_char_index(src_loc_tk.start_index(),
                                  files_->text(src_loc_tk.file_index())),
        };

        return {src_loc_tk.at_line(), column};
    }

    [[nodiscard]] auto source_of(const token& src_loc_tk) const
        -> std::string_view {

        return files_->text(src_loc_tk.file_index());
    }

  private:
    // 'line' and 'column' of the token, 'separator' between them
    [[nodiscard]] auto text(const token& src_loc_tk, const char separator) const
        -> std::string {

        const auto [line, col]{line_and_column(src_loc_tk)};
        return std::format("{}{}{}", line, separator, col);
    }
};
