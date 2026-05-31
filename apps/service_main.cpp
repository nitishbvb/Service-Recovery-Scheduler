#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <string>

namespace fs = std::filesystem;

// Utility execution tool to read active metrics out from cross-process system registries
void displayCurrentExternalState(const fs::path& mailboxDir, const std::string& serviceName) {
    fs::path stateFile = mailboxDir / (serviceName + ".state");
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Allow server execution delay gaps

    if (!fs::exists(stateFile)) {
        std::cout << "[Query Link] No history profile file generated yet for: " << serviceName << "\n";
        return;
    }

    std::ifstream in(stateFile);
    std::string configLine;
    std::cout << "\n=========================================\n";
    std::cout << " LIVE SCHEDULER STATE FOR: " << serviceName << "\n";
    std::cout << "=========================================\n";
    while (std::getline(in, configLine)) {
        std::cout << "  " << configLine << "\n";
    }
    std::cout << "=========================================\n";
}

int main() {
    fs::path mailboxDir = fs::current_path() / "ipc_mailbox";
    std::string identityName = "auth-service";

    std::cout << "[Service Client Process] Starting system logic processing arrays...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- TRIGGER FAILURE SEQUENCE EVENT 1 ---
    std::cout << "\n[Local Crash] Encountered file lock error! Dropping alert token 1...\n";
    {
        std::ofstream file(mailboxDir / (identityName + ".fail"));
    } // Closes file handle explicitly to flush block state

    displayCurrentExternalState(mailboxDir, identityName);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- TRIGGER FAILURE SEQUENCE EVENT 2 ---
    std::cout << "\n[Local Crash] Network thread timeout! Dropping alert token 2...\n";
    {
        std::ofstream file(mailboxDir / (identityName + ".fail"));
    }

    displayCurrentExternalState(mailboxDir, identityName);

    return 0;
}
