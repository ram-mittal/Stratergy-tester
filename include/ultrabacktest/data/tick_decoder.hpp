#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UltraBacktest — Tick Decoder
//
// Converts raw data rows (CSV lines or binary blocks) into Event structs.
// Uses std::from_chars for zero-allocation, high-performance number parsing.
// ─────────────────────────────────────────────────────────────────────────────

#include <ultrabacktest/events/event.hpp>
#include <ultrabacktest/common/config.hpp>

#include <string_view>
#include <charconv>
#include <vector>
#include <unordered_map>

namespace ultrabacktest {

// Maps a string symbol (e.g. "AAPL") to an internal integer ID.
class SymbolMapper {
public:
    Symbol get_or_create(std::string_view name) {
        // We do string->string_view mapping safely if needed, but for now
        // just find or insert.
        std::string s(name);
        auto it = map_.find(s);
        if (it != map_.end()) return it->second;
        
        Symbol id = next_id_++;
        map_[s] = id;
        return id;
    }

private:
    std::unordered_map<std::string, Symbol> map_;
    Symbol next_id_ = 1;
};

// Generic CSV Decoder.
// Default mapping assumes columns: 
// 0: Timestamp (Nanos)
// 1: Symbol (String)
// 2: EventType (T=Trade, Q=Quote, A=Add, C=Cancel, M=Modify, E=Execution)
// 3: Price (double, parsed to fixed-point)
// 4: Qty (int)
// 5: Side (B=Buy, S=Sell)
// ...
class TickDecoder {
public:
    explicit TickDecoder(SymbolMapper* mapper) : mapper_(mapper) {}

    bool decode_csv(std::string_view line, Event& out_event) {
        // Very fast split by comma
        const char* p = line.data();
        const char* end = p + line.size();
        
        std::string_view cols[10];
        int num_cols = 0;
        
        while (p < end && num_cols < 10) {
            const char* next_comma = static_cast<const char*>(std::memchr(p, ',', end - p));
            if (next_comma) {
                cols[num_cols++] = std::string_view(p, next_comma - p);
                p = next_comma + 1;
            } else {
                cols[num_cols++] = std::string_view(p, end - p);
                break;
            }
        }
        
        if (num_cols < 6) return false;

        // 0: Timestamp (Nanos)
        Nanos ts = 0;
        if (auto [ptr, ec] = std::from_chars(cols[0].data(), cols[0].data() + cols[0].size(), ts); ec != std::errc()) {
            return false;
        }

        // 1: Symbol
        Symbol sym = mapper_ ? mapper_->get_or_create(cols[1]) : 0;

        // 2: EventType
        if (cols[2].empty()) return false;
        char type_char = cols[2][0];

        // 3: Price (parse double then to fixed-point)
        double price_dbl = 0.0;
        // In C++20, from_chars for double is available in modern compilers
        if (auto [ptr, ec] = std::from_chars(cols[3].data(), cols[3].data() + cols[3].size(), price_dbl); ec != std::errc()) {
            return false;
        }
        Price price = from_double(price_dbl);

        // 4: Qty
        Qty qty = 0;
        if (auto [ptr, ec] = std::from_chars(cols[4].data(), cols[4].data() + cols[4].size(), qty); ec != std::errc()) {
            return false;
        }

        // 5: Side
        Side side = Side::Buy;
        if (!cols[5].empty() && (cols[5][0] == 'S' || cols[5][0] == 's' || cols[5][0] == 'A')) {
            side = Side::Sell;
        }

        // Create the event based on type
        switch (type_char) {
            case 'T': // Trade
                out_event = Event::make_trade(ts, sym, price, qty, side);
                return true;
            case 'A': // Add Order (L3)
            {
                out_event = Event();
                out_event.timestamp = ts;
                out_event.symbol = sym;
                out_event.type = EventType::OrderAdd;
                out_event.order_add.price = price;
                out_event.order_add.qty = qty;
                out_event.order_add.side = side;
                out_event.order_add.order_id = 0; // usually in col 6
                return true;
            }
            default:
                // Not fully implemented for all types yet
                return false;
        }
    }

private:
    SymbolMapper* mapper_ = nullptr;
};

} // namespace ultrabacktest
