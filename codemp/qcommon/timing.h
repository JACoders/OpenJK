#pragma once

class timing_c
{
private:
	int64_t	start;
	int64_t	end;

	int		reset;
public:
	timing_c(void)
	{
	}
	void Start()
	{
		const int64_t *s = &start;
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
		const int64_t *e = &end;
		int64_t	time;
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
