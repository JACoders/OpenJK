// netcodec internals: the networked fields of the JKA states (generated schema) and access to them in state images.
//
// A state image is the byte image of an engine structure (entityState_t, playerState_t). A field is a 32-bit word at
// the offset of the schema; its value is read with the codec width of the schema: the low bits of an unsigned
// field, a sign-extended signed field, the raw pattern of a float or of a 32-bit field.
#pragma once

#include "netcodec/codec.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

namespace netcodec::detail {

enum class CType : uint8_t { kInt, kFloat, kBool, kEnum };

// prediction a field is coded with: none, time against the snapshot time, position from velocity, angle to second order
enum class PredMode : uint8_t { kNone, kTime, kPos, kAngle };

struct FieldDesc {
	uint16_t offset = 0;              // in the state image
	int8_t bits = 0;                  // codec width: 0 float, < 0 signed
	CType ctype = CType::kInt;
	bool absolute = false;            // absolute mode, else delta against the base
	PredMode pmode = PredMode::kNone;
	uint16_t ctx = 0;                 // row of the field's contexts (kEntityFieldCtx or kPlayerFieldCtx)
	uint16_t relType = 0;             // position prediction: offsets of trType, time and velocity, and the state kind
	uint16_t relTime = 0;             // (0 entity, 1 player state); 0xFFFF when the field has no position prediction
	uint16_t relVel = 0;
	uint16_t relKind = 0;
};

// The contexts of one field, by kind: absolute and delta integers; float kind, integral float (absolute, delta),
// angle index, other float; time, position and angle predictions; boolean flip.
namespace fctx {
constexpr size_t kA = 0, kD = 1, kK = 2, kIA = 3, kID = 4, kAD = 5, kFD = 6, kT = 7, kFP = 8, kA2 = 9, kB = 10, kCount = 11;
}

#include "generated/jka_schema.inc"

static_assert(kEntitySize == kEntityBytes && kPlayerSize == kPlayerBytes, "schema and codec.h disagree");
inline constexpr uint32_t kNumEntityFields = ToU32(kEntityFields.size());

using ImageView = std::span<const uint8_t>;
using ImageRef = std::span<uint8_t>;

[[nodiscard]] inline uint32_t Load(ImageView img, uint32_t offset) noexcept {
	uint32_t v = 0;
	std::memcpy(&v, img.subspan(offset, 4).data(), 4);   // little-endian hosts (x86, x64)
	return v;
}

inline void StoreWord(ImageRef img, uint32_t offset, uint32_t v) noexcept {
	std::memcpy(img.subspan(offset, 4).data(), &v, 4);
}

// value of a word for a codec width (see the file comment)
[[nodiscard]] constexpr int64_t ValueBits(uint32_t w, int bits) noexcept {
	if (bits == 0 || bits == 32) return w;
	if (bits > 0) return w & ((1U << static_cast<uint32_t>(bits)) - 1);
	const auto ab = static_cast<uint32_t>(-bits);
	if (ab == 32) return static_cast<int32_t>(w);
	const uint32_t x = w & ((1U << ab) - 1);
	return (x >> (ab - 1)) != 0 ? static_cast<int64_t>(x) - static_cast<int64_t>(uint64_t{ 1 } << ab) : static_cast<int64_t>(x);
}

[[nodiscard]] inline int64_t Value(ImageView img, const FieldDesc &f) noexcept { return ValueBits(Load(img, f.offset), f.bits); }
inline void Store(ImageRef img, const FieldDesc &f, int64_t v) noexcept { StoreWord(img, f.offset, static_cast<uint32_t>(v)); }

// the field differs between two images: raw words first, the codec value only when they differ
[[nodiscard]] inline bool FieldDiffers(ImageView a, ImageView b, const FieldDesc &f) noexcept {
	const uint32_t wa = Load(a, f.offset), wb = Load(b, f.offset);
	return wa != wb && ValueBits(wa, f.bits) != ValueBits(wb, f.bits);
}

[[nodiscard]] inline const PlayerState &ZeroPlayer() noexcept {
	static const PlayerState zero{};
	return zero;
}

[[nodiscard]] inline const EntityState &ZeroEntity() noexcept {
	static const EntityState zero{};
	return zero;
}

} // namespace netcodec::detail
