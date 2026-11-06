// netcodec internals: the trained model, built once from the generated frequencies (generated/jka_tables.inc).
#pragma once

#include "netcodec/rans.h"

#include <array>
#include <vector>

namespace netcodec::detail {

class Model {
public:
	[[nodiscard]] const rans::Table &Table(uint32_t index) const noexcept { return tables_[index]; }
	[[nodiscard]] int8_t TrTypeBits() const noexcept { return trTypeBits_; }   // width of the entity field pos.trType

	[[nodiscard]] static const Model &Get();   // built on first use (thread-safe), shared by every coder

private:
	Model();

	std::vector<uint16_t> cum_;
	std::vector<rans::EncSymbol> enc_;
	std::vector<std::array<uint16_t, rans::kCoarseSize>> coarse_;
	std::vector<rans::Table> tables_;
	int8_t trTypeBits_ = 8;
};

} // namespace netcodec::detail
