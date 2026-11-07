// Compiled as C++11 (the engine's standard): engine.h must build there. Not linked into anything. MSVC has no C++11
// mode (CXX_STANDARD 11 gives its oldest, C++14) and reports __cplusplus as 199711L, so the check is GCC and Clang's.
#include "netcodec/engine.h"

#if !defined(_MSC_VER) && __cplusplus != 201103L
#error "this file checks engine.h in C++11"
#endif

size_t netcodec_engine_cxx11_check(netcodec::engine::ServerCodec &enc, const netcodec::engine::Frame *frames, uint8_t *out) {
	netcodec::engine::ServerMessage msg = netcodec::engine::ServerMessage();
	msg.seq = 1;
	msg.firstCommand = 1;
	const netcodec::engine::EntityRing ring = { nullptr, 0 };
	const netcodec::engine::CommandRing commands = { nullptr, 1024, netcodec::engine::kMinCommands, 0 };
	const netcodec::engine::Baselines baselines = { nullptr, 600 };
	uint32_t delta = 0;
	netcodec::engine::Record record;
	return enc.Encode(msg, frames, ring, 0, commands, baselines, out, 49152, delta, record);
}
