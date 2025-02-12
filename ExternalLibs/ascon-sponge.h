#ifndef ASCON_SPONGE_H
#define ASCON_SPONGE_H

// -- ascon-sponge.h --
// a single header library for a simple cryptographic sponge based on Ascon
//
// This library is based on the Ascon permutation, but is not a fully faithful
// implementation of any specified Ascon family member. Notably, it lacks
// padding of blocks.
//
// USAGE
// - as a single header library
//   - just include it and use it, all functions are inline
// - as a library compiled into a translation unit
//   - define ASCON_SPONGE_DECL_ONLY when including the header for usage
//   - define ASCON_SPONGE_IMPL_ONLY when compiling the translation unit
//   - all functions are non-inline in this use case
//
// CONFIGURATION
// - ASCON_SPONGE_ROUNDS
//   - the number of rounds of the permutation between each block
//   - must be between 1 and 16, should be between 6 and 12
//   - defaults to 8 (AsconHashA)
// - ASCON_SPONGE_IV
//   - when defined, ASCON_SPONGE_IV0 to ASCON_SPONGE_IV1 are used for
//     initialization
//   - defaults to the AsconHashA initialization constants

#include "minipstdint.h"
#include <stddef.h>

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
} ascon_state_t;

#ifndef ASCON_SPONGE_TYPES_ONLY

#ifndef ASCON_SPONGE_ROUNDS
// AsconHashA PB rounds
#define ASCON_SPONGE_ROUNDS 8
#endif

#ifndef ASCON_SPONGE_IV
#define ASCON_SPONGE_IV
// AsconHashA initialization constants
#define ASCON_SPONGE_IV0 UINT64_C(0x01470194fc6528a6)
#define ASCON_SPONGE_IV1 UINT64_C(0x738ec38ac0adffa7)
#define ASCON_SPONGE_IV2 UINT64_C(0x2ec8e3296c76384c)
#define ASCON_SPONGE_IV3 UINT64_C(0xd6f6a54d7f52377d)
#define ASCON_SPONGE_IV4 UINT64_C(0xa13c42a223be8d87)
#endif

#ifdef ASCON_SPONGE_DECL_ONLY

void ascon_sponge_init(ascon_state_t * s);
void ascon_sponge_permute(ascon_state_t * s, unsigned rounds);

void ascon_sponge_absorb_block(ascon_state_t * s, uint64_t n);
uint64_t ascon_sponge_squeeze_block(ascon_state_t * s);
uint64_t ascon_sponge_duplex_block(ascon_state_t * s, uint64_t n);
uint64_t ascon_sponge_inv_duplex_block(ascon_state_t * s, uint64_t n);

void ascon_sponge_absorb_blocks(ascon_state_t * s, const uint64_t * in, size_t len);
void ascon_sponge_squeeze_blocks(ascon_state_t * s, uint64_t * out, size_t len);
void ascon_sponge_duplex_blocks(ascon_state_t * s, const uint64_t * in, uint64_t * out, size_t len);
void ascon_sponge_inv_duplex_blocks(ascon_state_t * s, const uint64_t * in, uint64_t * out, size_t len);

#else

#ifdef ASCON_SPONGE_IMPL_ONLY
#define ASCON_INLINE
#else
#define ASCON_INLINE inline
#endif

ASCON_INLINE void ascon_sponge_init(ascon_state_t * s) {
    s->x0 = ASCON_SPONGE_IV0;
    s->x1 = ASCON_SPONGE_IV1;
    s->x2 = ASCON_SPONGE_IV2;
    s->x3 = ASCON_SPONGE_IV3;
    s->x4 = ASCON_SPONGE_IV4;
}

#define ASCON_ROR(x, n) (((x) >> (n)) ^ ((x) << (64 - (n))))
ASCON_INLINE void ascon_sponge_permute(ascon_state_t * s, unsigned rounds) {
    ascon_state_t t;

    for (unsigned i = 0, c = 0xf0; i < rounds; i++, c -= 0x0f) {
        /* round constant */
        s->x2 ^= c;

        /* sbox layer */
        s->x0 ^= s->x4;
        s->x4 ^= s->x3;
        s->x2 ^= s->x1;
        t.x0 = s->x0 ^ (~s->x1 & s->x2);
        t.x2 = s->x2 ^ (~s->x3 & s->x4);
        t.x4 = s->x4 ^ (~s->x0 & s->x1);
        t.x1 = s->x1 ^ (~s->x2 & s->x3);
        t.x3 = s->x3 ^ (~s->x4 & s->x0);
        t.x1 ^= t.x0;
        t.x3 ^= t.x2;
        t.x0 ^= t.x4;
        t.x2 = ~t.x2;

        /* linear layer */
        s->x0 = t.x0 ^ ASCON_ROR(t.x0, 19) ^ ASCON_ROR(t.x0, 28);
        s->x1 = t.x1 ^ ASCON_ROR(t.x1, 61) ^ ASCON_ROR(t.x1, 39);
        s->x2 = t.x2 ^ ASCON_ROR(t.x2,  1) ^ ASCON_ROR(t.x2,  6);
        s->x3 = t.x3 ^ ASCON_ROR(t.x3, 10) ^ ASCON_ROR(t.x3, 17);
        s->x4 = t.x4 ^ ASCON_ROR(t.x4,  7) ^ ASCON_ROR(t.x4, 41);
    }
}
#undef ASCON_ROR

ASCON_INLINE void ascon_sponge_absorb_block(ascon_state_t * s, uint64_t n) {
    s->x0 ^= n;
    ascon_sponge_permute(s, ASCON_SPONGE_ROUNDS);
}

ASCON_INLINE uint64_t ascon_sponge_squeeze_block(ascon_state_t * s) {
    uint64_t ret = s->x0;
    ascon_sponge_permute(s, ASCON_SPONGE_ROUNDS);

    return ret;
}

ASCON_INLINE uint64_t ascon_sponge_duplex_block(ascon_state_t * s, uint64_t n) {
    s->x0 ^= n;
    uint64_t ret = s->x0;
    ascon_sponge_permute(s, ASCON_SPONGE_ROUNDS);

    return ret;
}

ASCON_INLINE uint64_t ascon_sponge_inv_duplex_block(ascon_state_t * s, uint64_t n) {
    uint64_t ret = s->x0 ^ n;
    s->x0 = n;
    ascon_sponge_permute(s, ASCON_SPONGE_ROUNDS);

    return ret;
}

ASCON_INLINE void ascon_sponge_absorb_blocks(ascon_state_t * s, const uint64_t * in, size_t len) {
    for (size_t i = 0; i < len; i++, in++) {
        ascon_sponge_absorb_block(s, *in);
    }
}

ASCON_INLINE void ascon_sponge_squeeze_blocks(ascon_state_t * s, uint64_t * out, size_t len) {
    for (size_t i = 0; i < len; i++, out++) {
        *out = ascon_sponge_squeeze_block(s);
    }
}

ASCON_INLINE void ascon_sponge_duplex_blocks(ascon_state_t * s, const uint64_t * in, uint64_t * out, size_t len) {
    for (size_t i = 0; i < len; i++, in++, out++) {
        *out = ascon_sponge_duplex_block(s, *in);
    }
}

ASCON_INLINE void ascon_sponge_inv_duplex_blocks(ascon_state_t * s, const uint64_t * in, uint64_t * out, size_t len) {
    for (size_t i = 0; i < len; i++, in++, out++) {
        *out = ascon_sponge_inv_duplex_block(s, *in);
    }
}

#undef ASCON_INLINE

#endif // ASCON_SPONGE_DECL_ONLY

#endif // ASCON_SPONGE_TYPES_ONLY

#endif // ASCON_SPONGE_H
