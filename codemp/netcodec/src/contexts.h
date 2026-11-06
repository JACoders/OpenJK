// netcodec internals: the contexts of the format. Every symbol is coded with the table of one context; the contexts
// are numbered here from the schema, so that the coder, the model and the trainer share one definition.
//
// Layout, in this order:
//  - per entity field (codec order) and per player-state field name: one context per kind (fctx), unused kinds have
//    an empty alphabet;
//  - index of the next changed field given the previous one: player, pilot and vehicle tables, then entities per class;
//  - entity operation given the previous one; entity type changed and new type, per class of the base;
//  - message operation given the previous one; the scalar contexts of the message (enum Scalar).
#pragma once

#include "schema.h"

namespace netcodec::detail::ctx {

inline constexpr uint32_t kKinds = ToU32(fctx::kCount);   // contexts per field
inline constexpr uint32_t kEntityClassCount = kEntityClasses;
inline constexpr uint32_t kEntityGapAlphabet = kNumEntityFields + 1;   // a field index, or the end of the list
inline constexpr uint32_t kOpAlphabet = 8;

// the player-state tables, in the order of the pilot flag and the vehicle state
inline constexpr std::array<std::span<const FieldDesc>, 3> kPlayerTables{ kPlayerFields, kPilotFields, kVehicleFields };

enum class Scalar : uint8_t {
	kMDelta, kMAck, kMCmdSeq, kMRelSeq, kMNumCs, kMCsGap, kMClientNum, kMDlBlock, kMDlSize, kMDlLen, kDlByte, kHTime,
	kHFlags, kHArea, kPsPilot, kStrCmd, kStrSetgame, kStrCs, kEntGap, kScalarCount
};
// alphabets of the scalars: 36 for a class of a 64-bit value (zigzag differences), 256 for bytes
inline constexpr std::array<uint16_t, static_cast<size_t>(Scalar::kScalarCount)> kScalarAlphabet{ 256, 36, 36, 36, 16, 16, 36, 20, 36, 20, 256, 36,
                                                                     256, 256, 2, 256, 256, 256, 42 };

inline constexpr uint32_t kEntityField = 0;
inline constexpr uint32_t kPlayerField = kEntityField + (kNumEntityFields * kKinds);
inline constexpr uint32_t kPlayerGap = kPlayerField + (kPlayerNames * kKinds);
inline constexpr std::array<uint32_t, 3> kPlayerGapStart{
	kPlayerGap, kPlayerGap + ToU32(kPlayerFields.size()) + 1,
	kPlayerGap + ToU32(kPlayerFields.size()) + 1 + ToU32(kPilotFields.size()) + 1 };
inline constexpr uint32_t kEntityGap = kPlayerGapStart[2] + ToU32(kVehicleFields.size()) + 1;
inline constexpr uint32_t kEntityOp = kEntityGap + (kEntityClassCount * kEntityGapAlphabet);
inline constexpr uint32_t kTypeChanged = kEntityOp + 4;
inline constexpr uint32_t kTypeValue = kTypeChanged + kEntityClassCount;
inline constexpr uint32_t kOp = kTypeValue + kEntityClassCount;
inline constexpr uint32_t kScalar = kOp + kOpAlphabet;
inline constexpr uint32_t kCount = kScalar + static_cast<uint32_t>(Scalar::kScalarCount);

// The contexts of one field: row[kind] is the context of that kind.
struct Row {
	uint32_t first = 0;
	[[nodiscard]] constexpr uint32_t operator[](size_t kind) const noexcept { return first + ToU32(kind); }
};

[[nodiscard]] constexpr Row EntityField(uint32_t row) noexcept { return Row{ kEntityField + (row * kKinds) }; }
[[nodiscard]] constexpr Row PlayerField(uint32_t row) noexcept { return Row{ kPlayerField + (row * kKinds) }; }
// gap contexts: g + (previous changed field + 1)
[[nodiscard]] constexpr uint32_t PlayerGap(uint32_t table) noexcept { return kPlayerGapStart[table]; }
[[nodiscard]] constexpr uint32_t EntityGap(uint32_t cls) noexcept { return kEntityGap + (cls * kEntityGapAlphabet); }
[[nodiscard]] constexpr uint32_t Of(Scalar s) noexcept { return kScalar + static_cast<uint32_t>(s); }

// alphabet of a field kind: absolute integers up to 8 bits directly, wider ones and differences by class (bit
// length, 0 to the width, plus one for a sign); float kind, integral float classes (16), angle index differences (24),
// 32-bit differences and predictions (36), second-order angles (40), boolean flip
[[nodiscard]] constexpr uint32_t KindAlphabet(size_t kind, int bits, CType ctype) noexcept {
	const auto ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const bool isInt = bits != 0;
	switch (kind) {
	case fctx::kA:
		if (!isInt) return 0;
		return ab <= 8 ? 1U << ab : ab + 2;
	case fctx::kD: return isInt ? ab + 2 : 0;
	case fctx::kT: return isInt ? 36 : 0;
	case fctx::kK: return isInt ? 0 : 4;
	case fctx::kIA:
	case fctx::kID: return isInt ? 0 : 16;
	case fctx::kAD: return isInt ? 0 : 24;
	case fctx::kFD:
	case fctx::kFP: return isInt ? 0 : 36;
	case fctx::kA2: return isInt ? 0 : 40;
	case fctx::kB: return ctype == CType::kBool ? 2 : 0;
	default: return 0;
	}
}

// the descriptor of player-state field name `row` (the same in every table that has it)
[[nodiscard]] constexpr const FieldDesc *PlayerName(uint32_t row) noexcept {
	for (const std::span<const FieldDesc> t : kPlayerTables)
		for (const FieldDesc &f : t)
			if (f.ctx == row) return &f;
	return nullptr;
}

// number of symbols of a context (0: never coded)
[[nodiscard]] constexpr uint32_t Alphabet(uint32_t c) noexcept {
	if (c < kPlayerField) {
		const FieldDesc &f = kEntityFields[(c - kEntityField) / kKinds];
		return KindAlphabet((c - kEntityField) % kKinds, f.bits, f.ctype);
	}
	if (c < kPlayerGap) {
		const FieldDesc *f = PlayerName((c - kPlayerField) / kKinds);
		return f != nullptr ? KindAlphabet((c - kPlayerField) % kKinds, f->bits, f->ctype) : 0;
	}
	for (uint32_t t = 0; t < 3; t++)
		if (c < (t < 2 ? kPlayerGapStart[t + 1] : kEntityGap)) return ToU32(kPlayerTables[t].size()) + 1;
	if (c < kEntityOp) return kEntityGapAlphabet;
	if (c < kTypeChanged) return 4;
	if (c < kTypeValue) return 2;
	if (c < kOp) return 256;
	if (c < kScalar) return kOpAlphabet;
	return c < kCount ? kScalarAlphabet[c - kScalar] : 0;
}

} // namespace netcodec::detail::ctx
