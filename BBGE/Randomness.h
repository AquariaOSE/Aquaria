#ifndef RANDOMNESS_H
#define RANDOMNESS_H

#include <stdlib.h>

#define ASCON_SPONGE_TYPES_ONLY
#include "ascon-sponge.h"


namespace Randomness
{
	// Call this once on startup
	void init(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d);

	// Call this once per frame with some number
	void addEntropy(uint64_t x);

	uint64_t getBits64();
	uint32_t getBits32();
	float getFloat01(); // in [0..1)
}


class SplitMix64
{
public:
	SplitMix64();
	SplitMix64(uint64_t seed);
	uint64_t next();
	uint64_t state;
};


class SlowRand
{
public:
	SlowRand();
	void absorb(const uint64_t *seed, size_t n);
	void absorb(uint64_t val);
	uint64_t next();
	uint64_t duplex(uint64_t in); // extract one, put one back in
private:
	ascon_state_t ascon;
};

// Xoshiro128+ actually
struct Xoshiro128
{
public:
	Xoshiro128();
	Xoshiro128(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
	uint32_t next();
	float f01();

	union
	{
		uint32_t s[4];
		uint64_t q[2];
	} u;
};

struct FastRand
{
	FastRand();
	FastRand(uint64_t state);

	uint32_t next();
	float f01();

	uint64_t state;
};


#endif
