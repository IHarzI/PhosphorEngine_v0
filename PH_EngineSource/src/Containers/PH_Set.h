#pragma once

#include <unordered_set>

namespace PhosphorEngine {

	namespace Containers {
		// TODO: SET
		template<typename V>
		using PH_Set = std::unordered_set<V>;

		template<typename K, typename V>
		using PH_MultiSet = std::unordered_set<K, V>;
	}
}
