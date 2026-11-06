#include "model.h"

#include "contexts.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace netcodec {

void Quantize(std::span<const uint64_t> counts, std::span<uint16_t> freq) noexcept {
	// additive smoothing, truncated to the probability scale, at least 1 per symbol; the remainder goes to the first
	// most probable symbol. Double arithmetic, identical on every target (see CMakeLists.txt).
	constexpr double kAlpha = 0.01;
	const size_t k = freq.size();
	uint64_t n = 0;
	for (const uint64_t c : counts) n += c;
	const double scale = static_cast<double>(rans::kProbScale) / (static_cast<double>(n) + (kAlpha * static_cast<double>(k)));
	uint32_t sum = 0;
	for (size_t s = 0; s < k; s++) {
		const double c = s < counts.size() ? static_cast<double>(counts[s]) : 0.0;
		freq[s] = static_cast<uint16_t>(std::max(1.0, std::trunc((c + kAlpha) * scale)));
		sum += freq[s];
	}
	const auto top = std::ranges::max_element(freq);   // the first maximum
	*top = static_cast<uint16_t>(*top + (rans::kProbScale - sum));
}

namespace detail {

namespace {
#include "generated/jka_tables.inc"

constexpr uint32_t kNoTable = std::numeric_limits<uint32_t>::max();
}

Model::Model() {
	// distinct tables: the trained ones, then one uniform table per alphabet size
	std::vector<std::vector<uint16_t>> freqs;
	std::vector<uint32_t> tableOf(ctx::kCount, kNoTable);
	for (uint32_t t = 0; t < kTrainedCount; t++) {
		tableOf[kTrainedContext[t]] = ToU32(freqs.size());
		freqs.emplace_back(kTrainedFreq.begin() + kTrainedStart[t], kTrainedFreq.begin() + kTrainedStart[t + 1]);
	}
	std::vector<uint32_t> uniformOf;
	for (uint32_t c = 0; c < ctx::kCount; c++) {
		const uint32_t k = ctx::Alphabet(c);
		if (k == 0 || tableOf[c] != kNoTable) continue;
		if (uniformOf.size() <= k) uniformOf.resize(k + 1, kNoTable);
		if (uniformOf[k] == kNoTable) {
			uniformOf[k] = ToU32(freqs.size());
			Quantize({}, freqs.emplace_back(k));
		}
		tableOf[c] = uniformOf[k];
	}

	size_t total = 0;
	for (const auto &f : freqs) total += f.size();
	cum_.resize(total + freqs.size());
	enc_.resize(total);
	coarse_.resize(freqs.size() + 1);
	std::vector<rans::Table> shared;
	shared.reserve(freqs.size());
	size_t first = 0;
	for (size_t t = 0; t < freqs.size(); t++) {
		const std::vector<uint16_t> &f = freqs[t];
		const std::span<uint16_t> cum(&cum_[first + t], f.size() + 1);
		cum[0] = 0;
		for (size_t s = 0; s < f.size(); s++) {
			cum[s + 1] = static_cast<uint16_t>(cum[s] + f[s]);
			enc_[first + s] = rans::MakeEncSymbol(cum[s], f[s]);
		}
		rans::BuildCoarse(cum, coarse_[t]);
		shared.push_back(rans::Table{ .cum = cum, .enc = std::span<const rans::EncSymbol>(&enc_[first], f.size()), .coarse = coarse_[t] });
		first += f.size();
	}
	tables_.reserve(ctx::kCount);
	for (uint32_t c = 0; c < ctx::kCount; c++)
		tables_.push_back(tableOf[c] != kNoTable ? shared[tableOf[c]] : rans::Table{ .cum = {}, .enc = {}, .coarse = coarse_.back() });

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

} // namespace detail
} // namespace netcodec
