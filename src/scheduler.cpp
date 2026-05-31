#include <scheduler.hpp>
#include <mutex>
#include <iostream>

void RecoveryScheduler::registerService(std::string_view name, std::span<const RecoveryAction> recoverySequence) {
    std::unique_lock lock(m_mutex); // Writer lock protection block
    
    ServiceMetadata meta{
        .sequence = std::vector<RecoveryAction>(recoverySequence.begin(), recoverySequence.end())
    };
    m_configs[std::string(name)] = std::move(meta);
    // generate code to print this m_Configs map to cout for debug visibility   
    for (const auto& [serviceName, serviceMeta] : m_configs) {
        std::cout << "[Config Register] Service: " << serviceName << " | Recovery Sequence: ";
        for (const auto& action : serviceMeta.sequence) {
            std::cout << to_string(action) << " ";
        }
        std::cout << "\n";
    }
}

std::optional<RecoveryAction> RecoveryScheduler::processFailure(std::string_view name) {
    std::unique_lock lock(m_mutex); // Writer lock protection block
    
    std::string key(name);
    auto configIt = m_configs.find(key);
    if (configIt == m_configs.end() || configIt->second.sequence.empty()) {
        return std::nullopt;
    }

    const auto& config = configIt->second;
    auto& state = m_states[key]; // Dynamically generates baseline tracker if blank

    size_t indexToExecute = state.currentIndex;
    RecoveryAction actionToTake = config.sequence[indexToExecute];
    state.lastActionTaken = actionToTake;

    // Escalate to next sequential index level up, capping safely at final barrier element
    if (state.currentIndex + 1 < config.sequence.size()) {
        state.currentIndex++;
    }

    return actionToTake;
}

std::optional<LiveState> RecoveryScheduler::queryServiceState(std::string_view name) const {
    std::shared_lock lock(m_mutex); // Reader lock protection (Allows multiple simultaneous queries)
    
    auto it = m_states.find(std::string(name));
    if (it != m_states.end()) {
        return it->second;
    }
    return std::nullopt;
}
