#pragma once

#include <cstdint>
#include <stdexcept>

namespace sonic {

enum class LowStateTickDecision { kDue, kWait, kRebased };

/**
 * Phase-preserving rate gate for a uint32 millisecond simulation clock.
 *
 * Observe() never asks a caller to catch up more than once for one observed
 * state. It advances the anchor by all elapsed whole periods, retaining only
 * the fractional remainder. Normal uint32 wrap is handled by unsigned
 * subtraction; a larger backwards jump is treated as a simulator reset.
 */
struct LowStateTickGate {
  bool initialized = false;
  uint32_t anchor_ms = 0;
  uint32_t last_elapsed_ms = 0;
  uint32_t last_advanced_periods = 0;
  uint64_t accepted = 0;
  uint64_t skipped = 0;
  uint64_t discontinuities = 0;

  LowStateTickDecision Observe(uint32_t tick_ms, uint32_t period_ms) {
    if (period_ms == 0) {
      throw std::invalid_argument("LowState tick period must be positive");
    }
    last_advanced_periods = 0;
    if (!initialized) {
      initialized = true;
      anchor_ms = tick_ms;
      last_elapsed_ms = 0;
      accepted++;
      return LowStateTickDecision::kDue;
    }

    const uint32_t elapsed_ms = tick_ms - anchor_ms;
    last_elapsed_ms = elapsed_ms;
    const bool backwards_reset =
        tick_ms < anchor_ms && elapsed_ms > 0x80000000u;
    if (backwards_reset) {
      anchor_ms = tick_ms;
      skipped++;
      discontinuities++;
      return LowStateTickDecision::kRebased;
    }
    if (elapsed_ms < period_ms) {
      skipped++;
      return LowStateTickDecision::kWait;
    }

    last_advanced_periods = elapsed_ms / period_ms;
    anchor_ms += last_advanced_periods * period_ms;
    accepted++;
    if (last_advanced_periods > 4u) {
      discontinuities++;
    }
    return LowStateTickDecision::kDue;
  }
};

}  // namespace sonic
