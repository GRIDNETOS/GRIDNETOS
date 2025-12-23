#ifndef ULTIMIUM_H
#define ULTIMIUM_H
#include "stdafx.h"
#include <iostream>
#include <string>
extern "C" {
#include "./cryptohash/sph_blake.h"
#include "./cryptohash/sph_jh.h"
#include "./cryptohash/sph_keccak.h"
//#include "./cryptohash/sph_sha3.h"
#include "./cryptohash/sph_skein.h"
#include "./cryptohash/sph_shavite.h"
#include "./cryptohash/sph_hamsi.h"
}
class CUltimium {
private:
	//contexts needed by hash functions
	sph_blake512_context *cc_blake; 
	sph_keccak512_context *cc_keccak; 
	sph_jh512_context *cc_jh; 
	sph_skein512_context *cc_skein;
	sph_shavite512_context *cc_shavite;
	sph_hamsi512_context *cc_hamsi;
public:
	std::array<unsigned char, 32> digest32(const std::vector<uint8_t> &data);
	std::array<unsigned char, 64> digest64(const std::vector<uint8_t> & data);
	std::array<unsigned char, 64> digestBlake(const std::vector<uint8_t>& data);
	CUltimium();
	~CUltimium();



};

#endif
