#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <scheduler.hpp>

namespace fs = std::filesystem;

// Atomically serializes state out to disk using a standard temporary swap move
void broadcastLiveState(const fs::path& mailboxDir, std::string_view serviceName, const LiveState& state) {
    std::string filename = std::string(serviceName) + ".state";
    fs::path tempFile = mailboxDir / (filename + ".tmp");
    fs::path finalFile = mailboxDir / filename;

    std::ofstream out(tempFile, std::ios::trunc);
    if (out.is_open()) {
        out << "Current_Sequence_Level: " << state.currentIndex << "\n";
        out << "Last_Action_Executed: " << to_string(state.lastActionTaken) << "\n";
        out.close();

        std::error_code ec;
        fs::rename(tempFile, finalFile, ec); // Ensures atomic swap transaction on target systems
    }
}

int main() {
    RecoveryScheduler scheduler;

    // Services registered explicitly at startup phase 
    scheduler.registerService("auth-service", std::vector{
        RecoveryAction::RESTART, 
        RecoveryAction::RESTART, 
        RecoveryAction::DISABLE
    });
    
    scheduler.registerService("payment-gateway", std::vector{
        RecoveryAction::RESTART, 
        RecoveryAction::STOP
    });

    fs::path mailboxDir = fs::current_path() / "ipc_mailbox";
    fs::create_directories(mailboxDir);

    std::cout << "[Scheduler Server] Monitoring file-queue path: " << mailboxDir.string() << "\n";

    while (true) {
        std::error_code ec;
        if (fs::exists(mailboxDir, ec)) {
            for (const auto& entry : fs::directory_iterator(mailboxDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".fail") {
                    
                    std::string serviceName = entry.path().stem().string();
                    std::cout << "[Inbound Event] Failure token detected for: " << serviceName << "\n";

                    // 1. Dispatch signal into our core recovery library engine
                    auto activeAction = scheduler.processFailure(serviceName);

                    if (activeAction) {
                        std::cout << "[Action Order] Dispatched execution target: " << to_string(*activeAction) << "\n";
                        
                        // 2. Query updated tracking state and write structural state metrics back out
                        if (auto liveState = scheduler.queryServiceState(serviceName)) {
                            broadcastLiveState(mailboxDir, serviceName, *liveState);
                        }
                    } else {
                        std::cout << "[Error] Reported service '" << serviceName << "' lacks tracking metadata configs.\n";
                    }

                    // 3. Consume the command transaction by safely removing the inbound token flag
                    fs::remove(entry.path(), ec);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Restrict excessive loop spin-hammering
    }
    return 0;
}
