#include "base58.h"

#include "uint256.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <string>

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
bool DecodeBase58(const std::string& inCharVec, std::vector<unsigned char>& vch)
{
	uint32_t psz = 0;
	// Skip leading spaces.
	while (psz < inCharVec.size())
	{
		if (!(inCharVec[psz] >= -1 && inCharVec[psz] <= 255))
			return false;
		if (isspace(inCharVec[psz]))
			psz++;
		else
			break;
	}
	// Skip and count leading '1's.
	int zeroes = 0;
	int length = 0;
	while (inCharVec[psz] == '1') {
		zeroes++;
		psz++;
	}
	// Allocate enough space in big-endian base256 representation.
	size_t size = inCharVec.size() * 733 / 1000 + 1; // log(58) / log(256), rounded up.
	std::vector<unsigned char> b256(size);
	// Process the characters.
	while (psz < inCharVec.size() && !isspace(inCharVec[psz])) {
		// Decode base58 character
		const char* ch = strchr(pszBase58, inCharVec[psz]);
		if (ch == nullptr)
			return false;
		// Apply "b256 = b256 * 58 + ch".
		int carry = ch - pszBase58;
		int i = 0;
		for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
			carry += 58 * (*it);
			*it = carry % 256;
			carry /= 256;
		}
	 assertGN(carry == 0);
		length = i;
		psz++;
	}
	// Skip trailing spaces.
	while (isspace(inCharVec[psz]))
		psz++;
	if (inCharVec[psz] != 0)
		return false;
	// Skip leading zeroes in b256.
	std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
	while (it != b256.end() && *it == 0)
		it++;
	// Copy result into output vector.
	vch.reserve(zeroes + (b256.end() - it));
	vch.assign(zeroes, 0x00);
	while (it != b256.end())
		vch.push_back(*(it++));
	return true;
}

std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
	// Skip & count leading zeroes.
	int zeroes = 0;
	int length = 0;
	while (pbegin != pend && *pbegin == 0) {
		pbegin++;
		zeroes++;
	}
	// Allocate enough space in big-endian base58 representation.
	int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
	std::vector<unsigned char> b58(size);
	// Process the bytes.
	while (pbegin != pend) {
		int carry = *pbegin;
		int i = 0;
		// Apply "b58 = b58 * 256 + ch".
		for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
			carry += 256 * (*it);
			*it = carry % 58;
			carry /= 58;
		}

	 assertGN(carry == 0);
		length = i;
		pbegin++;
	}

	// Skip leading zeroes in base58 result.
	std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
	while (it != b58.end() && *it == 0)
		it++;
	// Translate the result into a string.
	std::string str;
	str.reserve(zeroes + (b58.end() - it));
	str.assign(zeroes, '1');
	while (it != b58.end())
		str += pszBase58[*(it++)];
	return str;
}

std::string EncodeBase58(const std::vector<unsigned char>& vch)
{
	return EncodeBase58(vch.data(), vch.data() + vch.size());
}


bool DecodeBase58(const std::string& str, Botan::secure_vector<unsigned char>& vchRet)
{
	return DecodeBase58(str.c_str(), vchRet);
}



std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn)
{
	std::unique_ptr<Botan::HashFunction> hash(Botan::HashFunction::create("SHA-256"));//Not it's the older NOt the SHA3-256
	// add 4-byte hash check to the end
	std::vector<uint8_t > vch(vchIn);
	//hash->
	hash->update(vch);
	
	std::vector<uint8_t> hasz = Botan::unlock(hash->final());
	//uint256 hashe(hasz);

	vch.insert(vch.end(), (unsigned char*)hasz.data(), (unsigned char*)hasz.data() + 4);
	return EncodeBase58(vch);
}

 bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
{
	std::unique_ptr<Botan::HashFunction> hash(Botan::HashFunction::create("SHA-256")); // Note it's the older NOt the SHA3-256
	if (!DecodeBase58(str, vchRet) ||
		(vchRet.size() < 4)) {
		vchRet.clear();
		return false;
	}
	std::vector<uint8_t> check; check.resize(4);
	std::memcpy(check.data(), vchRet.data() + vchRet.size() - 4, 4);
	vchRet.resize(vchRet.size() - 4);
	hash->update(vchRet);
	std::vector<uint8_t> hasz = Botan::unlock(hash->final());

	if (memcmp(hasz.data(), check.data(), 4) != 0) {
		vchRet.clear();
		return false;
	}
	return true;
}

 bool DecodeBase58Check(const std::string & str, Botan::secure_vector<unsigned char>& vchRet)
 {
	 std::vector<uint8_t> vec;
	 if (!DecodeBase58Check(str, vec))
		 return false;
	 vchRet = Botan::secure_vector<uint8_t>(vchRet.begin(), vchRet.end());
	 return true;
 }


