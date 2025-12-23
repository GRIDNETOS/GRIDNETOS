#pragma once
#include <list>
#include <vector>
#include <array>
#include <mutex>
#include <iostream>
#include <string>
#include <cstring>

#include "Settings.h"
#include "utilstrencodings.h"
#include "CryptoFactory.h"
#include "enums.h"

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) ((a) & (1ULL<<(b)))
#define BYTECODE_ID_UNSIGNED 1
#define BYTECODE_ID_SIGNED 2
#define BYTECODE_ID_DOUBLE 3
#define BYTECODE_ID_USER_OPCODE 4
#define BYTECODE_ID_STRING_LITERAL 5
#define GRIDSCRIPT_IMAGE_INIT_STRING "GRIDSCRIPT_V2_KEYWORD_IMAGE"

// Base opcode ID - codewords start at this ID
#define GRIDSCRIPT_BASE_OPCODE_ID 9

namespace SE {
	class CScriptEngine;
}

class CGridScriptCompiler
{
private:
	std::mutex mGuardian;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;
	std::shared_ptr<CTools> mTools;
	std::vector<uint8_t>  mCompiledByteCode;
	std::vector<uint8_t> mCurrentOpCode;
	uint64_t mCurrentOffset;
	void setOpCodeID(uint64_t opcodeID);
	bool mCompilingExtendedID;
	bool storeInlineLiteral(std::string &data);
	bool storeUserOpCode(uint64_t lowerLevelOpCodeIDToUse);
	bool storeNumerical(BigInt &number, uint8_t type);
	bool storeNumerical(BigSInt& number, uint8_t type);
	bool storeNumerical(BigFloat& number, uint8_t type);
	bool setLengthBits(size_t size, bool &wasExtLengthEncodingUsed);
	bool setContentBits(std::vector<uint8_t> content);
	bool setContentBits(uint64_t numerical);
	bool getContentLength(uint64_t &contentLength, bool movePointer = false);
	bool getContentBits(std::vector<uint8_t>& content, uint64_t length, bool movePointer);
	bool getIDBits(uint64_t & ID, bool movePointer);
	bool setIDBits(uint64_t ID);

	// V2 Hash chain support
	std::vector<uint8_t> mCurrentImageHash;
	uint64_t mBytecodeVersion;
	bool initializeKeywordHash();
	bool updateKeywordHash(const std::string& keywordName);
	std::vector<uint8_t> computeSHA256(const std::vector<uint8_t>& data);
	std::vector<uint8_t> getFinalKeywordHash() const;

public:
	// Production constructor - requires CTools and CBlockchainManager
	CGridScriptCompiler(std::shared_ptr<CTools> tools, std::shared_ptr<CBlockchainManager> bm);

	/// <summary>
	/// Standalone constructor for unit testing without full GRIDNET dependencies.
	/// Creates minimal CTools and CScriptEngine with nullptr dependencies.
	/// ScriptEngine is fully initialized with all codeword definitions via reset(true).
	/// </summary>
	/// <param name="bytecodeVersion">Bytecode version to use (default 2 for V2 format)</param>
	CGridScriptCompiler(uint64_t bytecodeVersion = 2);

	bool decompile(std::vector<uint8_t> bytecode, std::string & sourceCode, const std::vector<std::string> &codeWords= std::vector<std::string>());
	bool compile(std::string sourceCode, std::vector<uint8_t> &result);

	/// <summary>
	/// Compiles source and prepends V2 header (version byte + 32-byte hash)
	/// For cross-validation testing with JavaScript compiler
	/// </summary>
	bool compileWithHeader(const std::string& sourceCode, std::vector<uint8_t>& result);

	/// <summary>
	/// Returns the bytecode version of the most recently decompiled bytecode.
	/// Version 1: Legacy bytecode format
	/// Version 2+: New format with version marker and hash verification
	/// </summary>
	uint64_t getBytecodeVersion() const { return mBytecodeVersion; }

	/// <summary>
	/// Sets the bytecode version for compilation
	/// </summary>
	void setBytecodeVersion(uint64_t version) { mBytecodeVersion = version; }

};
