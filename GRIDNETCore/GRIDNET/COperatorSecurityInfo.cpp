#include "OperatorSecurityInfo.hpp"
#include "Tools.h"



// Methods to update security metrics
void COperatorSecurityInfo::incrementTimestampManipulationCount(uint64_t increment) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mTimestampManipulationCount += increment;
}

void COperatorSecurityInfo::incrementPowWaveAttackCount(uint64_t increment) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mPowWaveAttackCount += increment;
}

void COperatorSecurityInfo::addDetailedReport(const std::string& report) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mDetailedReports.push_back(report);
}

void COperatorSecurityInfo::addDetailedReports(const std::vector<std::string>& reports) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mDetailedReports.insert(mDetailedReports.end(), reports.begin(), reports.end());
}

// Methods to update activity tracking data
void COperatorSecurityInfo::addSolvedTime(uint64_t solvedTime) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mSolvedTimes.push_back(solvedTime);
}

void COperatorSecurityInfo::incrementBlocksMined() {
    std::lock_guard<std::mutex> lock(mGuardian);
    mBlocksMined++;
}

void COperatorSecurityInfo::incrementBlocksMinedDuringDifficultyDecrease() {
    std::lock_guard<std::mutex> lock(mGuardian);
    mBlocksMinedDuringDifficultyDecrease++;
}

void COperatorSecurityInfo::incrementBlocksMinedDuringDifficultyIncrease() {
    std::lock_guard<std::mutex> lock(mGuardian);
    mBlocksMinedDuringDifficultyIncrease++;
}

/**
 * @brief Computes the operator's malicious confidence level based on multiple factors
 *
 * Calculates a confidence score (0.0-1.0) indicating the likelihood of malicious behavior.
 * Takes into account:
 * - Mining pattern anomalies (CUSUM-based)
 * - Global anomaly participation
 * - Timestamp manipulation attempts (minor/major/critical)
 * - PoW wave attack participation
 * - Group coordination in attacks
 *
 * @return double Confidence level between 0.0 and 1.0
 */
double COperatorSecurityInfo::computeConfidenceLevel() {
    std::lock_guard<std::mutex> lock(mGuardian);

    // Early exit if no blocks mined
    if (mBlocksMined == 0) return 0.0;

    // Constants - BEGIN
    // Base calculation constants
    static constexpr double EPSILON = 1e-8;
    static constexpr double MAX_ANOMALY_SCORE = 1e6;
    static constexpr double CONFIDENCE_GRACE_THRESHOLD = 0.1;

    // Decay and contribution factors
    static constexpr double ANOMALY_DECAY = 0.9;          // 10% decay per update
    static constexpr double GLOBAL_ANOMALY_FACTOR = 0.2;  // 20% weight for global anomalies

    // Timestamp manipulation constants
    static constexpr double TIMESTAMP_MINOR_PENALTY = 0.05;     // 5% per minor violation
    static constexpr double TIMESTAMP_MAJOR_PENALTY = 0.10;     // 10% per major violation
    static constexpr double TIMESTAMP_CRITICAL_PENALTY = 0.20;  // 20% per critical violation
    static constexpr uint64_t TIMESTAMP_GRACE_PERIOD = 3;      // Grace threshold

    // PoW and group participation constants
    static constexpr double POW_WAVE_PENALTY_FACTOR = 0.15;    // 15% per PoW wave attack
    static constexpr uint64_t POW_WAVE_GRACE = 3;             // Grace for PoW attacks
    static constexpr double GROUP_PARTICIPATION_FACTOR = 0.2;  // 20% per group participation
    // Constants - END

    // Base Anomaly Score Calculation - BEGIN
    // Reset extremely small values to prevent underflow
    if (mAnomalyScore < EPSILON) {
        mAnomalyScore = 0.0;
    }

    // Apply time-based decay to historical anomaly score
    mAnomalyScore = std::max(0.0, mAnomalyScore * ANOMALY_DECAY);

    // Calculate CUSUM-based contribution with numerical stability
    double cusumContribution = 0.0;
    if (mEWMAMiningRate > EPSILON) {
        // Normalize CUSUM values by mining rate and block count
        cusumContribution = (std::abs(mCusumPositive) + std::abs(mCusumNegative)) /
            (std::max(mEWMAMiningRate, EPSILON) * std::max(1.0, static_cast<double>(mBlocksMined)));
    }

    // Add global anomaly contribution
    double globalAnomalyContribution = mGlobalAnomalies * GLOBAL_ANOMALY_FACTOR;

    // Update total anomaly score with bounds
    mAnomalyScore = std::min(MAX_ANOMALY_SCORE,
        mAnomalyScore + cusumContribution + globalAnomalyContribution);
    // Base Anomaly Score Calculation - END

    // Confidence Level Calculation - BEGIN
    // Calculate base confidence using logistic-like function
    double denominatorTerm = std::max(1.0, 1.0 + mAnomalyScore);
    double baseConfidence = mAnomalyScore / denominatorTerm;

    // Apply penalties only if base confidence exceeds grace threshold
    if (baseConfidence > CONFIDENCE_GRACE_THRESHOLD) {
        // Timestamp Manipulation Penalties - BEGIN
        if (mTimestampManipulationCount > TIMESTAMP_GRACE_PERIOD) {
            uint64_t excessManipulations = mTimestampManipulationCount - TIMESTAMP_GRACE_PERIOD;

            // Calculate penalty multiplier based on excess manipulations
            // Uses direct manipulation count since it's already weighted in analyzeBlockForTimestampManipulation
            double timestampPenaltyMultiplier = 1.0 + (excessManipulations * TIMESTAMP_MINOR_PENALTY);

            // Apply penalty with bounds checking
            baseConfidence = std::min(1.0, baseConfidence * timestampPenaltyMultiplier);
        }
        // Timestamp Manipulation Penalties - END

        // PoW Attack Penalties - BEGIN
        if (mPowWaveAttackCount > POW_WAVE_GRACE) {
            uint64_t excessPowAttacks = mPowWaveAttackCount - POW_WAVE_GRACE;
            double powPenalty = excessPowAttacks * POW_WAVE_PENALTY_FACTOR;

            // Apply PoW penalty with bounds
            baseConfidence = std::min(1.0, baseConfidence * (1.0 + powPenalty));
        }
        // PoW Attack Penalties - END

        // Group Participation Penalties - BEGIN
        if (mGroupParticipationCount > 0) {
            double groupPenalty = mGroupParticipationCount * GROUP_PARTICIPATION_FACTOR;

            // Apply group penalty with bounds
            baseConfidence = std::min(1.0, baseConfidence * (1.0 + groupPenalty));
        }
        // Group Participation Penalties - END
    }
    // Confidence Level Calculation - END

    // Update and return final confidence level
    mConfidenceLevel = baseConfidence;
    return mConfidenceLevel;
}
void COperatorSecurityInfo::incrementGroupParticipationCount() {
    std::lock_guard<std::mutex> lock(mGuardian);
    mGroupParticipationCount++;
}
std::string COperatorSecurityInfo::getConfidenceLevelExplanation() const {
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::lock_guard<std::mutex> lock(mGuardian);
    std::stringstream explanation;
    explanation << std::fixed << std::setprecision(2);

    // Header Section with prominent formatting
    explanation << tools->getColoredString(u8"╔════════════════ OPERATOR SECURITY REPORT ════════════════╗\n", eColor::lightCyan);
    explanation << tools->getColoredString(u8"║ Operator ID: ", eColor::blue) << mOperatorID << "\n";

    // Confidence Level with color based on severity
    std::string confidenceStr = u8"║ Confidence Level: " + std::to_string(mConfidenceLevel * 100.0) + "%";
    if (mConfidenceLevel >= 0.80) {
        explanation << tools->getColoredString(confidenceStr, eColor::cyborgBlood);
    }
    else if (mConfidenceLevel >= 0.60) {
        explanation << tools->getColoredString(confidenceStr, eColor::cyberWine);
    }
    else if (mConfidenceLevel >= 0.40) {
        explanation << tools->getColoredString(confidenceStr, eColor::orange);
    }
    else if (mConfidenceLevel >= 0.20) {
        explanation << tools->getColoredString(confidenceStr, eColor::lightCyan);
    }
    else {
        explanation << tools->getColoredString(confidenceStr, eColor::lightGreen);
    }
    explanation << u8"\n╚════════════════════════════════════════════════════════════╝\n\n";

    if (mBlocksMined == 0) {
        explanation << tools->getColoredString("No blocks mined yet. Confidence level is 0% due to insufficient data.", eColor::lightWhite);
        return explanation.str();
    }

    // Activity Statistics Section
    explanation << tools->getColoredString("[ Activity Statistics ]\n", eColor::lightCyan);
    explanation << tools->getColoredString(u8"• Total blocks mined: ", eColor::lightWhite) << mBlocksMined << "\n";
    explanation << tools->getColoredString(u8"• Mining rate: ", eColor::lightWhite) << mEWMAMiningRate << " blocks/sec\n";
    explanation << tools->getColoredString(u8"• Difficulty decrease blocks: ", eColor::lightWhite)
        << mBlocksMinedDuringDifficultyDecrease << " ("
        << (100.0 * mBlocksMinedDuringDifficultyDecrease / mBlocksMined) << "%)\n\n";

    // Risk Assessment Section
    std::string riskHeader;
    eColor::eColor riskColor;
    if (mConfidenceLevel >= 0.80) {
        riskHeader = "[ SEVERE RISK ASSESSMENT (80-100%) ]";
        riskColor = eColor::cyborgBlood;
    }
    else if (mConfidenceLevel >= 0.60) {
        riskHeader = "[ HIGH RISK ASSESSMENT (60-80%) ]";
        riskColor = eColor::cyberWine;
    }
    else if (mConfidenceLevel >= 0.40) {
        riskHeader = "[ ELEVATED RISK ASSESSMENT (40-60%) ]";
        riskColor = eColor::orange;
    }
    else if (mConfidenceLevel >= 0.20) {
        riskHeader = "[ MODERATE RISK ASSESSMENT (20-40%) ]";
        riskColor = eColor::lightCyan;
    }
    else {
        riskHeader = "[ LOW RISK ASSESSMENT (0-20%) ]";
        riskColor = eColor::lightGreen;
    }
    explanation << tools->getColoredString(riskHeader, riskColor) << "\n";

    // Risk Details
    if (mConfidenceLevel < 0.20 && mConfidenceLevel < EPSILON) {
        explanation << tools->getColoredString("Operator shows no suspicious behavior. Mining patterns are within normal parameters.", eColor::lightGreen) << "\n";
    }
    else {
        explanation << tools->getColoredString("Notable Patterns:", eColor::lightWhite) << "\n";
        if (mPositiveAnomalies > 0) {
            explanation << tools->getColoredString("• " + std::to_string(mPositiveAnomalies) + " instances of increased mining rate\n", eColor::orange);
        }
        if (mNegativeAnomalies > 0) {
            explanation << tools->getColoredString("• " + std::to_string(mNegativeAnomalies) + " instances of decreased mining rate\n", eColor::orange);
        }
    }

    // Statistical Analysis Section
    explanation << "\n" << tools->getColoredString("[ Statistical Analysis ]\n", eColor::lightCyan);
    if (std::abs(mCusumPositive) > EPSILON || std::abs(mCusumNegative) > EPSILON) {
        explanation << tools->getColoredString("CUSUM Analysis:\n", eColor::lightWhite);
        explanation << tools->getColoredString(u8"• Positive deviation: ", eColor::lightWhite) << mCusumPositive << "\n";
        explanation << tools->getColoredString(u8"• Negative deviation: ", eColor::lightWhite) << mCusumNegative << "\n";
    }

    // Volatility
    if (mVolatility > EPSILON) {
        explanation << tools->getColoredString(u8"• Mining rate volatility: ", eColor::lightWhite)
            << tools->getColoredString(std::to_string(mVolatility * 100.0) + "%\n",
                mVolatility > 0.5 ? eColor::cyberWine : eColor::lightCyan);
    }

    // Timestamp Manipulation Section
    if (mTimestampManipulationCount > 0) {
        explanation << "\n" << tools->getColoredString("[ Timestamp Manipulation Analysis ]\n", eColor::lightCyan);
        explanation << tools->getColoredString(u8"• Total manipulations: ", eColor::lightWhite) << mTimestampManipulationCount << "\n";

        if (mTimestampManipulationCount > TIMESTAMP_MANIPULATION_GRACE) {
            explanation << tools->getColoredString(u8"• Exceeds grace threshold by: ", eColor::cyborgBlood)
                << (mTimestampManipulationCount - TIMESTAMP_MANIPULATION_GRACE) << "\n";
            explanation << tools->getColoredString(u8"• Contributing to elevated risk level\n", eColor::cyborgBlood);
        }
        else {
            explanation << tools->getColoredString(u8"• Within grace threshold (no penalty applied)\n", eColor::lightGreen);
        }
    }

    // Recommendations Section
    explanation << "\n" << tools->getColoredString("[ Recommendations ]\n", eColor::lightCyan);
    if (mConfidenceLevel > 0.90) {
        explanation << tools->getColoredString(u8"❌ Trading highly discouraged. Assets may be ceased at any moment.\n", eColor::cyborgBlood);
        explanation << tools->getColoredString(u8"❌ Permanent mining restrictions imminent.\n", eColor::cyborgBlood);
        explanation << tools->getColoredString(u8"⚠ High likelihood of coordinated malicious activities.\n", eColor::cyborgBlood);
    }
    else if (mConfidenceLevel > 0.60) {
        explanation << tools->getColoredString(u8"⚠ Caution: Trading discouraged\n", eColor::cyberWine);
        explanation << tools->getColoredString(u8"⚠ Temporary mining restrictions likely\n", eColor::cyberWine);
        explanation << tools->getColoredString(u8"⚠ Monitor for coordinated activities\n", eColor::cyberWine);
    }
    else if (mConfidenceLevel > 0.40) {
        explanation << tools->getColoredString(u8"ℹ Enhanced monitoring recommended\n", eColor::orange);
        explanation << tools->getColoredString(u8"ℹ Review mining patterns regularly\n", eColor::orange);
    }
    else if (mConfidenceLevel > 0.20) {
        explanation << tools->getColoredString(u8"✓ Trading allowed with monitoring\n", eColor::lightCyan);
        explanation << tools->getColoredString(u8"ℹ Review unusual patterns if persistent\n", eColor::lightCyan);
    }
    else {
        explanation << tools->getColoredString(u8"✓ Safe to Trade - Noble Operator Status\n", eColor::lightGreen);
    }

    explanation << tools->getColoredString(u8"\n╚════════════════ END OF REPORT ════════════════╝\n", eColor::lightCyan);

    return explanation.str();
}
void COperatorSecurityInfo::incrementGlobalAnomalyCount() {
    std::lock_guard<std::mutex> lock(mGuardian);
    mGlobalAnomalies++;
}

size_t COperatorSecurityInfo::getGlobalAnomalyCount() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mGlobalAnomalies;
}



void COperatorSecurityInfo::listAnomalies(std::stringstream& ss) const {
    if (mPositiveAnomalies > 0 || mNegativeAnomalies > 0) {
        if (mPositiveAnomalies > 0) {
            ss << "- " << mPositiveAnomalies << " instances of abnormal mining rate increases\n";
        }
        if (mNegativeAnomalies > 0) {
            ss << "- " << mNegativeAnomalies << " instances of abnormal mining rate decreases\n";
        }

        double anomalyRate = static_cast<double>(mPositiveAnomalies + mNegativeAnomalies)
            / mBlocksMined;
        ss << "- Anomaly rate: " << (anomalyRate * 100.0) << "% of mined blocks\n";
    }
}

void COperatorSecurityInfo::listTimestampManipulations(std::stringstream& ss) const {
    if (mTimestampManipulationCount > 0) {
        ss << "\nTimestamp Manipulation Details:\n"
            << "- Total manipulations: " << mTimestampManipulationCount << "\n"
            << "- Manipulation rate: "
            << (static_cast<double>(mTimestampManipulationCount) / mBlocksMined * 100.0)
            << "% of mined blocks\n";

        if (mTimestampManipulationCount > TIMESTAMP_MANIPULATION_GRACE) {
            ss << "- Exceeds grace threshold, contributing to risk assessment\n";
        }
    }
}

double COperatorSecurityInfo::getConfidenceLevel() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mConfidenceLevel;
}
void COperatorSecurityInfo::updateMiningStatistics(uint64_t blockHeight, uint64_t blockTime) {
    std::lock_guard<std::mutex> lock(mGuardian);

    if (mLastBlockTime == 0 || blockTime <= mLastBlockTime) {
        mLastBlockTime = blockTime;
        mLastBlockHeight = blockHeight;
        return;
    }

    // Constants
    const double MIN_INTER_BLOCK_TIME = 1.0;     // Minimum acceptable inter-block time
    const double MAX_MINING_RATE = 1.0;          // Maximum acceptable mining rate (blocks per second)
    const double EPSILON = 1e-8;                 // Small epsilon to avoid division by zero
    const size_t RECENT_DEVIATIONS_SIZE = 100;   // Number of recent deviations to keep
    const double CUSUM_K_FACTOR = 0.5;           // Sensitivity factor for CUSUM
    const double CUSUM_H_FACTOR = 5.0;           // Threshold factor for CUSUM
    const double MAX_ANOMALY_SCORE = 1e6;        // Maximum value for anomaly scores

    // Calculate inter-block time with minimum threshold
    double interBlockTime = std::max(MIN_INTER_BLOCK_TIME, static_cast<double>(blockTime - mLastBlockTime));

    // Calculate mining rate with maximum threshold
    double miningRate = std::min(MAX_MINING_RATE, 1.0 / interBlockTime);

    // Update EWMA
    if (mEWMAMiningRate < EPSILON) {
        mEWMAMiningRate = miningRate;
    }
    else {
        // Stable EWMA formula with smoothing factor alpha = 0.1
        mEWMAMiningRate += 0.1 * (miningRate - mEWMAMiningRate);
    }

    // Calculate deviation and store recent deviations
    double deviation = miningRate - mEWMAMiningRate;
    mRecentDeviations.push_back(deviation);
    if (mRecentDeviations.size() > RECENT_DEVIATIONS_SIZE) {
        mRecentDeviations.pop_front();
    }

    // Calculate volatility based on recent deviations
    double volatility = calculateVolatility();
    mVolatility = volatility;

    // Dynamic reference value k for CUSUM
    double k = CUSUM_K_FACTOR * mEWMAMiningRate * (1.0 + volatility);

    // Update CUSUM statistics with bounds
    mCusumPositive = std::min(MAX_ANOMALY_SCORE, std::max(0.0, mCusumPositive + deviation - k));
    mCusumNegative = std::max(-MAX_ANOMALY_SCORE, std::min(0.0, mCusumNegative + deviation + k));

    // Dynamic threshold h based on volatility
    double threshold = CUSUM_H_FACTOR * mEWMAMiningRate * (1.0 + volatility);

    // Detect positive anomaly with smoother reset
    if (mCusumPositive > threshold) {
        mPositiveAnomalies++;
        mCusumPositive *= 0.7; // Partial reset for stability
    }

    // Detect negative anomaly with smoother reset
    if (mCusumNegative < -threshold) {
        mNegativeAnomalies++;
        mCusumNegative *= 0.7; // Partial reset for stability
    }

    // Update last block time and height
    mLastBlockTime = blockTime;
    mLastBlockHeight = blockHeight;
}

double COperatorSecurityInfo::calculateVolatility() {

    if (mRecentDeviations.empty()) return 0.0;

    // Numerically stable calculation of standard deviation
    double mean = 0.0, M2 = 0.0;
    size_t count = 0;

    for (double dev : mRecentDeviations) {
        count++;
        double delta = dev - mean;
        mean += delta / count;
        double delta2 = dev - mean;
        M2 += delta * delta2;
    }

    double variance = M2 / count;
    return std::sqrt(std::max(0.0, variance));
}

void COperatorSecurityInfo::incrementBlocksMinedDuringAnomalousIntervals(uint64_t blocks) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mBlocksMinedDuringAnomalousIntervals += blocks;
}

void COperatorSecurityInfo::setLastReportedAtHeight(uint64_t height) {
    std::lock_guard<std::mutex> lock(mGuardian);
    mLastReportedAtHeight = height;
}

// Getters
std::string COperatorSecurityInfo::getOperatorID() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mOperatorID;
}

uint64_t COperatorSecurityInfo::getTimestampManipulationCount() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mTimestampManipulationCount;
}

uint64_t COperatorSecurityInfo::getPowWaveAttackCount() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mPowWaveAttackCount;
}

std::vector<std::string> COperatorSecurityInfo::getDetailedReports() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mDetailedReports;
}

uint64_t COperatorSecurityInfo::getBlocksMined() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mBlocksMined;
}

uint64_t COperatorSecurityInfo::getBlocksMinedDuringDifficultyDecrease() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mBlocksMinedDuringDifficultyDecrease;
}

uint64_t COperatorSecurityInfo::getBlocksMinedDuringDifficultyIncrease() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mBlocksMinedDuringDifficultyIncrease;
}

uint64_t COperatorSecurityInfo::getBlocksMinedDuringAnomalousIntervals() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mBlocksMinedDuringAnomalousIntervals;
}

std::vector<uint64_t> COperatorSecurityInfo::getSolvedTimes() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mSolvedTimes;
}

uint64_t COperatorSecurityInfo::getLastReportedAtHeight() const {
    std::lock_guard<std::mutex> lock(mGuardian);
    return mLastReportedAtHeight;
}

// Serialization
bool COperatorSecurityInfo::getPackedData(std::vector<uint8_t>& bytes) const {
    std::lock_guard<std::mutex> lock(mGuardian);
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::DER_Encoder enc;

        enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
            // Basic security metrics
            .encode(tools->stringToBytes(mOperatorID), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<uint64_t>(mTimestampManipulationCount))
            .encode(static_cast<uint64_t>(mPowWaveAttackCount))

            // Activity tracking data
            .encode(static_cast<uint64_t>(mBlocksMined))
            .encode(static_cast<uint64_t>(mBlocksMinedDuringDifficultyDecrease))
            .encode(static_cast<uint64_t>(mBlocksMinedDuringDifficultyIncrease))
            .encode(static_cast<uint64_t>(mBlocksMinedDuringAnomalousIntervals))
            .encode(static_cast<uint64_t>(mLastReportedAtHeight))

            // EWMA and CUSUM statistics - using proper double serialization
            .encode(tools->doubleToByteVector(mEWMAMiningRate), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->doubleToByteVector(mEWMAAlpha), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->doubleToByteVector(mCusumPositive), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->doubleToByteVector(mCusumNegative), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->doubleToByteVector(mCusumThreshold), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<uint64_t>(mLastBlockHeight))
            .encode(static_cast<uint64_t>(mLastBlockTime))
            .encode(static_cast<uint64_t>(mPositiveAnomalies))
            .encode(static_cast<uint64_t>(mNegativeAnomalies))
            .encode(tools->doubleToByteVector(mConfidenceLevel), Botan::ASN1_Tag::OCTET_STRING);

        // Encode solved times sequence
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& time : mSolvedTimes) {
            enc.encode(static_cast<uint64_t>(time));
        }
        enc.end_cons();

        // Encode detailed reports sequence
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& report : mDetailedReports) {
            enc.encode(tools->stringToBytes(report), Botan::ASN1_Tag::OCTET_STRING);
        }
        enc.end_cons();

        enc.end_cons();

        bytes = enc.get_contents_unlocked();
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}
// Deserialization
std::shared_ptr<COperatorSecurityInfo> COperatorSecurityInfo::instantiate(const std::vector<uint8_t>& packedData) {
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::BER_Decoder dec(packedData);

        auto result = std::make_shared<COperatorSecurityInfo>();
        std::vector<uint8_t> temp;

        dec.start_cons(Botan::ASN1_Tag::SEQUENCE);

        // Decode basic security metrics
        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mOperatorID = tools->bytesToString(temp);

        uint64_t tempUint64;
        dec.decode(tempUint64);
        result->mTimestampManipulationCount = tempUint64;

        dec.decode(tempUint64);
        result->mPowWaveAttackCount = tempUint64;

        // Decode activity tracking data
        dec.decode(tempUint64);
        result->mBlocksMined = tempUint64;

        dec.decode(tempUint64);
        result->mBlocksMinedDuringDifficultyDecrease = tempUint64;

        dec.decode(tempUint64);
        result->mBlocksMinedDuringDifficultyIncrease = tempUint64;

        dec.decode(tempUint64);
        result->mBlocksMinedDuringAnomalousIntervals = tempUint64;

        dec.decode(tempUint64);
        result->mLastReportedAtHeight = tempUint64;

        // Decode EWMA and CUSUM statistics - using proper double deserialization
        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mEWMAMiningRate = tools->bytesToDouble(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mEWMAAlpha = tools->bytesToDouble(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mCusumPositive = tools->bytesToDouble(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mCusumNegative = tools->bytesToDouble(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mCusumThreshold = tools->bytesToDouble(temp);

        dec.decode(tempUint64);
        result->mLastBlockHeight = tempUint64;

        dec.decode(tempUint64);
        result->mLastBlockTime = tempUint64;

        dec.decode(tempUint64);
        result->mPositiveAnomalies = tempUint64;

        dec.decode(tempUint64);
        result->mNegativeAnomalies = tempUint64;

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        result->mConfidenceLevel = tools->bytesToDouble(temp);

        // Decode solved times sequence
        Botan::BER_Decoder solvedTimesDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (solvedTimesDec.more_items()) {
            uint64_t time;
            solvedTimesDec.decode(time);
            result->mSolvedTimes.push_back(time);
        }
        solvedTimesDec.end_cons();

        // Decode detailed reports sequence
        Botan::BER_Decoder reportsDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (reportsDec.more_items()) {
            reportsDec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
            result->mDetailedReports.push_back(tools->bytesToString(temp));
        }
        reportsDec.end_cons();

        dec.end_cons();

        return result;
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
    }
}
// Input validation
void COperatorSecurityInfo::validateInputs() {
    if (mOperatorID.empty()) {
        throw std::invalid_argument("Operator ID cannot be empty");
    }
    if (mTimestampManipulationCount < 0 || mPowWaveAttackCount < 0) {
        throw std::invalid_argument("Security metrics cannot be negative");
    }
}
