#pragma once
#include <variant>
#include <cstdint>

struct TickData {
    double price;
    int64_t volume;
};

struct EZNewsData {
    double score;
};

using EventPayload = std::variant<TickData, EZNewsData>;

struct TradingEvent {
    int64_t timestamp;
    EventPayload data;
};

inline TradingEvent myEventFactory() {
    return {0, TickData{0.0, 0}};
};