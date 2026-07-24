#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Binary Event Reader
//
// Reads raw Event structs directly from a binary file.
// In production, this would use memory mapping (mmap / MapViewOfFile) for
// zero-copy access. For cross-platform simplicity in v1, it uses a large
// chunked buffer with std::fread.
// ─────────────────────────────────────────────────────────────────────────────

#include "data_reader.hpp"
#include <cstdio>
#include <vector>

namespace ultrabacktest {

class BinaryReader : public DataReader<BinaryReader> {
public:
    BinaryReader() = default;

    ~BinaryReader() {
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
        buffer_.resize(EVENTS_PER_READ);
        buffer_idx_ = 0;
        buffer_count_ = 0;
        is_eof_ = false;
        
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
            buffer_idx_ = 0;
            buffer_count_ = 0;
            is_eof_ = false;
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
        if (buffer_idx_ >= buffer_count_) {
            if (is_eof_) {
                has_next_ = false;
                return false;
            }
            
            // Read next chunk
            buffer_count_ = std::fread(buffer_.data(), sizeof(Event), EVENTS_PER_READ, file_);
            buffer_idx_ = 0;
            
            if (buffer_count_ == 0) {
                is_eof_ = true;
                has_next_ = false;
                return false;
            }
            
            if (buffer_count_ < EVENTS_PER_READ) {
                is_eof_ = true;
            }
        }
        
        next_event_ = buffer_[buffer_idx_++];
        has_next_ = true;
        return true;
    }

    FILE* file_ = nullptr;
    
    // Read 64K events at a time (64K * 64B = 4MB buffer)
    static constexpr size_t EVENTS_PER_READ = 65536;
    std::vector<Event> buffer_;
    size_t buffer_idx_ = 0;
    size_t buffer_count_ = 0;
    bool is_eof_ = false;

    Event next_event_{};
    bool has_next_ = false;
};

} // namespace ultrabacktest
