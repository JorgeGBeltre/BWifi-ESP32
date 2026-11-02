#include "Application/Services/CarouselKinematicsService.h"
#include <cmath>

namespace bwifi::app {

int CarouselKinematicsService::wrapIndex(int index, int totalOptions) {
    if (totalOptions <= 0) {
        return 0;
    }
    int wrapped = index % totalOptions;
    if (wrapped < 0) {
        wrapped += totalOptions;
    }
    return wrapped;
}

float CarouselKinematicsService::shortestRingDelta(float targetIndex, float currentPos, int totalOptions) {
    if (totalOptions <= 0) {
        return 0.0f;
    }
    const float totalF = static_cast<float>(totalOptions);
    const float halfF  = totalF * 0.5f;
    float diff = targetIndex - currentPos;
    while (diff > halfF) {
        diff -= totalF;
    }
    while (diff < -halfF) {
        diff += totalF;
    }
    return diff;
}

float CarouselKinematicsService::advancePosition(int targetIndex, float currentPos, int totalOptions, uint32_t dtMs) {
    if (totalOptions <= 0) {
        return 0.0f;
    }
    if (dtMs > 80) {
        dtMs = 80;
    }
    const float totalF = static_cast<float>(totalOptions);
    const float diff = shortestRingDelta(static_cast<float>(targetIndex), currentPos, totalOptions);

    if (std::fabs(diff) > 0.008f) {
        float stepFactor = 1.0f - std::exp(-static_cast<float>(dtMs) * 0.028f);
        if (stepFactor < 0.32f) {
            stepFactor = 0.32f;
        }
        if (stepFactor > 0.80f) {
            stepFactor = 0.80f;
        }
        currentPos += diff * stepFactor;
        while (currentPos >= totalF) {
            currentPos -= totalF;
        }
        while (currentPos < 0.0f) {
            currentPos += totalF;
        }
        return currentPos;
    }
    return static_cast<float>(targetIndex);
}

} // namespace bwifi::app

