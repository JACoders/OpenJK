#pragma once

#include "../rd-common/tr_public.h"
#include "../rd-common/tr_font.h"

extern refimport_t ri;

/*
================================================================================
Noise Generation
================================================================================
*/
// Initialize the noise generator.
void R_NoiseInit( void );

// Get random 4-component vector.
float R_NoiseGet4f( float x, float y, float z, float t );

// Get the noise time.
float GetNoiseTime( int t );
