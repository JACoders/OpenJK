/*
===========================================================================
Copyright (C) 2013 - 2016, OpenJK contributors

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
#include "qcommon/md5.h"

//#define DEBUG_SV_CHALLENGE // Enable for Com_DPrintf debugging output

static const size_t SECRET_KEY_LENGTH = MD5_DIGEST_SIZE; // Key length equal to digest length is adequate

static qboolean challengerInitialized = qfalse;
static hmacMD5Context_t challenger;

#ifdef DEBUG_SV_CHALLENGE
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
#endif

/*
====================
SV_ChallengeInit

Initialize the HMAC context for generating challenges.
====================
*/
void SV_ChallengeInit()
{
	if (challengerInitialized) {
		SV_ChallengeShutdown();
	}

	// Generate a secret key from the OS RNG
	byte secretKey[SECRET_KEY_LENGTH];
	if (!Sys_RandomBytes(secretKey, sizeof(secretKey))) {
		Com_Error(ERR_FATAL, "SV_ChallengeInit: Sys_RandomBytes failed");
	}

#ifdef DEBUG_SV_CHALLENGE
	Com_DPrintf("Initialize challenger: %s\n", BufferToHexString(secretKey, sizeof(secretKey)));
#endif

	HMAC_MD5_Init(&challenger, secretKey, sizeof(secretKey));

	challengerInitialized = qtrue;
}

/*
====================
SV_ChallengeShutdown

Clear the HMAC context used to generate challenges.
====================
*/
void SV_ChallengeShutdown()
{
	if (challengerInitialized) {
		memset(&challenger, 0, sizeof(challenger));
		challengerInitialized = qfalse;
	}
}

/*
====================
SV_CreateChallenge (internal)

Create a challenge for the given client address and timestamp.
====================
*/
static int SV_CreateChallenge(int timestamp, netadr_t from)
{
	const char *clientParams = NET_AdrToString(from);
	size_t clientParamsLen = strlen(clientParams);

	// Create an unforgeable, temporal challenge for this client using HMAC(secretKey, clientParams + timestamp)
	byte digest[MD5_DIGEST_SIZE];
	HMAC_MD5_Update(&challenger, (byte*)clientParams, clientParamsLen);
	HMAC_MD5_Update(&challenger, (byte*)&timestamp, sizeof(timestamp));
	HMAC_MD5_Final(&challenger, digest);
	HMAC_MD5_Reset(&challenger);

	// Use first 4 bytes of the HMAC digest as an int (client only deals with numeric challenges)
	// The most-significant bit stores whether the timestamp is odd or even. This lets later verification code handle the
	// case where the engine timestamp has incremented between the time this challenge is sent and the client replies.
	int challenge;
	memcpy(&challenge, digest, sizeof(challenge));
	challenge &= 0x7FFFFFFF;
	challenge |= (unsigned int)(timestamp & 0x1) << 31;

#ifdef DEBUG_SV_CHALLENGE
	if ( com_developer->integer ) {
		Com_Printf( "Generated challenge %d (timestamp = %d) for %s\n", challenge, timestamp, NET_AdrToString( from ) );
	}
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
	if (!challengerInitialized) {
		Com_Error(ERR_FATAL, "SV_CreateChallenge: The challenge subsystem has not been initialized");
	}

	// The current time gets 14 bits chopped off to create a challenge timestamp that changes every 16.384 seconds
	// This allows clients at least ~16 seconds from now to reply to the challenge
	int currentTimestamp = svs.time >> 14;
	return SV_CreateChallenge(currentTimestamp, from);
}

/*
====================
SV_VerifyChallenge

Verify a challenge received by the client matches the expected challenge.
====================
*/
qboolean SV_VerifyChallenge(int receivedChallenge, netadr_t from)
{
	if (!challengerInitialized) {
		Com_Error(ERR_FATAL, "SV_VerifyChallenge: The challenge subsystem has not been initialized");
	}

	int currentTimestamp = svs.time >> 14;
	int currentPeriod = currentTimestamp & 0x1;

	// Use the current timestamp for verification if the current period matches the client challenge's period.
	// Otherwise, use the previous timestamp in case the current timestamp incremented in the time between the
	// client being sent a challenge and the client's reply that's being verified now.
	int challengePeriod = ((unsigned int)receivedChallenge >> 31) & 0x1;
	int challengeTimestamp = currentTimestamp - (currentPeriod ^ challengePeriod);

#ifdef DEBUG_SV_CHALLENGE
	if ( com_developer->integer ) {
		Com_Printf( "Verifying challenge %d (timestamp = %d) for %s\n", receivedChallenge, challengeTimestamp, NET_AdrToString( from ) );
	}
#endif

	int expectedChallenge = SV_CreateChallenge(challengeTimestamp, from);
	return (qboolean)(receivedChallenge == expectedChallenge);
}
