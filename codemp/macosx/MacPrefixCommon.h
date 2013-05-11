#pragma once

//#define MAC_PORT		1
#define _JK2			1

// gcc-only
#define MACOS_X			1

// CW-only
//#define __MACOS__		1

#define strupr		Q_strupr
#define strnicmp	Q_stricmpn

// JKJA uses a version of powf with different parameters than the standard one.
// Additionally, the standard version of powf requires you to link against libmx
// which requires 10.3+. We'll do some preprocessor magic to work around this
// mess. LBO 8/31/04

// First, eliminate the powf used in the system headers
#define powf _powf
#include <math.h>
#undef powf

// Now re-define powf so that it uses the double-precision equivalent
#define powf pow

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))
