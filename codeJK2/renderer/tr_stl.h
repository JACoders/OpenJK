// Filename: tr_stl.h
//
//  I had to make this new file, because if I put the STL "map" include inside tr_local.h then one of the other header
//	files got compile errors because of using "map" in the function protos as a GLEnum, this way seemed simpler...

#ifndef TR_STL_H


// REM this out if you want to compile without using STL (but slower of course)
//
#define USE_STL_FOR_SHADER_LOOKUPS




#ifdef USE_STL_FOR_SHADER_LOOKUPS

	void ShaderEntryPtrs_Clear(void);
	int ShaderEntryPtrs_Size(void);
	const char  *ShaderEntryPtrs_Lookup(const char *psShaderName);
	void ShaderEntryPtrs_Insert(const char  *token, const char  *p);

#else

	#define ShaderEntryPtrs_Clear()	

#endif	// #ifdef USE_STL_FOR_SHADER_LOOKUPS

#endif	// #ifndef TR_STL_H


