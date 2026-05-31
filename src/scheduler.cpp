#include "scheduler.hpp"
#include <mutex>

void RecoveryScheduler::registerService(std::string_view name, std::string_view address, std::span<const RecoveryAction> seq) {
    std::unique_lock lock(m_mutex); // Writer Lock
    
    ServiceMetadata meta{
        .targetAddress = std::string(address),
        .sequence = std::vector<RecoveryAction>(seq.begin(), seq.end())
    };
    m_configs[std::string(name)] = std::move(meta);
}

std::optional<std::pair<std::string, RecoveryAction>> RecoveryScheduler::processFailure(std::string_view name) {
    std::unique_lock lock(m_mutex); // Writer Lock
    
    std::string key(name);
    auto configIt = m_configs.find(key);
    if (configIt == m_configs.end() || configIt->second.sequence.empty()) {
        return std::nullopt;
    }

    const auto& config = configIt->second;
    auto& state = m_states[key]; // Automatically creates state if missing

    size_t indexToExecute = state.currentIndex;
    RecoveryAction action = config.sequence[indexToExecute];

    // Escalate to next index, capping at final element sequence boundaries
    if (state.currentIndex + 1 < config.sequence.size()) {
        state.currentIndex++;
    }

    return std::make_pair(config.targetAddress, action);
}

std::optional<LiveState> RecoveryScheduler::queryServiceState(std::string_view name) const {
    std::shared_lock lock(m_mutex); // Reader Lock (Allows simultaneous queries)
    
    auto it = m_states.find(std::string(name));
    if (it != m_states.end()) {
        return it->second;
    }
    return std::nullopt;
}
