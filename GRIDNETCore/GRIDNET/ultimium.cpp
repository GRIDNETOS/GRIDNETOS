#include "Ultimium.h"
#pragma once
extern "C" {
#include "./cryptohash/sph_blake.h"
#include "./cryptohash/sph_jh.h"7
#include "./cryptohash/sph_keccak.h"
#include "./cryptohash/sph_skein.h"
#include "./cryptohash/sph_hamsi.h"
}
#include "Worker.h"
#include "CryptoFactory.h"




#define _ALIGN(x) __declspec(align(x))
CUltimium::CUltimium()
{
	cc_blake = new sph_blake512_context();
	cc_keccak = new sph_keccak512_context();
	cc_jh = new sph_jh512_context();
	cc_skein = new sph_skein512_context();
	cc_shavite = new sph_shavite512_context();
	cc_hamsi = new sph_hamsi512_context();

	sph_blake512_init(cc_blake);
	sph_keccak512_init(cc_keccak);
	sph_jh512_init(cc_jh);
	sph_skein512_init(cc_skein);
	sph_shavite512_init(cc_shavite);
	sph_hamsi512_init(cc_hamsi);
}
CUltimium::~CUltimium()
{
	if (cc_blake != nullptr) {
		delete cc_blake;
		cc_blake = nullptr;
	}

	if (cc_jh != nullptr) {
		delete cc_jh;
		cc_jh = nullptr;
	}

	if (cc_keccak != nullptr) {
		delete cc_keccak;
		cc_keccak = nullptr;
	}

	if (cc_shavite != nullptr) {
		delete cc_shavite;
		cc_shavite = nullptr;
	}

	if (cc_skein != nullptr) {
		delete cc_skein;
		cc_skein = nullptr;
	}

	if (cc_hamsi != nullptr) {
		delete cc_hamsi;
		cc_hamsi = nullptr;
	}
}


inline void be32enc(void *pp, unsigned int  x)
{
	unsigned char *p = (unsigned char *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}



	std::array<unsigned char, 32> CUltimium::digest32(const std::vector<uint8_t>& data)
	{
		
		std::array<unsigned char, 64> powT{};
		std::array<unsigned char, 32> pow{};
		powT = digest64(data);
		std::memcpy(&pow[0], &powT[0], 32);
		return pow;
	}
	std::array<unsigned char, 64> CUltimium::digestBlake(const std::vector<uint8_t>& data)
	{

		std::array<unsigned char, 64> hash{};
		
		size_t mSS = data.size();
		sph_blake512(cc_blake, data.data(), data.size());
		sph_blake512_close(cc_blake, hash.data());
		return hash;
	}
	typedef union {
		unsigned char h1[64];
		unsigned int h4[16];
		unsigned long long h8[8];
	} hash_t;

	typedef struct hsd
	{
		hash_t cross_h;
		hash_t temp_h;
	} h_pair_t;

	std::array<unsigned char, 64> CUltimium::digest64(const std::vector<uint8_t>& data)
	{

		std::array<unsigned char, 64> pow{};
		std::array<unsigned char, 64> temp{};
		//std::array<unsigned char, 64> tempB{};
		std::array<unsigned char, 64> crossTemp{};

		size_t len = data.size();


		//1
		sph_blake512(cc_blake, data.data(), data.size());
		sph_blake512_close(cc_blake, temp.data());
		std::memcpy(crossTemp.data(), temp.data(), 32);

		//2
		sph_keccak512(cc_keccak, temp.data(), 64);
		sph_keccak512_close(cc_keccak, temp.data());
		std::memcpy(&crossTemp[32], &temp[32], 32);

	

		
		//3
		sph_hamsi512(cc_hamsi, crossTemp.data(), 64);
		sph_hamsi512_close(cc_hamsi, temp.data());
		std::memcpy(crossTemp.data(), temp.data(), 32);

		

		
		//4
		sph_shavite512(cc_shavite, &temp[0], 64);
		sph_shavite512_close(cc_shavite, &temp[0]);
		std::memcpy(&crossTemp[32], &temp[32], 32);
		
		//5
		//hash_t * cross_h = (hash_t*)crossTemp.data();
		//cl_ulong test= cross_h->h8[0];
		sph_jh512(cc_jh, crossTemp.data(), 64);
		sph_jh512_close(cc_jh, temp.data());


	
	
		//6
		sph_skein512(cc_skein, &temp[0], 64);
		sph_skein512_close(cc_skein, &temp[0]);	

		///test
		std::memcpy(pow.data(), temp.data(), 64);
		//end test
		return pow;
	}
