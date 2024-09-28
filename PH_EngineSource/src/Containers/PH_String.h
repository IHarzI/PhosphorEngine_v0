#pragma once
#include "Core/PH_Assert.h"

#include <string_view>
#include <cstdint>
#include <string>

namespace PhosphorEngine {
	namespace Containers {
		// TODO: STRING
		using PH_String = std::string;
		/*
		class PH_String
		{

		};
		*/
	}
	using namespace PhosphorEngine::Containers;

	namespace StringUtilities
	{
		// FNV-1a 32bit hashing algorithm.
		constexpr uint32_t fnv1a_32(char const* s, uint64 count)
		{
			return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
		}

		constexpr uint64_t const_strlen(const char* s)
		{
			uint64_t size = 0;
			while (s[size]) { size++; };
			return size;
		}

		struct StringHash
		{
			uint32 computedHash;

			constexpr StringHash(uint32 hash) noexcept : computedHash(hash) {}

			constexpr StringHash(const char* s) noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s, const_strlen(s));
			}

			constexpr StringHash(const char* s, uint64 count) noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s, count);
			}

			constexpr StringHash(std::string_view s) noexcept : computedHash(0)
			{
				computedHash = fnv1a_32(s.data(), s.size());
			}

			StringHash(const StringHash& other) = default;

			constexpr operator uint32() noexcept { return computedHash; }
		};

		PH_String NumberToString(int64 Number);

		PH_String FloatingPointNumberToString(float64 Number, uint64 Precision = 6);
	}
}
