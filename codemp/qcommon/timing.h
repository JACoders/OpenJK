#ifndef _WIN32
#include <inttypes.h>
typedef int64_t __int64;
#endif

class timing_c
{
private:
	__int64	start;
	__int64	end;

	int		reset;
public:
	timing_c(void)
	{
	}
	void Start()
	{
		const __int64 *s = &start;
#ifdef _WIN32
		__asm
		{
			push eax
			push ebx
			push edx

			rdtsc
			mov ebx, s
			mov	[ebx], eax
			mov [ebx + 4], edx

			pop edx
			pop ebx
			pop eax
		}
#endif
	}
	int End()
	{
		const __int64 *e = &end;
		__int64	time;
#ifdef _WIN32
		__asm
		{
			push eax
			push ebx
			push edx

			rdtsc
			mov ebx, e
			mov	[ebx], eax
			mov [ebx + 4], edx

			pop edx
			pop ebx
			pop eax
		}
#endif 
		time = end - start;
		if (time < 0)
		{
			time = 0;
		}
		return((int)time);
	}
};
// end
