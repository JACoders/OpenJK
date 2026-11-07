// Included before every file of the 1.01 build: the original q_shared.h declares its own powf(float, int), which
// conflicts with the C library's; math.h first, then the game's function under another name.
#include <math.h>
#define powf jka_powf
