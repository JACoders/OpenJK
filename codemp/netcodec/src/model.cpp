#include "model.h"

#include "schema.h"

namespace netcodec::detail {

namespace {
#include "generated/jka_tables.inc"
}

Model::Model() {
	const uint32_t total = kTableStart[kTableCount];
	cum_.resize(total + kTableCount);
	enc_.resize(total);
	coarse_.resize(kTableCount);
	tables_.reserve(kTableCount);
	for (uint32_t t = 0; t < kTableCount; t++) {
		const uint32_t first = kTableStart[t];
		const uint32_t k = kTableStart[t + 1] - first;
		const std::span<uint16_t> cum(&cum_[first + t], k + 1);
		cum[0] = 0;
		for (uint32_t s = 0; s < k; s++) {
			cum[s + 1] = static_cast<uint16_t>(cum[s] + kFreq[first + s]);
			enc_[first + s] = rans::MakeEncSymbol(cum[s], kFreq[first + s]);
		}
		rans::BuildCoarse(cum, coarse_[t]);
		tables_.push_back(rans::Table{ .cum = cum, .enc = std::span<const rans::EncSymbol>(&enc_[first], k), .coarse = coarse_[t] });
	}
	for (const FieldDesc &f : kEntityFields) {
		if (f.relType == 0xFFFF) continue;
		for (const FieldDesc &g : kEntityFields)
			if (g.offset == f.relType) trTypeBits_ = g.bits;
	}
}

const Model &Model::Get() {
	static const Model model;
	return model;
}

} // namespace netcodec::detail
