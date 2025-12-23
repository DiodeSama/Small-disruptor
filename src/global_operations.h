#pragma once
#include <atomic>
#include <iostream>

class KillSwitch {
public:
    static inline std::atomic<bool> global_kill_switch{false};

    static void trigger_kill() {
        global_kill_switch.store(true, std::memory_order_release);
        std::cerr << "[WARNING] Emergency Kill Switch Triggered!" << std::endl;
    }

    static bool is_killed() {
        return global_kill_switch.load(std::memory_order_acquire);
    }
};