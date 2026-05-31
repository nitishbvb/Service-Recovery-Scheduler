#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <string_view>
#include <span>

enum class RecoveryAction { RESTART, STOP, DISABLE };

struct ServiceMetadata {
    std::string targetAddress;
    std::vector<RecoveryAction> sequence;
};

struct LiveState {
    size_t currentIndex{0};
};

class RecoveryScheduler {
public:
    RecoveryScheduler() = default;
    
    // Disable copying to enforce strict architectural integrity
    RecoveryScheduler(const RecoveryScheduler&) = delete;
    RecoveryScheduler& operator=(const RecoveryScheduler&) = delete;

    void registerService(std::string_view name, std::string_view address, std::span<const RecoveryAction> seq);
    std::optional<std::pair<std::string, RecoveryAction>> processFailure(std::string_view name);
    std::optional<LiveState> queryServiceState(std::string_view name) const;

private:
    std::unordered_map<std::string, ServiceMetadata> m_configs;
    std::unordered_map<std::string, LiveState> m_states;
    mutable std::shared_mutex m_mutex;
};
