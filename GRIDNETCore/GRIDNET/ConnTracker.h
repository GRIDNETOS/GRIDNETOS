#ifndef CCONNTRACKER_H
#define CCONNTRACKER_H

#include <string>
#include "robin_hood.h"
#include <vector>
#include <ctime>
#include <mutex>
#include <optional>

#include "CryptoFactory.h" 
struct ConnectionState {
    std::optional<std::time_t> startTime;
    std::optional<std::time_t> endTime;
};

class CConnTracker {
public:
    /**
     * @brief Constructs a new CConnTracker object.
     */
    CConnTracker();

    /**
     * @brief Records the state change of a connection for a given IP address.
     *
     * @param ipAddress The IP address associated with the connection.
     * @param state The new state of the connection.
     */
    void pingState(const std::string& ipAddress, eConversationState::eConversationState state);

    /**
     * @brief Calculates the total connected time within a specified time window for a given IP address.
     *
     * @param ipAddress The IP address to query.
     * @param timeWindow The time window in seconds to consider.
     * @return The total connected time in seconds within the specified time window.
     */
    uint64_t getTime(const std::string& ipAddress, uint64_t timeWindow);

    /**
     * @brief Calculates when a new connection will be allowed for a given IP address.
     *
     * @param ipAddress The IP address to check.
     * @param timeWindow The total time window to consider, in seconds.
     * @param maxDuration The maximum allowed connection duration within the time window, in seconds.
     * @return The Unix timestamp when a new connection will be allowed.
     */
    uint64_t getRenewalTime(const std::string& ipAddress, uint64_t timeWindow, uint64_t maxDuration);

    /**
     * @brief Cleans up old connection records that ended before a specified time.
     *
     * @param olderThan Connections that ended before this Unix timestamp will be removed.
     */
    void cleanupOldConnections(std::time_t olderThan);

private:
    /**
     * @brief Represents a single connection state change event.
     */
    struct ConnectionEvent {
        eConversationState::eConversationState state; ///< The state of the connection.
        std::time_t timestamp; ///< The timestamp of the state change.
    };

    /**
     * @brief Represents a single connection interval.
     */
    struct ConnectionInterval {
        std::time_t startTime;                 ///< The start time of the connection.
        std::optional<std::time_t> endTime;    ///< The end time of the connection (optional).
    };

    // Maps IP addresses to their connection events (for accountability)
    robin_hood::unordered_map<std::vector<uint8_t>, std::vector<ConnectionEvent>> mConnectionEvents;

    // Maps IP addresses to their active connection intervals
    robin_hood::unordered_map<std::vector<uint8_t>, std::vector<ConnectionInterval>> mActiveConnections;

    std::mutex mGuardian; ///< Mutex to protect shared resources.
};

#endif // CCONNTRACKER_H
