#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Data Reader Interface
//
// CRTP base class for all data readers (CSV, Binary, Parquet).
// Avoids virtual dispatch overhead in the replay loop.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/events/event.hpp>
#include <string>

namespace ultrabacktest {

template <typename Derived>
class DataReader {
public:
    bool open(const std::string& path) {
        return static_cast<Derived*>(this)->do_open(path);
    }
    
    bool read_next(Event& out_event) {
        return static_cast<Derived*>(this)->do_read_next(out_event);
    }
    
    bool has_next() const {
        return static_cast<const Derived*>(this)->do_has_next();
    }
    
    void reset() {
        static_cast<Derived*>(this)->do_reset();
    }
};

} // namespace ultrabacktest
