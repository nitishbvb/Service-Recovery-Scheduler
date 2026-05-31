#include <gtest/gtest.h>
#include <scheduler.hpp>

class SchedulerInternalFixture : public ::testing::Test {
protected:
    RecoveryScheduler engine;
    const std::string testNode = "database-shard";

    void SetUp() override {
        std::vector<RecoveryAction> pathway = {
            RecoveryAction::RESTART,
            RecoveryAction::STOP,
            RecoveryAction::DISABLE
        };
        engine.registerService(testNode, pathway);
    }
};

TEST_F(SchedulerInternalFixture, ValidatesSequentialLevelEscalations) {
    // Failure Step 1 -> Should read RESTART
    auto res1 = engine.processFailure(testNode);
    ASSERT_TRUE(res1.has_value());
    EXPECT_EQ(*res1, RecoveryAction::RESTART);

    // Failure Step 2 -> Should translate to STOP
    auto res2 = engine.processFailure(testNode);
    ASSERT_TRUE(res2.has_value());
    EXPECT_EQ(*res2, RecoveryAction::STOP);
}

TEST_F(SchedulerInternalFixture, ValidatesSaturationCappingLimits) {
    engine.processFailure(testNode); // Index 0 (RESTART)
    engine.processFailure(testNode); // Index 1 (STOP)
    engine.processFailure(testNode); // Index 2 (DISABLE)

    auto resCap = engine.processFailure(testNode);
    ASSERT_TRUE(resCap.has_value());
    EXPECT_EQ(*resCap, RecoveryAction::RESTART);
}

TEST_F(SchedulerInternalFixture, GracefullyRejectsUnregisteredTargets) {
    auto badOutcome = engine.processFailure("missing-unknown-node");
    EXPECT_FALSE(badOutcome.has_value());
}
