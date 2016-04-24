/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * An implementation of HMAC-MD5 (originally for OpenJK) is also
 * provided, and also released into the public domain.
 */

#pragma once

#include "q_shared.h"

typedef struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
} MD5_CTX;

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len);
void MD5Final(struct MD5Context *ctx, unsigned char *digest);

const size_t MD5_BLOCK_SIZE = 64;
const size_t MD5_DIGEST_SIZE = 16;
typedef struct {
	struct MD5Context md5Context;
	unsigned char iKeyPad[MD5_BLOCK_SIZE];
	unsigned char oKeyPad[MD5_BLOCK_SIZE];
} hmacMD5Context_t;

// Initialize a new HMAC-MD5 construct using the specified secret key.
void HMAC_MD5_Init(hmacMD5Context_t *ctx, unsigned char const *key, unsigned int keylen);

// Update the HMAC message with len number of bytes from the given buffer.
void HMAC_MD5_Update(hmacMD5Context_t *ctx, unsigned char const *buf, unsigned int len);

// Finalize the HMAC calculation and fill the given buffer with the digest bytes.
// 'digest' must point to a buffer that can hold MD5_DIGEST_SIZE bytes!
void HMAC_MD5_Final(hmacMD5Context_t *ctx, unsigned char *digest);

// Reset the context to begin working on a new message, using the same secret key as previously initialised.
void HMAC_MD5_Reset(hmacMD5Context_t *ctx);