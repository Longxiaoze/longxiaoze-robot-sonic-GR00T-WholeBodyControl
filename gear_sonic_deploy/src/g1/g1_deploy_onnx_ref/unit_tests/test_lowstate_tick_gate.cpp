#include "gtest/gtest.h"

#include "../include/lowstate_tick_gate.hpp"

#include <cstdint>

using sonic::LowStateTickDecision;
using sonic::LowStateTickGate;

TEST(LowStateTickGate, PreservesPhaseAndSkipsDuplicateTicks) {
  LowStateTickGate gate;
  EXPECT_EQ(gate.Observe(0, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(gate.Observe(5, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(10, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(15, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(20, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(gate.Observe(20, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(40, 20), LowStateTickDecision::kDue);

  LowStateTickGate phase;
  EXPECT_EQ(phase.Observe(0, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(phase.Observe(10, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(phase.Observe(25, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(phase.anchor_ms, 20u);
  EXPECT_EQ(phase.Observe(35, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(phase.Observe(45, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(phase.anchor_ms, 40u);
}

TEST(LowStateTickGate, RunsOnceAfterLargeForwardJump) {
  LowStateTickGate gate;
  EXPECT_EQ(gate.Observe(45, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(gate.Observe(245, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(gate.anchor_ms, 245u);
  EXPECT_EQ(gate.last_advanced_periods, 10u);
  EXPECT_EQ(gate.discontinuities, 1u);
  EXPECT_EQ(gate.Observe(245, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(264, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(gate.Observe(265, 20), LowStateTickDecision::kDue);
}

TEST(LowStateTickGate, ReportsResetAndHandlesUint32Wrap) {
  LowStateTickGate reset;
  EXPECT_EQ(reset.Observe(1000, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(reset.Observe(0, 20), LowStateTickDecision::kRebased);
  EXPECT_EQ(reset.anchor_ms, 0u);
  EXPECT_EQ(reset.discontinuities, 1u);
  EXPECT_EQ(reset.Observe(19, 20), LowStateTickDecision::kWait);
  EXPECT_EQ(reset.Observe(20, 20), LowStateTickDecision::kDue);

  LowStateTickGate wrap;
  EXPECT_EQ(
      wrap.Observe(UINT32_MAX - 10u, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(wrap.Observe(9, 20), LowStateTickDecision::kDue);
  EXPECT_EQ(wrap.anchor_ms, 9u);
}

TEST(LowStateTickGate, RejectsZeroPeriod) {
  LowStateTickGate gate;
  EXPECT_THROW(gate.Observe(0, 0), std::invalid_argument);
}
