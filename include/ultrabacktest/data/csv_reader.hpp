#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Fast CSV Reader
//
// A fast, buffered CSV reader that works with TickDecoder to parse CSV rows
// into Event structs. Uses fread and in-place string_view parsing to avoid
// std::string allocations on the hot path.
// ─────────────────────────────────────────────────────────────────────────────

#include "data_reader.hpp"
#include "tick_decoder.hpp"

#include <cstdio>
#include <string_view>
#include <vector>
#include <stdexcept>

namespace ultrabacktest {

class CsvReader : public DataReader<CsvReader> {
public:
    explicit CsvReader(const TickDecoder& decoder) 
        : decoder_(decoder) 
    {}

    ~CsvReader() {
        close();
    }

    bool do_open(const std::string& path) {
        close();
#ifdef _WIN32
        auto err = fopen_s(&file_, path.c_str(), "rb");
        if (err != 0 || !file_) return false;
#else
        file_ = std::fopen(path.c_str(), "rb");
        if (!file_) return false;
#endif
        buffer_.resize(BUFFER_SIZE);
        buffer_pos_ = 0;
        buffer_end_ = 0;
        is_eof_ = false;

        // Skip header if present
        std::string_view header;
        read_line(header);
        
        return advance();
    }

    bool do_read_next(Event& out_event) {
        if (!has_next_) return false;
        out_event = next_event_;
        advance();
        return true;
    }

    [[nodiscard]] bool do_has_next() const noexcept {
        return has_next_;
    }

    void do_reset() {
        if (file_) {
            std::rewind(file_);
            buffer_pos_ = 0;
            buffer_end_ = 0;
            is_eof_ = false;
            
            // Skip header
            std::string_view header;
            read_line(header);
            
            advance();
        }
    }

private:
    void close() {
        if (file_) {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    bool advance() {
        has_next_ = false;
        std::string_view line;
        
        while (read_line(line)) {
            if (line.empty()) continue; // skip empty lines
            
            if (decoder_.decode_csv(line, next_event_)) {
                has_next_ = true;
                return true;
            }
            // If decode fails, log or throw depending on strictness.
            // For now, we skip malformed lines.
        }
        return false;
    }

    bool read_line(std::string_view& out_line) {
        if (buffer_pos_ >= buffer_end_ && is_eof_) {
            return false;
        }

        while (true) {
            // Find newline in current buffer
            const char* start = buffer_.data() + buffer_pos_;
            const char* end = buffer_.data() + buffer_end_;
            const char* nl = static_cast<const char*>(std::memchr(start, '\n', end - start));

            if (nl) {
                size_t len = nl - start;
                out_line = std::string_view(start, len);
                
                // Handle \r\n
                if (!out_line.empty() && out_line.back() == '\r') {
                    out_line.remove_suffix(1);
                }
                
                buffer_pos_ += len + 1;
                return true;
            }

            // No newline found. If EOF, yield remaining as last line
            if (is_eof_) {
                size_t len = end - start;
                if (len > 0) {
                    out_line = std::string_view(start, len);
                    buffer_pos_ += len;
                    return true;
                }
                return false;
            }

            // Need to read more data. Move remainder to front and refill.
            size_t rem = end - start;
            if (rem > 0 && buffer_pos_ > 0) {
                std::memmove(buffer_.data(), start, rem);
            }
            buffer_pos_ = 0;
            buffer_end_ = rem;

            // Grow buffer if line is longer than capacity (rare)
            if (buffer_end_ == buffer_.size()) {
                buffer_.resize(buffer_.size() * 2);
            }

            size_t bytes_read = std::fread(buffer_.data() + buffer_end_, 1, buffer_.size() - buffer_end_, file_);
            buffer_end_ += bytes_read;

            if (bytes_read == 0) {
                is_eof_ = true;
            }
        }
    }

    FILE* file_ = nullptr;
    TickDecoder decoder_;
    
    static constexpr size_t BUFFER_SIZE = 256 * 1024; // 256 KB chunks
    std::vector<char> buffer_;
    size_t buffer_pos_ = 0;
    size_t buffer_end_ = 0;
    bool is_eof_ = false;

    Event next_event_{};
    bool has_next_ = false;
};

} // namespace ultrabacktest
