/**
 * @file GridScriptParser.cpp
 * @brief Implements the CSendCommandParsingResult class and parseSendCommand function for GridScript parsing.
 * @author AI Assistant
 * @date 2024-09-03
 *
 * ╔════════════════════════════════════════════════════════════════════════════════════════════╗
 * ║                              ⚠️  CRITICAL CONSENSUS WARNING ⚠️                              ║
 * ╠════════════════════════════════════════════════════════════════════════════════════════════╣
 * ║                                                                                            ║
 * ║  This file implements CONSENSUS-CRITICAL GridScript parsing.                               ║
 * ║                                                                                            ║
 * ║  Parser output is used to write metadata to TXStatsContainerID in the Merkle Patricia     ║
 * ║  Trie state database. Different parser outputs cause different blockchain perspectives    ║
 * ║  and consensus splits.                                                                     ║
 * ║                                                                                            ║
 * ║  MODIFYING PARSER BEHAVIOR REQUIRES AN OBLIGATORY SOFT FORK!                               ║
 * ║                                                                                            ║
 * ║  SAFE MODIFICATION PROCESS:                                                                ║
 * ║    1. Create NEW parser version function (e.g., parseSendCommand_V3)                      ║
 * ║    2. NEVER modify existing version implementations (Legacy, Cumulative, etc.)            ║
 * ║    3. Update parseSendCommand() wrapper to handle new version                             ║
 * ║    4. Add version entry in CGlobalSecSettings::getGridScriptParserVersion()               ║
 * ║    5. Coordinate SOFT FORK with network (set future keyHeight threshold)                  ║
 * ║    6. Test extensively before activation                                                   ║
 * ║                                                                                            ║
 * ║  See detailed instructions in CGlobalSecSettings.cpp:getGridScriptParserVersion()         ║
 * ║                                                                                            ║
 * ╚════════════════════════════════════════════════════════════════════════════════════════════╝
 */
#include "ScriptEngine.h"
#include "GridScriptParser.h"
#include "CGlobalSecSettings.h"
#include <sstream>
#include <stdexcept>

CSendCommandParsingResult::CSendCommandParsingResult() : mSuccess(false), mAmount(0) {}

void CSendCommandParsingResult::setSuccess(bool success) {
    mSuccess = success;
}

void CSendCommandParsingResult::setAmount(const BigInt& amount) {
    mAmount = amount;
}

void CSendCommandParsingResult::setRecipient(const std::string& recipient) {
    mRecipient = recipient;
}

bool CSendCommandParsingResult::isSuccess() const {
    return mSuccess;
}

const BigInt& CSendCommandParsingResult::getAmount() const {
    return mAmount;
}

const std::string& CSendCommandParsingResult::getRecipient() const {
    return mRecipient;
}

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <cctype>

// Assume CSendCommandParsingResult, CTools, BigInt, etc. are defined and included appropriately

// Forward declarations for internal implementation functions
std::shared_ptr<CSendCommandParsingResult> parseSendCommand_Legacy(const std::string& sourceCode);
std::shared_ptr<CSendCommandParsingResult> parseSendCommand_Cumulative(const std::string& sourceCode);

// ════════════════════════════════════════════════════════════════════════════════════════════════
//                            VERSION-AWARE PARSER ENTRY POINT
// ════════════════════════════════════════════════════════════════════════════════════════════════
//
// ⚠️  WARNING: This is the ONLY function that should be called externally!
//
// This wrapper function selects the appropriate parser version based on keyHeight.
// Parser version selection is defined in CGlobalSecSettings::getGridScriptParserVersion().
//
// TO ADD A NEW PARSER VERSION:
//   1. Implement new version below (e.g., parseSendCommand_V3)
//   2. Update the if/else chain below to include new version
//   3. Add version threshold in CGlobalSecSettings::getGridScriptParserVersion()
//   4. Coordinate SOFT FORK with network operators
//
// NEVER modify existing parser implementations (parseSendCommand_Legacy, parseSendCommand_Cumulative)!
// Always add NEW versions to maintain historical consensus compatibility.
//
// ════════════════════════════════════════════════════════════════════════════════════════════════

/**
 * @brief Main entry point for GridScript parsing (version-aware).
 *
 * This function selects the appropriate parser version based on keyHeight
 * to ensure consensus compatibility across different blockchain heights.
 *
 * @param sourceCode The GridScript source code to parse
 * @param keyHeight The block keyHeight (determines which parser version to use)
 * @return Parsing result containing amount, recipient, and success status
 */
std::shared_ptr<CSendCommandParsingResult> parseSendCommand(const std::string& sourceCode, uint64_t keyHeight) {
    // Get the parser version for this keyHeight
    uint64_t parserVersion = CGlobalSecSettings::getGridScriptParserVersion(keyHeight);

    if (parserVersion == 1) {
        // Legacy parser (r3962): Returns first send command only
        return parseSendCommand_Legacy(sourceCode);
    }
    else {
        // Cumulative parser (r4811): Returns sum of all send commands
        return parseSendCommand_Cumulative(sourceCode);
    }
}

// ════════════════════════════════════════════════════════════════════════════════════════════════
//                          PARSER VERSION 1: LEGACY (r3962)
// ════════════════════════════════════════════════════════════════════════════════════════════════
//
// ⛔ DO NOT MODIFY THIS IMPLEMENTATION! ⛔
//
// This parser version is required for consensus with historical blocks (keyHeight < 110300).
// Any modifications will break consensus and cause network fragmentation.
//
// If you need different behavior, create a NEW parser version instead.
//
// ════════════════════════════════════════════════════════════════════════════════════════════════

/**
 * @brief Legacy GridScript parser implementation (Version 1 - r3962).
 *
 * Returns amount and recipient from the FIRST send/sendEx command only.
 * This version was used up to r3962 and is required for consensus compatibility
 * with blocks having keyHeight < 110300.
 *
 * KEY BEHAVIOR:
 *   - Parses tokens sequentially
 *   - Returns FIRST send/sendEx command only
 *   - BREAKS after finding first send (does NOT continue parsing)
 *   - Used for: keyHeight < 110300
 *
 * ⛔ DO NOT MODIFY - Required for historical consensus! ⛔
 */
std::shared_ptr<CSendCommandParsingResult> parseSendCommand_Legacy(const std::string& sourceCode) {
    auto result = std::make_shared<CSendCommandParsingResult>();
    std::shared_ptr<CTools> tools = CTools::getInstance();

    if (!tools) {
        throw std::runtime_error("Failed to obtain CTools instance");
    }

    // Tokenize the source code, handling quoted strings
    std::vector<std::string> tokens;
    {
        std::istringstream iss(sourceCode);
        char c;
        bool inQuotes = false;
        char quoteChar = '\0';
        std::string token;

        while (iss.get(c)) {
            if (inQuotes) {
                if (c == quoteChar) {
                    inQuotes = false;
                    tokens.push_back(token);
                    token.clear();
                }
                else {
                    token += c;
                }
            }
            else {
                if (std::isspace(static_cast<unsigned char>(c))) {
                    if (!token.empty()) {
                        tokens.push_back(token);
                        token.clear();
                    }
                }
                else if (c == '\'' || c == '\"') {
                    inQuotes = true;
                    quoteChar = c;
                }
                else {
                    token += c;
                }
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // Stack to simulate GridScript stack
    std::vector<std::string> stack;

    // Variables to keep track of parsing
    bool foundSendCommand = false;

    // Iterate through tokens without making assumptions
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        if (token == "data") {
            // 'data' command, take next token and push onto stack
            if (i + 1 < tokens.size()) {
                ++i;
                stack.push_back(tokens[i]); // Push the data onto the stack
            }
            else {
                // No argument after 'data', invalid syntax
                continue; // Skip invalid 'data' command
            }
        }
        else if (token == "sendEx") {
            // 'sendEx' command, pop amount, recipient, and flags from stack
            if (stack.size() < 3) {
                continue; // Not enough items on stack, skip this command
            }
            // Pop amount, recipient, and flags
            std::string amountStr = stack.back();
            stack.pop_back();
            std::string recipient = stack.back();
            stack.pop_back();
            std::string sendFlagsStr = stack.back();
            stack.pop_back();

            // Parse flags
            unsigned int sendFlagsInt;
            try {
                sendFlagsInt = std::stoul(sendFlagsStr);
            }
            catch (const std::exception&) {
                // Invalid sendFlags
                continue; // Skip this command
            }
            SE::sendFlags sendFlags(sendFlagsInt);

            // Validate and convert amount
            try {
                BigInt amount;
                if (sendFlags.isInGNC) {
                    // Convert GNC to Atto units

                    if (!tools->isAlphaNumericStr(amountStr))
                    {
                        continue;
                    }

                    double gncAmount = std::stod(amountStr);
                    amount = tools->GNCToAtto(gncAmount);
                }
                else {

                    if (!tools->isValidDecimalNumber(amountStr))
                    {
                        continue;
                    }
                    amount = BigInt(amountStr);
                }
                result->setSuccess(true);
                result->setAmount(amount);
                result->setRecipient(recipient);
                //result->setIsGNC(sendFlags.isInGNC);
            }
            catch (const std::exception&) {
                // Invalid amount
                continue; // Skip this command
            }

            foundSendCommand = true;
            break; // LEGACY BEHAVIOR: Stop after first send command
        }
        else if (token == "send") {
            // 'send' command, expect next tokens as recipient and amount
            size_t remainingTokens = tokens.size() - i - 1;
            if (remainingTokens < 2) {
                continue; // Not enough tokens, skip this command
            }
            std::string recipient = tokens[++i];
            std::string amountStr = tokens[++i];

            // Collect optional flags
            bool isGNC = false;
            bool notax = false;
            while (i + 1 < tokens.size() && tokens[i + 1].rfind("-", 0) == 0) { // Token starts with '-'
                std::string flag = tokens[++i];
                if (flag == "-gnc" || flag == "-GNC") {
                    isGNC = true;
                }
                else if (flag == "-notax") {
                    notax = true;
                }
                // Handle other flags if necessary
            }

            // Validate and convert amount
            try {
                BigInt amount;
                if (isGNC) {
                    // Convert GNC to Atto units
                    double gncAmount = std::stod(amountStr);
                    amount = tools->GNCToAtto(gncAmount);
                }
                else {
                    // Since amount is in Atto units, ensure it's an integer
                    if (amountStr.find('.') != std::string::npos) {
                        // Invalid amount format for Atto units
                        continue; // Skip this command
                    }

                    // CVE_607
                    if (!tools->isValidDecimalNumber(amountStr))
                    {
                        continue;
                    }

                    amount = BigInt(amountStr);
                }
                result->setSuccess(true);
                result->setAmount(amount);
                result->setRecipient(recipient);
                //result->setIsGNC(isGNC);
            }
            catch (const std::exception&) {
                // Invalid amount
                continue; // Skip this command
            }

            foundSendCommand = true;
            break; // LEGACY BEHAVIOR: Stop after first send command
        }
        else if (token == "cd") {
            // 'cd' command, skip next token (path), but don't make assumptions
            if (i + 1 < tokens.size()) {
                ++i; // Skip path
            }
            // Continue processing other tokens
        }
        else {
            // For other tokens, push numbers and data onto the stack
            // Check if token is a number (including negative numbers)
            if (std::isdigit(token[0]) || (token[0] == '-' && token.size() > 1 && std::isdigit(token[1]))) {
                // Numbers are pushed onto the stack
                stack.push_back(token);
            }
            else {
                // Ignore unknown keywords
                continue;
            }
        }
    }

    if (!foundSendCommand) {
        // 'send' or 'sendEx' command not found
        result->setSuccess(false);
    }

    return result;
}

// ════════════════════════════════════════════════════════════════════════════════════════════════
//                          PARSER VERSION 2: CUMULATIVE (r4811)
// ════════════════════════════════════════════════════════════════════════════════════════════════
//
// ⛔ DO NOT MODIFY THIS IMPLEMENTATION! ⛔
//
// This parser version is required for consensus with blocks having keyHeight >= 110300.
// Any modifications will break consensus and cause network fragmentation.
//
// If you need different behavior, create a NEW parser version instead.
//
// ════════════════════════════════════════════════════════════════════════════════════════════════

/**
 * @brief Cumulative GridScript parser implementation (Version 2 - r4811).
 *
 * Returns cumulative amount from ALL send/sendEx commands.
 * This version is used for blocks with keyHeight >= 110300.
 *
 * KEY BEHAVIOR:
 *   - Parses ALL tokens in source code
 *   - Accumulates amounts from ALL send/sendEx commands
 *   - Does NOT break after first send (continues parsing entire script)
 *   - Keeps track of first recipient for backward compatibility
 *   - Used for: keyHeight >= 110300
 *
 * ⛔ DO NOT MODIFY - Required for consensus! ⛔
 */
std::shared_ptr<CSendCommandParsingResult> parseSendCommand_Cumulative(const std::string& sourceCode) {
    auto result = std::make_shared<CSendCommandParsingResult>();
    std::shared_ptr<CTools> tools = CTools::getInstance();

    if (!tools) {
        throw std::runtime_error("Failed to obtain CTools instance");
    }

    // Tokenize the source code, handling quoted strings
    std::vector<std::string> tokens;
    {
        std::istringstream iss(sourceCode);
        char c;
        bool inQuotes = false;
        char quoteChar = '\0';
        std::string token;

        while (iss.get(c)) {
            if (inQuotes) {
                if (c == quoteChar) {
                    inQuotes = false;
                    tokens.push_back(token);
                    token.clear();
                }
                else {
                    token += c;
                }
            }
            else {
                if (std::isspace(static_cast<unsigned char>(c))) {
                    if (!token.empty()) {
                        tokens.push_back(token);
                        token.clear();
                    }
                }
                else if (c == '\'' || c == '\"') {
                    inQuotes = true;
                    quoteChar = c;
                }
                else {
                    token += c;
                }
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    // Stack to simulate GridScript stack
    std::vector<std::string> stack;

    // Variables to keep track of parsing
    bool foundSendCommand = false;
    BigInt cumulativeAmount = 0; // Accumulate total amount from all send operations
    std::string firstRecipient; // Keep track of first recipient for backwards compatibility

    // Iterate through tokens without making assumptions
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        if (token == "data") {
            // 'data' command, take next token and push onto stack
            if (i + 1 < tokens.size()) {
                ++i;
                stack.push_back(tokens[i]); // Push the data onto the stack
            }
            else {
                // No argument after 'data', invalid syntax
                continue; // Skip invalid 'data' command
            }
        }
        else if (token == "sendEx") {
            // 'sendEx' command, pop amount, recipient, and flags from stack
            if (stack.size() < 3) {
                continue; // Not enough items on stack, skip this command
            }
            // Pop amount, recipient, and flags
            std::string amountStr = stack.back();
            stack.pop_back();
            std::string recipient = stack.back();
            stack.pop_back();
            std::string sendFlagsStr = stack.back();
            stack.pop_back();

            // Parse flags
            unsigned int sendFlagsInt;
            try {
                sendFlagsInt = std::stoul(sendFlagsStr);
            }
            catch (const std::exception&) {
                // Invalid sendFlags
                continue; // Skip this command
            }
            SE::sendFlags sendFlags(sendFlagsInt);

            // Validate and convert amount
            try {
                BigInt amount;
                if (sendFlags.isInGNC) {
                    // Convert GNC to Atto units

                    if (!tools->isAlphaNumericStr(amountStr))
                    {
                        continue;
                    }

                    double gncAmount = std::stod(amountStr);
                    amount = tools->GNCToAtto(gncAmount);
                }
                else {

                    if (!tools->isValidDecimalNumber(amountStr))
                    {
                        continue;
                    }
                    amount = BigInt(amountStr);
                }

                // Accumulate amount from this send operation
                cumulativeAmount += amount;

                // Keep track of first recipient for backwards compatibility
                if (!foundSendCommand) {
                    firstRecipient = recipient;
                    foundSendCommand = true;
                }
                //result->setIsGNC(sendFlags.isInGNC);
            }
            catch (const std::exception&) {
                // Invalid amount
                continue; // Skip this command
            }

            // Don't break - continue parsing to find more send operations
        }
        else if (token == "send") {
            // 'send' command, expect next tokens as recipient and amount
            size_t remainingTokens = tokens.size() - i - 1;
            if (remainingTokens < 2) {
                continue; // Not enough tokens, skip this command
            }
            std::string recipient = tokens[++i];
            std::string amountStr = tokens[++i];

            // Collect optional flags
            bool isGNC = false;
            bool notax = false;
            while (i + 1 < tokens.size() && tokens[i + 1].rfind("-", 0) == 0) { // Token starts with '-'
                std::string flag = tokens[++i];
                if (flag == "-gnc" || flag == "-GNC") {
                    isGNC = true;
                }
                else if (flag == "-notax") {
                    notax = true;
                }
                // Handle other flags if necessary
            }

            // Validate and convert amount
            try {
                BigInt amount;
                if (isGNC) {
                    // Convert GNC to Atto units
                    double gncAmount = std::stod(amountStr);
                    amount = tools->GNCToAtto(gncAmount);
                }
                else {
                    // Since amount is in Atto units, ensure it's an integer
                    if (amountStr.find('.') != std::string::npos) {
                        // Invalid amount format for Atto units
                        continue; // Skip this command
                    }

                    // CVE_607
                    if (!tools->isValidDecimalNumber(amountStr))
                    {
                        continue;
                    }

                    amount = BigInt(amountStr);
                }

                // Accumulate amount from this send operation
                cumulativeAmount += amount;

                // Keep track of first recipient for backwards compatibility
                if (!foundSendCommand) {
                    firstRecipient = recipient;
                    foundSendCommand = true;
                }
                //result->setIsGNC(isGNC);
            }
            catch (const std::exception&) {
                // Invalid amount
                continue; // Skip this command
            }

            // Don't break - continue parsing to find more send operations
        }
        else if (token == "cd") {
            // 'cd' command, skip next token (path), but don't make assumptions
            if (i + 1 < tokens.size()) {
                ++i; // Skip path
            }
            // Continue processing other tokens
        }
        else {
            // For other tokens, push numbers and data onto the stack
            // Check if token is a number (including negative numbers)
            if (std::isdigit(token[0]) || (token[0] == '-' && token.size() > 1 && std::isdigit(token[1]))) {
                // Numbers are pushed onto the stack
                stack.push_back(token);
            }
            else {
                // Ignore unknown keywords
                continue;
            }
        }
    }

    if (!foundSendCommand) {
        // 'send' or 'sendEx' command not found
        result->setSuccess(false);
    }
    else {
        // Set the cumulative amount from all send operations
        result->setSuccess(true);
        result->setAmount(cumulativeAmount);
        result->setRecipient(firstRecipient);
    }

    return result;
}
