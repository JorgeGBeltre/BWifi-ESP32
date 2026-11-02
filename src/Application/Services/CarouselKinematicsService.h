#pragma once

#include <cstdint>

namespace bwifi::app {

/**
 * @brief Pure application service computing circular menu wrapping and
 * frame-rate-independent exponential interpolation for the 60+ FPS OLED carousel.
 */
class CarouselKinematicsService {
public:
    static int wrapIndex(int index, int totalOptions);
    static float shortestRingDelta(float targetIndex, float currentPos, int totalOptions);
    static float advancePosition(int targetIndex, float currentPos, int totalOptions, uint32_t dtMs);
};

} // namespace bwifi::app

