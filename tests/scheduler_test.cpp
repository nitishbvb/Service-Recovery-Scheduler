#include <gtest/gtest.h>
#include <scheduler.hpp>
#include <thread>
#include <chrono>

class SchedulerCoreFixture : public ::testing::Test {
protected:
    RecoveryScheduler scheduler;
    const std::string testService = "compute-node";
    const std::string targetAddr = "127.0.0.1:9091";

    void SetUp() override {
        // Set up generic test timeline route configuration array
        std::vector<RecoveryAction> sequence = {
            RecoveryAction::RESTART,
            RecoveryAction::RESTART,
            RecoveryAction::DISABLE
        };
        scheduler.registerService(testService, targetAddr, sequence);
    }
};

// 1. Verifies linear escalation pathways through continuous notifications
TEST_F(SchedulerCoreFixture, VerifyLinearEscalationSequence) {
    // Failure 1: Level 0 -> Should map to RESTART
    auto step1 = scheduler.processFailure(testService);
    ASSERT_TRUE(step1.has_value());
    EXPECT_EQ(step1->first, targetAddr);
    EXPECT_EQ(step1->second, RecoveryAction::RESTART);

    // Failure 2: Level 1 -> Should map to RESTART again
    auto step2 = scheduler.processFailure(testService);
    ASSERT_TRUE(step2.has_value());
    EXPECT_EQ(step2->second, RecoveryAction::RESTART);

    // Failure 3: Level 2 -> Should escalate out to DISABLE
    auto step3 = scheduler.processFailure(testService);
    ASSERT_TRUE(step3.has_value());
    EXPECT_EQ(step3->second, RecoveryAction::DISABLE);
}

// 2. Asserts saturation bounds work correctly when actions overflow
TEST_F(SchedulerCoreFixture, VerifySaturationAtSequenceBoundary) {
    scheduler.processFailure(testService); // 0
    scheduler.processFailure(testService); // 1
    scheduler.processFailure(testService); // 2 (DISABLE)

    // Failure 4: Beyond index allocations -> Should cap and maintain final element behavior
    auto stepOverflow = scheduler.processFailure(testService);
    ASSERT_TRUE(stepOverflow.has_value());
    EXPECT_EQ(stepOverflow->second, RecoveryAction::DISABLE);
}

// 3. Validates protection mechanisms when processing query exceptions
TEST_F(SchedulerCoreFixture, RejectUnregisteredServiceRequests) {
    auto badOutcome = scheduler.processFailure("invalid-missing-service");
    EXPECT_FALSE(badOutcome.has_value());
}


