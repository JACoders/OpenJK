/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// sv_challenge.cpp -- stateless challenge creation and verification functions

#include "server.h"
#include <mbedtls/error.h>
#include <mbedtls/md.h>

//#define DEBUG_SV_CHALLENGE

// TODO: DOCS
static const mbedtls_md_type_t HMAC_ALGORITHM = MBEDTLS_MD_SHA1;
static const size_t HMAC_DIGEST_LENGTH = 20; // HMAC-SHA1 digest is 20 bytes (160 bits)
static const int SECRET_KEY_LENGTH = 20; // Key length equal to digest length is adequate
static const int CHALLENGER_UPDATE_INTERVAL = 30000; // msec

static qboolean challengerInitialized = qfalse;
static mbedtls_md_context_t challengers[2];
static int primaryChallenger;
static int lastUpdateTime;

/*
====================
SV_mbedTLS_ErrString

Return a string representation of the given mbed TLS error code.
====================
*/
static const char *SV_mbedTLS_ErrString(int errnum)
{
	static char errorString[1024];
	mbedtls_strerror(errnum, errorString, sizeof(errorString));
	return errorString;
}

/*
====================
BufferToHexString

Format a byte buffer as a lower-case hex string.
====================
*/
static const char *BufferToHexString(byte *buffer, size_t bufferLen)
{
	static char hexString[1023];
	static const size_t maxBufferLen = (sizeof(hexString) - 1) / 2;
	static const char *hex = "0123456789abcdef";
	if (bufferLen > maxBufferLen) {
		bufferLen = maxBufferLen;
	}
	for (size_t i = 0; i < bufferLen; i++) {
		hexString[i * 2] = hex[buffer[i] / 16];
		hexString[i * 2 + 1] = hex[buffer[i] % 16];
	}
	hexString[bufferLen * 2] = '\0';
	return hexString;
}

/*
====================
SV_InitChallengers

Initialize and allocate the HMAC contexts for generating challenges.
====================
*/
void SV_InitChallengers()
{
	if (challengerInitialized) {
		SV_ShutdownChallengers();
	}

	const mbedtls_md_info_t *info = mbedtls_md_info_from_type(HMAC_ALGORITHM);
	if (!info) {
		Com_Error(ERR_FATAL, "SV_InitChallengers: The given digest type is unavailable");
	}

	int errorNum;
	byte secretKey[SECRET_KEY_LENGTH];
	for (int i = 0; i < 2; i++) {
		mbedtls_md_context_t *challenger = &challengers[i];
		mbedtls_md_init(challenger);

		if ((errorNum = mbedtls_md_setup(challenger, info, 1)) != 0) {
			Com_Error(ERR_FATAL, "SV_InitChallengers: mbedtls_md_setup failed - %s", SV_mbedTLS_ErrString(errorNum));
		}

		// Generate a secret key from the OS RNG
		if (!Sys_RandomBytes(secretKey, sizeof(secretKey))) {
			Com_Error(ERR_FATAL, "SV_InitChallengers: Sys_RandomBytes failed");
		}
#ifdef DEBUG_SV_CHALLENGE
		Com_DPrintf("Initialize challenger %d: %s\n", i, BufferToHexString(secretKey, sizeof(secretKey)));
#endif

		if ((errorNum = mbedtls_md_hmac_starts(challenger, secretKey, sizeof(secretKey))) != 0) {
			Com_Error(ERR_FATAL, "SV_InitChallengers: mbedtls_md_hmac_starts failed - %s", SV_mbedTLS_ErrString(errorNum));
		}
	}

	primaryChallenger = 0;
	lastUpdateTime = svs.time;
	challengerInitialized = qtrue;
}

/*
====================
SV_ShutdownChallengers

Free the resources allocated to generate challenges.
====================
*/
void SV_ShutdownChallengers()
{
	if (challengerInitialized) {
		mbedtls_md_free(&challengers[0]);
		mbedtls_md_free(&challengers[1]);
		challengerInitialized = qfalse;
	}
}

/*
====================
SV_UpdateChallengers

Roll over to a new primary challenge key if the update interval has passed.
====================
*/
void SV_UpdateChallengers()
{
	if (!challengerInitialized)
		return;

	// Generate a new key every so often to prevent challenge reuse attacks
	if (svs.time >= lastUpdateTime + CHALLENGER_UPDATE_INTERVAL) {
		byte secretKey[SECRET_KEY_LENGTH];
		primaryChallenger ^= 1; // Flip over to a different challenger

		// Generate a secret key from the OS RNG
		if (!Sys_RandomBytes(secretKey, sizeof(secretKey))) {
			Com_Error(ERR_FATAL, "SV_UpdateChallengers: Sys_RandomBytes failed");
		}
#ifdef DEBUG_SV_CHALLENGE
		Com_DPrintf("Switching to challenger %d: %s\n", primaryChallenger, BufferToHexString(secretKey, sizeof(secretKey)));
#endif

		// Initialize the challenger with the new key
		int errorNum;
		if ((errorNum = mbedtls_md_hmac_starts(&challengers[primaryChallenger], secretKey, sizeof(secretKey))) != 0) {
			Com_Error(ERR_FATAL, "SV_UpdateChallengers: mbedtls_md_hmac_starts failed - %s", SV_mbedTLS_ErrString(errorNum));
		}

		lastUpdateTime = svs.time;
	}
}

/*
====================
SV_CreateChallenge (internal)

Create a challenge for the given client address using a specific challenger.
====================
*/
static int SV_CreateChallenge(int challengerIndex, netadr_t from)
{
	// Create an unforgeable, temporal challenge for this client using HMAC(secretKey, from)
	int errorNum;
	byte digest[HMAC_DIGEST_LENGTH];
	mbedtls_md_context_t *challenger = &challengers[challengerIndex];
	if ((errorNum = mbedtls_md_hmac_update(challenger, (byte*)&from, sizeof(from))) != 0 ||
		(errorNum = mbedtls_md_hmac_finish(challenger, digest)) != 0 ||
		(errorNum = mbedtls_md_hmac_reset(challenger)) != 0) {
		Com_Error(ERR_FATAL, "SV_UpdateChallengers: failed - %s", SV_mbedTLS_ErrString(errorNum));
	}

	// Use first 4 bytes of the HMAC digest as an int (client only deals with numeric challenges)
	// The most-significant bit stores the challenger (0 or 1) used to create this challenge for later verification
	int challenge = *(int *)digest & 0x7FFFFFFF;
	challenge |= (challengerIndex & 0x1) << 31;

#ifdef DEBUG_SV_CHALLENGE
	Com_DPrintf("Generated challenge %d (#%d) for %s\n", challenge, challengerIndex, NET_AdrToString(from));
#endif

	return challenge;
}

/*
====================
SV_CreateChallenge

Create an unforgeable, temporal challenge for the given client address.
====================
*/
int SV_CreateChallenge(netadr_t from)
{
	return SV_CreateChallenge(primaryChallenger, from);
}

/*
====================
SV_VerifyChallenge

Verify a challenge received by the client matches the expected challenge.
====================
*/
qboolean SV_VerifyChallenge(int receivedChallenge, netadr_t from)
{
	// Retrieve the challenger used from the most-significant bit, then re-create the expected challenge
	int challengerIndex = (receivedChallenge >> 31) & 0x1; // 0 or 1

#ifdef DEBUG_SV_CHALLENGE
	Com_DPrintf("Verifying challenge %d (#%d) for %s\n", receivedChallenge, challengerIndex, NET_AdrToString(from));
#endif

	int expectedChallenge = SV_CreateChallenge(challengerIndex, from);
	return (qboolean)(receivedChallenge == expectedChallenge);
}
