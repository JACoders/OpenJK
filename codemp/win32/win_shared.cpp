//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "win_local.h"
#ifndef _XBOX
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#endif

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (bool baseTime)
{
	static int sys_timeBase = timeGetTime();
	int			sys_curtime;

	sys_curtime = timeGetTime();
	if(!baseTime)
	{
		sys_curtime -= sys_timeBase;
	}

	return sys_curtime;
}

/*
================
Sys_SnapVector
================
*/
void Sys_SnapVector( float *v )
{
	int i;
	float f;

	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	/*
	*v = myftol(*v);
	v++;
	*v = myftol(*v);
	v++;
	*v = myftol(*v);
	*/
}


/*
**
** Disable all optimizations temporarily so this code works correctly!
**
*/
#pragma optimize( "", off )

/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/
static inline void CPUID( int func, unsigned int regs[4] )
{
	unsigned regEAX, regEBX, regECX, regEDX;

	__asm mov eax, func
	__asm __emit 00fh
	__asm __emit 0a2h
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx

	regs[0] = regEAX;
	regs[1] = regEBX;
	regs[2] = regECX;
	regs[3] = regEDX;
}

static int IsPentium( void )
{
	__asm 
	{
		pushfd						// save eflags
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		set21				// bit 21 is not set, so jump to set_21
		and		eax, 0xffdfffff		// clear bit 21
		push	eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		good
		jmp		err					// cpuid not supported
set21:
		or		eax, 0x00200000		// set ID bit
		push	eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop		eax
		test	eax, 0x00200000		// if bit 21 is on
		jnz		good
		jmp		err
	}

err:
	return qfalse;
good:
	return qtrue;
}

static int Is3DNOW( void )
{
	unsigned regs[4];
	char pstring[16];
	char processorString[13];

	// get name of processor
	CPUID( 0, ( unsigned int * ) pstring );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

//  REMOVED because you can have 3DNow! on non-AMD systems
//	if ( strcmp( processorString, "AuthenticAMD" ) )
//		return qfalse;

	// check AMD-specific functions
	CPUID( 0x80000000, regs );
	if ( regs[0] < 0x80000000 )
		return qfalse;

	// bit 31 of EDX denotes 3DNOW! support
	CPUID( 0x80000001, regs );
	if ( regs[3] & ( 1 << 31 ) )
		return qtrue;

	return qfalse;
}

static int IsKNI( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 25 of EDX denotes KNI existence
	if ( regs[3] & ( 1 << 25 ) )
		return qtrue;

	return qfalse;
}

static int IsWIL( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 26 of EDX denotes WIL existence
	if ( regs[3] & ( 1 << 26 ) )
	{
		// Ok, CPU supports this instruction, but does the OS?
		//
		// Test a WIL instruction and make sure you don't get an exception...
		//
		__try
		{
			__asm
			{
				pushad;
			// 	xorpd xmm0,xmm0;  // Willamette New Instructions 
					__emit 0x0f
					__emit 0x56
					__emit 0xc9
				popad;
			}
		}// If OS creates an exception, it doesn't support PentiumIV Instructions
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
//			if(_exception_code()==STATUS_ILLEGAL_INSTRUCTION)	// forget it, any exception should count as fail for safety
				return qfalse; // Willamette New Instructions not supported
		}

		return qtrue;	// Williamette/P4 instructions available
	}

	return qfalse;

}


static int IsMMX( void )
{
	unsigned int regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		return qtrue;
	return qfalse;
}

int Sys_GetProcessorId( void )
{
#if defined _M_ALPHA
	return CPUID_AXP;
#elif !defined _M_IX86
	return CPUID_GENERIC;
#else

	// verify we're at least a Pentium or 486 w/ CPUID support
	if ( !IsPentium() )
		return CPUID_INTEL_UNSUPPORTED;

	// check for MMX
	if ( !IsMMX() )
	{
		// Pentium or PPro
		return CPUID_INTEL_PENTIUM;
	}

	// see if we're an AMD 3DNOW! processor
	if ( Is3DNOW() )
	{
		return CPUID_AMD_3DNOW;
	}

	// see if we're an Intel Katmai
	if ( IsKNI() )
	{
		// if we are, see if we're a Williamette as well...
		//
		if ( IsWIL() )
		{
			return CPUID_INTEL_WILLIAMETTE;
		}
		return CPUID_INTEL_KATMAI;
	}

	// by default we're functionally a vanilla Pentium/MMX or P2/MMX
	return CPUID_INTEL_MMX;

#endif
}

/*
**
** Re-enable optimizations back to what they were
**
*/
#pragma optimize( "", on )

//============================================

char *Sys_GetCurrentUser( void )
{
#ifdef _XBOX
	return NULL;
#else
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if ( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
#endif
}

char	*Sys_DefaultHomePath(void) {
	return NULL;
}

char *Sys_DefaultInstallPath(void)
{
	return Sys_Cwd();
}

int Sys_GetPhysicalMemory( void ) 
{ 
   MEMORYSTATUS	MemoryStatus;

   memset( &MemoryStatus, sizeof(MEMORYSTATUS), 0 );
   MemoryStatus.dwLength = sizeof(MEMORYSTATUS);

   GlobalMemoryStatus( &MemoryStatus );

   return( (int)(MemoryStatus.dwTotalPhys / (1024 * 1024)) + 1 );

} 


#ifndef _XBOX
int Sys_GetCPUSpeedOld()
{
	timeBeginPeriod(1);

#ifdef WIN32
	int iPriority;
	HANDLE hThread = GetCurrentThread();

	iPriority = GetThreadPriority(hThread);
	if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )
	{
		SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	}
#endif // WIN32

	DWORD clockStart = timeGetTime();
	DWORD clockEnd = clockStart + 100;

	unsigned long start;
	unsigned long end;

	__asm
	{
		rdtsc
		mov start, eax
	}

	while(timeGetTime() < clockEnd)
	{	// loop for 1 tenth of a second
	}
	__asm
	{
		rdtsc
		mov end, eax
	}


#ifdef WIN32
	// Reset priority
	if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )
	{
		SetThreadPriority(hThread, iPriority);
	}
#endif // WIN32
	
	timeEndPeriod(1);

	unsigned long	time;
	time = end - start;
	int coarse = time / 100000;
	int firsttry = floor((coarse + 25) / 50) * 50;
	if (abs(firsttry - coarse) < 10)
	{
		return firsttry;
	}
	else
	{
		return floor(floor((coarse + 17) / 33.3) * 33.3);
	}
}

int Sys_GetCPUSpeed()
{
	unsigned long raw_freq;		// Raw frequency of CPU in MHz
	unsigned long norm_freq;	// Normalized frequency of CPU in MHz.
	LARGE_INTEGER t0,t1;			// Variables for High-Resolution Performance Counter reads

	unsigned long freq  =0;			// Most current frequ. calculation
	unsigned long freq2 =0;			// 2nd most current frequ. calc.
	unsigned long freq3 =0;			// 3rd most current frequ. calc.
	
	unsigned long total;			// Sum of previous three frequency calculations
	int tries=0;			// Number of times a calculation has been made on this call to cpuspeed
	unsigned long  total_cycles=0, cycles;	// Clock cycles elapsed during test
	unsigned long  stamp0=0, stamp1=0;		// Time Stamp Variable for beginning and end  of test
	unsigned long  total_ticks=0, ticks;	// Microseconds elapsed during test	
	LARGE_INTEGER count_freq;		// High Resolution Performance Counter frequency

#define TOLERANCE		1		// Number of MHz to allow samplings to deviate from average of samplings.
#define ROUND_THRESHOLD		6

#ifdef WIN32
	int iPriority;
	HANDLE hThread = GetCurrentThread();
#endif // WIN32;

	if ( !QueryPerformanceFrequency ( &count_freq ) )
		return Sys_GetCPUSpeedOld();	//should never happen

	// On processors supporting the Read 
	//   Time Stamp opcode, compare elapsed
	//   time on the High-Resolution Counter
	//   with elapsed cycles on the Time 
	//   Stamp Register.
	
	do {	// This do loop runs up to 20 times or until the average of the previous 
	   		//   three calculated frequencies is within 1 MHz of each of the 
	   		//   individual calculated frequencies. This resampling increases the 
			//   accuracy of the results since outside factors could affect this calculation
			
		tries++;		// Increment number of times sampled on this call to cpuspeed
			
		freq3 = freq2;	// Shift frequencies back to make
		freq2 = freq;	//   room for new frequency measurement

    	QueryPerformanceCounter(&t0);// Get high-resolution performance counter time
			
		t1 = t0;		// Set Initial time

#ifdef WIN32
		iPriority = GetThreadPriority(hThread);
		if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )
		{
			SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
		}
#endif // WIN32

   		while ( (unsigned long)t1.LowPart - (unsigned long)t0.LowPart<50) {
   			// Loop until 50 ticks have passed since last read of hi-res counter. This accounts for overhead later.
			QueryPerformanceCounter(&t1);
			_asm {
				rdtsc;						// Read Time Stamp
				MOV stamp0, EAX
			}
		}
			
		t0 = t1;		// Reset Initial Time

   		while ((unsigned long)t1.LowPart-(unsigned long)t0.LowPart<2000 ) {
   		// Loop until enough ticks have passed since last read of hi-res counter. This allows for elapsed time for sampling.
   			QueryPerformanceCounter(&t1);
			__asm {
				rdtsc;						// Read Time Stamp
				MOV stamp1, EAX
			}
		}

#ifdef WIN32
		if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )
		{	// Reset priority
			SetThreadPriority(hThread, iPriority);
		}
#endif // WIN32

       	cycles = stamp1 - stamp0;	// Number of internal clock cycles is difference between two time stamp readings.

    	ticks = (unsigned long) t1.LowPart - (unsigned long) t0.LowPart;	
				// Number of external ticks is difference between two hi-res counter reads.
	

		// Note that some seemingly arbitrary mulitplies and
		//   divides are done below. This is to maintain a 
		//   high level of precision without truncating the 
		//   most significant data. According to what value 
		//   ITERATIIONS is set to, these multiplies and
		//   divides might need to be shifted for optimal
		//   precision.

		ticks = ticks * 100000;	// Convert ticks to hundred thousandths of a tick
			
		ticks = ticks / ( count_freq.LowPart/10 );		
						// Hundred Thousandths of a Ticks / ( 10 ticks/second ) = microseconds (us)

		total_ticks += ticks;
		total_cycles += cycles;

		if ( ticks%count_freq.LowPart > count_freq.LowPart/2 )
			ticks++;			// Round up if necessary
			
		if (!ticks){
			ticks++;			// prevent DIV by ZERO
		}

		freq = cycles/ticks;	// Cycles / us  = MHz
        										
     	if ( cycles%ticks > ticks/2 )
       		freq++;				// Round up if necessary
          	
		total = ( freq + freq2 + freq3 ); // Total last three frequency calculations

	} while ( (tries < 3 ) || 
	          (tries < 20)&&
	          ((abs(3 * freq -total) > 3*TOLERANCE )||
	           (abs(3 * freq2-total) > 3*TOLERANCE )||
	           (abs(3 * freq3-total) > 3*TOLERANCE )));	
				// Compare last three calculations to average of last three calculations.		

	if (!total_ticks){
		total_ticks++;			// prevent DIV by ZERO
	}

	// Try one more significant digit.
	freq3 = ( total_cycles * 10 ) / total_ticks;
	freq2 = ( total_cycles * 100 ) / total_ticks;


	if ( freq2 - (freq3 * 10) >= ROUND_THRESHOLD )
		freq3++;

	raw_freq = total_cycles / total_ticks;
	norm_freq = raw_freq;

	freq = raw_freq * 10;
	if( (freq3 - freq) >= ROUND_THRESHOLD )
		norm_freq++;

	return norm_freq;
}
#endif

