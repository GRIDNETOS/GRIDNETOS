#pragma once

/**
 * @file GridScriptParser.hpp
 * @brief Defines the CSendCommandParsingResult class and parseSendCommand function for GridScript parsing.
 * @author AI Assistant
 * @date 2024-09-03
 *
 * ╔════════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              ⚠️  CRITICAL CONSENSUS WARNING ⚠️                              ║
 * ╠════════════════════════════════════════════════════════════════════════════════════════════╣
 * ║                                                                                            ║
 * ║  This parser is CONSENSUS-CRITICAL. Any changes to parsing behavior will affect           ║
 * ║  the blockchain state (TXStatsContainerID in MPT) and WILL cause consensus splits!        ║
 * ║                                                                                            ║
 * ║  BEFORE MODIFYING THIS PARSER:                                                             ║
 * ║    1. Understand that changes require a SOFT FORK                                          ║
 * ║    2. Implement new version (e.g., parseSendCommand_V3) - NEVER modify existing versions  ║
 * ║    3. Add compatibility entry in CGlobalSecSettings::getGridScriptParserVersion()          ║
 * ║    4. Coordinate soft fork with all network operators                                      ║
 * ║    5. Test thoroughly before activation keyHeight                                          ║
 * ║                                                                                            ║
 * ║  See CGlobalSecSettings.cpp:getGridScriptParserVersion() for detailed instructions.       ║
 * ║                                                                                            ║
 * ╚════════════════════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef GRIDSCRIPT_PARSER_HPP
#define GRIDSCRIPT_PARSER_HPP

#include <string>
#include <memory>
#include "tools.h"  // Assuming this header contains the BigInt definition

 /**
  * @class CSendCommandParsingResult
  * @brief Represents the result of parsing a 'send' command in GridScript.
  *
  * This class encapsulates the outcome of parsing a 'send' command,
  * including whether the parsing was successful, the amount to be sent (in atto units),
  * and the recipient of the transaction.
  */
class CSendCommandParsingResult {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a CSendCommandParsingResult object with default values.
     */
    CSendCommandParsingResult();

    /**
     * @brief Setter for the success status.
     * @param success Boolean indicating whether parsing was successful.
     */
    void setSuccess(bool success);

    /**
     * @brief Setter for the amount.
     * @param amount BigInt representing the amount in atto units.
     */
    void setAmount(const BigInt& amount);

    /**
     * @brief Setter for the recipient.
     * @param recipient String representing the recipient's address or identifier.
     */
    void setRecipient(const std::string& recipient);

    /**
     * @brief Getter for the success status.
     * @return Boolean indicating whether parsing was successful.
     */
    bool isSuccess() const;

    /**
     * @brief Getter for the amount.
     * @return BigInt representing the amount in atto units.
     */
    const BigInt& getAmount() const;

    /**
     * @brief Getter for the recipient.
     * @return String representing the recipient's address or identifier.
     */
    const std::string& getRecipient() const;

private:
    bool mSuccess;        ///< Indicates whether parsing was successful
    BigInt mAmount;       ///< The amount to be sent, in atto units
    std::string mRecipient; ///< The recipient's address or identifier
};

/**
 * @brief Parses a GridScript 'send' command from the given source code (version-aware).
 *
 * This function searches for a 'send' command within the provided GridScript source code,
 * extracts the relevant information (recipient and amount), and returns the result
 * as a CSendCommandParsingResult object.
 *
 * The parsing algorithm used depends on the keyHeight parameter:
 * - Version 1 (keyHeight < 110300): Legacy parser - returns first send command only
 * - Version 2 (keyHeight >= 110300): Cumulative parser - returns sum of all send commands
 *
 * @param sourceCode The GridScript source code to parse.
 * @param keyHeight The block key-height (determines parser version for consensus compatibility).
 *                  Default value of 0 will use the appropriate version based on CGlobalSecSettings.
 * @return std::shared_ptr<CSendCommandParsingResult> A shared pointer to the parsing result.
 * @throw std::runtime_error If CTools singleton instance cannot be obtained.
 */
std::shared_ptr<CSendCommandParsingResult> parseSendCommand(const std::string& sourceCode, uint64_t keyHeight = 0);

#endif // GRIDSCRIPT_PARSER_HPP