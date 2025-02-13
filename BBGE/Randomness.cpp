// Must be included first to put the implementation here
#include "ascon-sponge.h"

#include "Base.h"
#include "Randomness.h"
#include <stdlib.h>
#include <time.h>
#include <SDL.h>


static SlowRand s_sysrand;
static SplitMix64 s_splitmix;
static Xoshiro128 s_xosh;
static FastRand s_fastr;


static inline uint32_t rotl32(const uint32_t x, uint32_t k)
{
	return (x << k) | (x >> (32u - k));
}

// via https://blog.bithole.dev/blogposts/random-float/
static inline float int_to_float01(uint32_t r)
{
	union { uint32_t u32; float f; } u;
	u.u32 = (r >> 9u) | 0x3f800000;
	return u.f - 1.0f;
}



void Randomness::init(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d)
{
	uint64_t v[] =
	{
		uint64_t(a),
		uint64_t(rand()),
		uint64_t(rand()),
		uint64_t(rand()),
		uint64_t(time(NULL)),
		uint64_t(clock()),
		uint64_t(&SDL_GetTicks),
		uint64_t(&time),
		uint64_t(&s_sysrand),
		// These functions should be ok to call without calling SDL_Init() first
		uint64_t(SDL_GetThreadID(NULL)),
#if SDL_VERSION_ATLEAST(2, 0, 0)
		uint64_t(SDL_GetPerformanceCounter()),
		uint64_t(SDL_GetPerformanceFrequency()),
		uint64_t(SDL_GetSystemRAM()),
		uint64_t(SDL_GetCPUCount())
#endif
	};
	s_sysrand.absorb(v, Countof(v));
	s_splitmix.state = s_sysrand.duplex(b);

	s_xosh.u.q[0] = s_sysrand.duplex(c);
	s_xosh.u.q[1] = s_sysrand.duplex(d);
}

void Randomness::addEntropy(uint64_t x)
{
	s_fastr.state ^= s_sysrand.duplex(x);
	s_sysrand.absorb(s_splitmix.next()); // pulling from splitmix permutes it
	s_sysrand.absorb(s_xosh.next());
}

uint64_t Randomness::getBits64()
{
	return s_splitmix.next();
}

uint32_t Randomness::getBits32()
{
	return s_fastr.next();
}

float Randomness::getFloat01()
{
	return s_fastr.f01();
}


SplitMix64::SplitMix64()
	: state(s_sysrand.next())
{
}

SplitMix64::SplitMix64(uint64_t seed)
	: state(seed)
{
}

uint64_t SplitMix64::next()
{
	uint64_t z = (state += UINT64_C(0x9e3779b97f4a7c15));
	z = (z ^ (z >> 30u)) * UINT64_C(0xbf58476d1ce4e5b9);
	z = (z ^ (z >> 27u)) * UINT64_C(0x94d049bb133111eb);
	return z ^ (z >> 31u);
}



SlowRand::SlowRand()
{
	ascon_sponge_init(&ascon);
}


void SlowRand::absorb(const uint64_t* seed, size_t n)
{
	ascon_sponge_absorb_blocks(&ascon, seed, n);
}

void SlowRand::absorb(uint64_t val)
{
	ascon_sponge_absorb_block(&ascon, val);
}

uint64_t SlowRand::duplex(uint64_t duplex)
{
	return ascon_sponge_duplex_block(&ascon, duplex);
}

uint64_t SlowRand::next()
{
	return ascon_sponge_squeeze_block(&ascon);
}

Xoshiro128::Xoshiro128()
{
	u.q[0] = s_splitmix.next();
	u.q[1] = s_sysrand.duplex((uintptr_t)this);
}

Xoshiro128::Xoshiro128(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	u.s[0] = a;
	u.s[1] = b;
	u.s[2] = c;
	u.s[3] = d;
}

uint32_t Xoshiro128::next()
{
	const uint32_t result = u.s[0] + u.s[3];
	const uint32_t t = u.s[1] << 9;
	u.s[2] ^= u.s[0];
	u.s[3] ^= u.s[1];
	u.s[1] ^= u.s[2];
	u.s[0] ^= u.s[3];
	u.s[2] ^= t;
	u.s[3] = rotl32(u.s[3], 11);
	return result;
}

float Xoshiro128::f01()
{
	return int_to_float01(next());
}

FastRand::FastRand()
	: state(s_sysrand.next())
{
}

FastRand::FastRand(uint64_t state)
	: state(state)
{
}

uint32_t FastRand::next()
{
	// SDL3's rand()
	state = state * 0xff1cd035u + 0x05;
	return state >> 32u;
}

float FastRand::f01()
{
	return int_to_float01(next());
}
