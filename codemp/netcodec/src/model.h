// netcodec internals: the model, one table per context (contexts.h), built once: the trained frequencies of
// generated/jka_tables.inc, and for every other context the table of zero counts (uniform), shared by alphabet size.
#pragma once

#include "netcodec/rans.h"

#include <array>
#include <vector>

namespace netcodec::detail {

class Model {
public:
	[[nodiscard]] const rans::Table &Table(uint32_t ctx) const noexcept { return tables_[ctx]; }
	[[nodiscard]] int8_t TrTypeBits() const noexcept { return trTypeBits_; }   // width of the entity field pos.trType

	[[nodiscard]] static const Model &Get();   // built on first use (thread-safe), shared by every coder

private:
	Model();

	std::vector<uint16_t> cum_;
	std::vector<rans::EncSymbol> enc_;
	std::vector<std::array<rans::CoarseEntry, rans::kCoarseSize>> coarse_;   // one per distinct table, plus one for empty contexts
	std::vector<rans::Table> tables_;                               // per context
	int8_t trTypeBits_ = 8;
};

} // namespace netcodec::detail
