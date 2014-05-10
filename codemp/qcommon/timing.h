#pragma once

#ifdef _WIN32
#include <intrin.h>
#endif

class timing_c
{
private:
	uint64_t	start;
	uint64_t	end;

public:
	timing_c(void)
	{
	}

	void Start()
	{
#ifdef _WIN32
		start = __rdtsc();
#else
		start = 0;
#endif
	}

	int End()
	{
		int64_t	time;

#ifdef _WIN32
		end = __rdtsc();
#else
		end = 0;
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
