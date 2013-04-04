// Filename:	mc_compress2.h
//

#ifndef MC_COMPRESS2_H
#define MC_COMPRESS2_H

#ifdef __cplusplus
extern "C"
{
#endif

#pragma optimize( "p", on )	// improve floating-point consistancy (makes release do bone-pooling as good as debug)



void MC_Compress2(const float mat[3][4],unsigned char * comp);
void MC_UnCompress2(float mat[3][4],const unsigned char * comp);

void QuatSlerpCompTo3x4(	float fLerp01,	
							const unsigned char *pComp0,
							const unsigned char *pComp1,
							float fDestMat[3][4]
							);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef MC_COMPRESS2_H

///////////// eof /////////////

