#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <string_view>
#include <span>

// 1. Minimum 3 supported distinct recovery actions
enum class RecoveryAction { 
    RESTART = 0, 
    STOP = 1, 
    DISABLE = 2 
};

// Helper utility to safely convert enum state values to string logs
inline std::string_view to_string(RecoveryAction action) {
    switch (action) {
        case RecoveryAction::RESTART: return "RESTART";
        case RecoveryAction::STOP:    return "STOP";
        case RecoveryAction::DISABLE: return "DISABLE";
    }
    return "UNKNOWN";
}

struct ServiceMetadata {
    std::vector<RecoveryAction> sequence;
};

struct LiveState {
    size_t currentIndex{0};
    RecoveryAction lastActionTaken{RecoveryAction::RESTART};
};

class RecoveryScheduler {
public:
    RecoveryScheduler() = default;
    
    // Disable copying explicitly to preserve system data pipeline integrity
    RecoveryScheduler(const RecoveryScheduler&) = delete;
    RecoveryScheduler& operator=(const RecoveryScheduler&) = delete;

    // Public Core API Operations
    void registerService(std::string_view name, std::span<const RecoveryAction> recoverySequence);
    std::optional<RecoveryAction> processFailure(std::string_view name);
    std::optional<LiveState> queryServiceState(std::string_view name) const;

private:
    std::unordered_map<std::string, ServiceMetadata> m_configs;
    std::unordered_map<std::string, LiveState> m_states;
    mutable std::shared_mutex m_mutex; // Thread safety manager
};
