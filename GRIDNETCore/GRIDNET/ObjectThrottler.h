#include <deque>
#include <cmath>
#include <ctime>
#include <algorithm>
// Add current observation
                // When adding a new observation, also store the current timestamp

                /* **Dynamic Adjustment of Minimum Required Objects in a Block Using Exponential Moving Average (EMA) and Volatility**

This mechanism dynamically determines the minimum number of objects (transactions or other entities) that should be included in a block,
leveraging the concepts of Exponential Moving Average (EMA) and volatility.

1. **Exponential Moving Average (EMA) Overview**:
   - EMA provides a way to calculate an average value that gives more weight to recent observations.
   This makes it more responsive to recent changes and is especially useful in a context where values can be volatile.
   - The formula for EMA is:
     \[ \text{EMA}_{\text{new}} = (\text{Value}_{\text{new}} \times \text{ALPHA}) + (\text{EMA}_{\text{previous}} \times (1 - \text{ALPHA})) \]
   where `ALPHA` is a coefficient between 0 and 1 that determines the weight given to the most recent observation. A higher ALPHA makes the EMA more responsive to recent changes.

2. **Observation Management**:
   - A list, `mMemPoolEMAObservations`, is maintained to keep track of recent EMAs, with its size capped at `N` entries.
   - When a new observation is made, if the list already contains `N` entries, the oldest EMA is removed to make space for the new one.

3. **Calculating the EMA**:
   - The EMA is recalculated using the formula mentioned above. The previous EMA (if any) is retrieved from the end of the `mMemPoolEMAObservations` list,
   and the new observation is the current number of available objects.
   - If there are no prior EMA observations (i.e., the list is empty), the current number of available objects is used in place of the previous EMA.

4. **Calculating Volatility**:
   - Volatility measures the relative change in consecutive EMAs, indicating the fluctuation in the number of available objects.
   - Volatility is calculated as:
     \[ \text{volatility} = \frac{|\text{EMA}_{\text{new}} - \text{EMA}_{\text{previous}}|}{\text{EMA}_{\text{previous}}} \]

5. **Adaptive Sensitivity**:
   - Sensitivity determines how reactive the minimum required objects are to changes in the observed values. A higher sensitivity means the minimum requirement will adjust more aggressively to changes.
   - If the calculated volatility exceeds a predefined threshold (`VOLATILITY_THRESHOLD`), indicating a high fluctuation, the sensitivity (`SENSITIVITY`) is adjusted (usually dampened) to make the mechanism less reactive to large, abrupt changes.

6. **Adjusting the Minimum Required Objects**:
   - Based on the EMA and the adaptive sensitivity, the `minNumberOfObjectsInBlock` is adjusted. The rate of change between the new EMA and the previous EMA (or the new value if there's no previous EMA) is scaled down by the adaptive sensitivity,
   and the result is added to the current `minNumberOfObjectsInBlock`.

7. **Ensuring Bounds**:
   - To ensure that the adjusted `minNumberOfObjectsInBlock` does not exceed reasonable limits or go below a practical minimum, it's clamped between predefined bounds: `MIN_MIN_OBJECTS` and `MAX_MIN_OBJECTS`.

8. **Final Step - Update Observations**:
   - The newly calculated EMA is appended to the `mMemPoolEMAObservations` list for future reference.


By leveraging EMA and volatility, this mechanism provides a dynamic approach to adjusting the minimum required objects in a block.
It ensures that the system remains responsive to recent trends while avoiding over-reactivity to sudden spikes or drops.
It's particularly useful in decentralized systems where transaction rates can vary significantly over short periods.
                */


/*

## `CObjectThrottler` Documentation

### Overview

`CObjectThrottler` is a utility class designed to compute and adjust the minimum number of objects required in a block over time. It utilizes an Exponential Moving Average (EMA) mechanism, in conjunction with volatility measures and adaptive sensitivity, to react dynamically to changes in the number of available objects.

### Features

1. **Exponential Moving Average (EMA)**: Provides a balance between current observations and historical data to smooth out erratic behavior.
2. **Volatility Detection**: Measures the variability in recent observations to determine the stability of the object count.
3. **Adaptive Sensitivity**: Adjusts the sensitivity of changes based on detected volatility to provide a balance between reactivity and stability.

### Main Components

- **EMA Observations (`mMemPoolEMAObservations`)**: A list storing the recent computed EMA values.

- **Timestamps (`mMemPoolEMATimestamps`)**: A list storing the timestamps of recent observations, corresponding to EMA observations.

### Constructor

The `CObjectThrottler` can be initialized with various parameters:

1. **Alpha**: The weight given to the latest observation in EMA calculation.
2. **N**: The window size for the number of recent observations stored.
3. **Sensitivity**: A factor that determines the responsiveness of the throttler to changes.
4. **Volatility Threshold**: The limit above which the system considers the object count to be volatile.
5. **Min-Min Objects and Max-Min Objects**: Define the bounds for the minimum number of objects.

### Main Function: `calc()`

#### Purpose

To update the internal state of the throttler based on new observations and/or to compute the required number of objects based on current state and time.

#### Arguments

- `isMeasurement`: Boolean flag. If `true`, the function will take `nrOfAvailableObjects` as the new measurement. If `false`, the function will only compute the required number of objects based on the current timestamp and stored data.

- `nrOfAvailableObjects`: The latest count of available objects. Used when `isMeasurement` is `true`.

#### Operation

1. **Timestamp Handling**: The current time is fetched, and the time difference since the last observation is calculated.
2. **EMA Calculation**: Using the provided alpha and the last observation (or the provided number if `isMeasurement` is true), the EMA is computed. This value is adjusted for time decay based on the time since the last observation.
3. **Volatility Calculation**: The volatility (rate of change) is calculated based on the difference between the current EMA and the previous EMA. If this volatility exceeds the set threshold, the sensitivity is doubled to dampen the rate of change.
4. **Adjusting `minNumberOfObjectsInBlock`**: The rate of change is divided by the (possibly adjusted) sensitivity, and this value is added to `minNumberOfObjectsInBlock`. This value is then clamped within the set bounds.
5. **Updating Observations and Timestamps**: If `isMeasurement` is `true`, the new EMA and current timestamp are added to their respective lists. If the list size exceeds the window size `N`, the oldest entry is removed.

### Usage Scenario

This throttler can be employed in situations where:

1. You need to dynamically allocate resources based on changing conditions.
2. You want the system to be responsive to rapid changes but also remain stable during periods of minor fluctuations.

### Conclusion

`CObjectThrottler` is a powerful tool for managing dynamic systems with changing resource availability. By balancing responsiveness and stability, it ensures that resources are allocated efficiently and reactively.
*/
class CObjectThrottler {
private:
    double ALPHA;                 // Smoothing factor for EMA
    std::size_t N;                // Number of observations to store
    uint64_t SENSITIVITY;         // Base sensitivity value
    double VOLATILITY_THRESHOLD;  // Threshold for high volatility
    int64_t MIN_MIN_OBJECTS;      // Minimum bound for minNumberOfObjectsInBlock
    int64_t MAX_MIN_OBJECTS;      // Maximum bound for minNumberOfObjectsInBlock

    std::deque<double> mMemPoolEMAObservations;
    std::deque<uint64_t> mMemPoolEMATimestamps;

public:
    CObjectThrottler(
        double alpha = 0.2,
        std::size_t n = 10,
        uint64_t sensitivity = 1,
        double volatilityThreshold = 0.05,
        int64_t minMinObjects = 1,
        int64_t maxMinObjects = 10)
        : ALPHA(alpha),
        N(n),
        SENSITIVITY(sensitivity),
        VOLATILITY_THRESHOLD(volatilityThreshold),
        MIN_MIN_OBJECTS(minMinObjects),
        MAX_MIN_OBJECTS(maxMinObjects) {}

    int64_t calc(int64_t nrOfAvailableObjects = 0, bool isMeasurement = true) {
        uint64_t currentTime = std::time(0);
        int64_t lastObservation = mMemPoolEMAObservations.size() > 0 ? mMemPoolEMAObservations.back() : 0;

        if (isMeasurement) {
            // Add current observation
            if (mMemPoolEMAObservations.size() == N) {
                mMemPoolEMAObservations.pop_front();
                mMemPoolEMATimestamps.pop_front(); // Remove the oldest timestamp
            }

            mMemPoolEMAObservations.push_back(nrOfAvailableObjects);
            mMemPoolEMATimestamps.push_back(currentTime);

            lastObservation = nrOfAvailableObjects;
        }

        // Calculate actual time difference
        uint64_t lastObservationTime = mMemPoolEMATimestamps.size() > 0 ? mMemPoolEMATimestamps.back() : currentTime;
        double timeDifference = currentTime - lastObservationTime;
        double decayFactor = exp(-ALPHA * timeDifference);

        // Calculate EMA
        double ema = (lastObservation * ALPHA + (1.0 - ALPHA) * lastObservation) * decayFactor;

        // Calculate Volatility
        double volatility = 0.0;
        if (mMemPoolEMAObservations.size() > 0) {
            volatility = abs(ema - mMemPoolEMAObservations.back()) / mMemPoolEMAObservations.back();
        }

        // Calculate Adaptive Sensitivity
        uint64_t adaptiveSensitivity = SENSITIVITY;
        if (volatility > VOLATILITY_THRESHOLD) {
            adaptiveSensitivity *= 2;  // Dampen the rate of change if there's high volatility
        }

        // Safely calculate rateOfChange
        int64_t previousVal = lastObservation;
        int64_t rateOfChange = static_cast<int64_t>(ema) - previousVal;

        // Adjust minNumberOfObjectsInBlock using EMA and Adaptive Sensitivity

        int64_t minNumberOfObjectsInBlock = (double)previousVal + ((double)rateOfChange / (double)adaptiveSensitivity);

        // Ensure bounds
        minNumberOfObjectsInBlock = max(MIN_MIN_OBJECTS, minNumberOfObjectsInBlock);
        minNumberOfObjectsInBlock = min(MAX_MIN_OBJECTS, minNumberOfObjectsInBlock);

        return minNumberOfObjectsInBlock;
    }
};
