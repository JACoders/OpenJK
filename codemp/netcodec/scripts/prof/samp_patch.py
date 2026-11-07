# Sampling profiler (Windows, clang), used on the copies of ab_clang.sh: patches <copy>/codemp/netcodec/bench/bench_codec.cpp
# with a thread on core 24 that suspends the benchmark thread every 50 us and records RIP, around a warm encode loop
# (each message coded twice, the second sampled); the patched program writes samples.txt.
# Usage: python -I samp_patch.py <copy>
import sys, pathlib, re
b = pathlib.Path(sys.argv[1]) / "codemp" / "netcodec" / "bench" / "bench_codec.cpp"
u = b.read_bytes().decode()
head = '#include "message_compare.h"\n'
assert u.count(head) == 1
u = u.replace(head, head + r"""
#include <windows.h>
#include <atomic>
#include <thread>
namespace samp {
std::atomic<int> phase{ 0 };
std::atomic<bool> stop{ false };
std::vector<uint64_t> samples;
void Run(HANDLE th) {
	SetThreadAffinityMask(GetCurrentThread(), 0x1000000);
	LARGE_INTEGER f, t;
	QueryPerformanceFrequency(&f);
	const LONGLONG step = f.QuadPart / 20000;
	QueryPerformanceCounter(&t);
	LONGLONG next = t.QuadPart;
	while (!stop.load(std::memory_order_relaxed)) {
		do QueryPerformanceCounter(&t); while (t.QuadPart < next);
		next += step;
		if (phase.load(std::memory_order_relaxed) == 0) continue;
		SuspendThread(th);
		CONTEXT c{};
		c.ContextFlags = CONTEXT_CONTROL;
		if (phase.load(std::memory_order_relaxed) != 0 && GetThreadContext(th, &c)) samples.push_back(c.Rip);
		ResumeThread(th);
	}
}
}
""")
m = re.search(r"\t\t\t\tif \(enc\.Encode\(it\.cm->msg, it\.base, ([^,]+), r->serverHistory, 1 << 20, out, rec\)\) acc \+= out\.size\(\);\n", u)
assert m
bl = m.group(1)
a3 = "\tconst double tEncLegacy = Best(passes, [&] {"
assert u.count(a3) == 1
u = u.replace(a3, r"""	{
		SetThreadAffinityMask(GetCurrentThread(), 0x100000);
		HANDLE mainTh = nullptr;
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &mainTh, 0, FALSE, DUPLICATE_SAME_ACCESS);
		samp::samples.reserve(1 << 22);
		std::thread sampler(samp::Run, mainTh);
		for (int rep = 0; rep < 40; rep++)
			for (auto &r : runs)
				for (const Item &it : r->items) {
					HeaderRecord rec;
					if (enc.Encode(it.cm->msg, it.base, BL, r->serverHistory, 1 << 20, out, rec)) sink = sink + out.size();
					samp::phase = 1;
					if (enc.Encode(it.cm->msg, it.base, BL, r->serverHistory, 1 << 20, out, rec)) sink = sink + out.size();
					samp::phase = 0;
				}
		samp::stop = true;
		sampler.join();
		const auto base = reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr));
		FILE *sf = fopen("samples.txt", "w");
		for (const uint64_t rip : samp::samples) fprintf(sf, "1 0x%llx\n", static_cast<unsigned long long>(rip - base + 0x140000000ULL));
		fclose(sf);
		printf("samples %zu\n", samp::samples.size());
	}
""".replace("BL", bl) + a3)
b.write_bytes(u.encode())
print("ok", bl)
