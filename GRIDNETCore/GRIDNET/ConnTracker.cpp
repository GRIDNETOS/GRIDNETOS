#include "ConnTracker.h"

CConnTracker::CConnTracker()
{
}

void CConnTracker::pingState(const std::string& ipAddress, eConversationState::eConversationState state) {
    std::lock_guard<std::mutex> lock(mGuardian); // Protect shared resources

    // Generate a hash key for the IP address.
    auto hashKey = CCryptoFactory::getInstance()->getSHA2_256Vec(
        std::vector<uint8_t>(ipAddress.begin(), ipAddress.end())
    );

    std::time_t currentTime = std::time(nullptr);

    // Record the state change for accountability.
    mConnectionEvents[hashKey].push_back(ConnectionEvent{ state, currentTime });

    // Update active intervals based on the state.
    switch (state) {
    case eConversationState::running:
        // Connection has been established; start a new interval.
        mActiveConnections[hashKey].emplace_back(ConnectionInterval{ currentTime, std::nullopt });
        break;

    case eConversationState::ended:
        // Connection has ended; set end time of the most recent active interval.
        if (!mActiveConnections[hashKey].empty()) {
            for (auto it = mActiveConnections[hashKey].rbegin(); it != mActiveConnections[hashKey].rend(); ++it) {
                if (!it->endTime) {
                    it->endTime = currentTime;
                    break;
                }
            }
        }
        break;

    default:
        // Other states are recorded but do not affect active intervals.
        break;
    }
}

uint64_t CConnTracker::getTime(const std::string& ipAddress, uint64_t timeWindow) {
    std::lock_guard<std::mutex> lock(mGuardian); // Protect shared resources

    // Generate a hash key for the IP address.
    auto hashKey = CCryptoFactory::getInstance()->getSHA2_256Vec(
        std::vector<uint8_t>(ipAddress.begin(), ipAddress.end())
    );

    auto it = mActiveConnections.find(hashKey);
    if (it == mActiveConnections.end()) {
        return 0; // No connection history.
    }

    const auto& intervals = it->second;
    std::time_t currentTime = std::time(nullptr);
    std::time_t windowStart = currentTime - timeWindow;

    // Collect and adjust intervals within the time window.
    std::vector<std::pair<std::time_t, std::time_t>> adjustedIntervals;

    for (const auto& interval : intervals) {
        std::time_t startTime = std::max(interval.startTime, windowStart);
        std::time_t endTime = interval.endTime.value_or(currentTime);
        endTime = std::min(endTime, currentTime);

        if (endTime <= startTime) {
            continue; // No overlap with the time window.
        }

        adjustedIntervals.emplace_back(startTime, endTime);
    }

    if (adjustedIntervals.empty()) {
        return 0;
    }

    // Merge overlapping intervals.
    std::sort(adjustedIntervals.begin(), adjustedIntervals.end());
    std::vector<std::pair<std::time_t, std::time_t>> mergedIntervals;

    auto currentInterval = adjustedIntervals.front();
    for (size_t i = 1; i < adjustedIntervals.size(); ++i) {
        const auto& interval = adjustedIntervals[i];
        if (interval.first <= currentInterval.second) {
            // Overlapping intervals; merge them.
            currentInterval.second = std::max(currentInterval.second, interval.second);
        }
        else {
            // Non-overlapping interval; add the current interval to the list.
            mergedIntervals.push_back(currentInterval);
            currentInterval = interval;
        }
    }
    mergedIntervals.push_back(currentInterval);

    // Calculate total connected time.
    uint64_t totalConnectedTime = 0;
    for (const auto& interval : mergedIntervals) {
        totalConnectedTime += static_cast<uint64_t>(interval.second - interval.first);
    }

    return totalConnectedTime;
}

uint64_t CConnTracker::getRenewalTime(const std::string& ipAddress, uint64_t timeWindow, uint64_t maxDuration) {
    std::lock_guard<std::mutex> lock(mGuardian); // Protect shared resources

    // Generate a hash key for the IP address.
    auto hashKey = CCryptoFactory::getInstance()->getSHA2_256Vec(
        std::vector<uint8_t>(ipAddress.begin(), ipAddress.end())
    );

    std::time_t currentTime = std::time(nullptr);
    std::time_t windowStart = currentTime - timeWindow;

    // Find connection intervals for the IP address.
    auto it = mActiveConnections.find(hashKey);
    if (it == mActiveConnections.end()) {
        return currentTime; // No connection history; connection allowed now.
    }

    const auto& intervals = it->second;

    // Collect and adjust intervals within the time window.
    std::vector<std::pair<std::time_t, std::time_t>> adjustedIntervals;
    for (const auto& interval : intervals) {
        std::time_t startTime = std::max(interval.startTime, windowStart);
        std::time_t endTime = interval.endTime.value_or(currentTime);
        endTime = std::min(endTime, currentTime);

        if (endTime <= startTime) {
            continue; // No overlap with the time window.
        }

        adjustedIntervals.emplace_back(startTime, endTime);
    }

    if (adjustedIntervals.empty()) {
        return currentTime; // Connection is allowed now.
    }

    // Merge overlapping intervals.
    std::sort(adjustedIntervals.begin(), adjustedIntervals.end());
    std::vector<std::pair<std::time_t, std::time_t>> mergedIntervals;

    auto currentInterval = adjustedIntervals.front();
    for (size_t i = 1; i < adjustedIntervals.size(); ++i) {
        const auto& interval = adjustedIntervals[i];
        if (interval.first <= currentInterval.second) {
            // Overlapping intervals; merge them.
            currentInterval.second = std::max(currentInterval.second, interval.second);
        }
        else {
            // Non-overlapping interval; add the current interval to the list.
            mergedIntervals.push_back(currentInterval);
            currentInterval = interval;
        }
    }
    mergedIntervals.push_back(currentInterval);

    // Calculate total connected time and create events.
    uint64_t totalConnectedTime = 0;
    std::vector<std::pair<std::time_t, int>> events; // time, +1 for start, -1 for end

    for (const auto& interval : mergedIntervals) {
        totalConnectedTime += static_cast<uint64_t>(interval.second - interval.first);
        // Event when interval starts.
        events.emplace_back(interval.first, +1);
        // Event when interval ends.
        events.emplace_back(interval.second, -1);
    }

    if (totalConnectedTime < maxDuration) {
        return currentTime; // Connection is allowed now.
    }

    // Sort events by time.
    std::sort(events.begin(), events.end());

    // Simulate the progression of time.
    uint64_t cumulativeTime = totalConnectedTime;
    int activeIntervals = 0;
    std::time_t lastEventTime = currentTime;

    for (const auto& event : events) {
        std::time_t eventTime = event.first;
        int delta = event.second;

        // Skip events in the past.
        if (eventTime <= currentTime) {
            activeIntervals += delta;
            lastEventTime = eventTime;
            continue;
        }

        // Time elapsed since last event.
        std::time_t elapsedTime = eventTime - lastEventTime;

        // Adjust cumulative connected time.
        cumulativeTime -= static_cast<uint64_t>(activeIntervals * elapsedTime);

        if (cumulativeTime <= maxDuration) {
            return eventTime;
        }

        activeIntervals += delta;
        lastEventTime = eventTime;
    }

    // If cumulative time never drops below maxDuration, return the last event time.
    return lastEventTime;
}

void CConnTracker::cleanupOldConnections(std::time_t olderThan) {
    std::lock_guard<std::mutex> lock(mGuardian); // Protect shared resources

    // Clean up connection events.
    for (auto it = mConnectionEvents.begin(); it != mConnectionEvents.end(); ) {
        auto& events = it->second;
        events.erase(std::remove_if(events.begin(), events.end(),
            [olderThan](const ConnectionEvent& event) {
                return event.timestamp < olderThan;
            }), events.end());

        if (events.empty()) {
            it = mConnectionEvents.erase(it);
        }
        else {
            ++it;
        }
    }

    // Clean up active connections.
    for (auto it = mActiveConnections.begin(); it != mActiveConnections.end(); ) {
        auto& intervals = it->second;
        intervals.erase(std::remove_if(intervals.begin(), intervals.end(),
            [olderThan](const ConnectionInterval& interval) {
                std::time_t endTime = interval.endTime.value_or(std::time(nullptr));
                return endTime < olderThan;
            }), intervals.end());

        if (intervals.empty()) {
            it = mActiveConnections.erase(it);
        }
        else {
            ++it;
        }
    }
}