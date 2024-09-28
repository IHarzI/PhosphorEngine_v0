#pragma once

#include "Core/PH_Assert.h"

namespace PhosphorEngine {

	namespace Containers {
		template <typename ValueT, size_t ArraySize>
		struct PH_StaticArray
		{
			ValueT InternalStorage[ArraySize];

			PH_INLINE ValueT& operator[](size_t index) { PH_ASSERT(index < ArraySize); return InternalStorage[index]; }
			PH_INLINE const ValueT& operator[](size_t index) const { PH_ASSERT(index < ArraySize); return InternalStorage[index]; }

			PH_INLINE constexpr ValueT* data() noexcept { return InternalStorage; };
			PH_INLINE constexpr const ValueT* data() const noexcept { return InternalStorage; };

			PH_INLINE constexpr ValueT* begin()	const { return InternalStorage; };
			PH_INLINE constexpr ValueT* end()	const { return InternalStorage + ArraySize; };

			PH_INLINE constexpr ValueT* begin() { return InternalStorage; };
			PH_INLINE constexpr ValueT* end() { return InternalStorage + ArraySize; };

			PH_INLINE constexpr size_t size() const { return ArraySize; };
		};

	}
};

