// Filename:-	matcomp.h
//

#ifndef MATCOMP_H
#define MATCOMP_H

//#ifdef __cplusplus
//extern "C"
//{
//#endif

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

#define MC_COMP_BYTES 24	//(((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*9)+7)/8)

void MC_Compress(const float mat[3][4],unsigned char * comp);
void MC_UnCompress(float mat[3][4],const unsigned char * comp);
void MC_UnCompressQuat(float mat[3][4],const unsigned char * comp);

/*
typedef float vec_t;
typedef vec_t vec3_t[3];

typedef struct
{
	vec3_t	vec;
} cqpoint_t;

extern cqpoint_t* quaternionIndex;

extern FILE* out;
*/

//#ifdef __cplusplus
//}
//#endif

#endif	// #ifndef MATCOMP_H


///////////// eof ///////////