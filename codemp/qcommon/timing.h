#pragma once

class timing_c
{
private:
	int64_t	start;
	int64_t	end;

public:
	timing_c(void)
	{
	}
	void Start()
	{
#if defined(_MSC_VER) && !defined(idx64)
		const int64_t *s = &start;
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
		int64_t	time;
#if defined(_MSC_VER) && !defined(idx64)
		const int64_t *e = &end;
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
