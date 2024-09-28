#pragma once

#include <unordered_map>
#include <map>
#include <vector>
//#include "Memory/PH_Memory.h"

//#define PH_FLAT_MAP_BASE_MAX_INSERT_TRIES (4*(1 << total_size/1000)%100)

namespace PhosphorEngine {

	namespace Containers {
		// TO DO OWN MAP
		template<typename K, typename V, typename Hash = std::hash<K>>
		using PH_Map = std::unordered_map<K, V, Hash>;
		// TODO: Flat Map, if Needed?
		/*
		template<typename K, typename V>
		class PH_FlatMap
		{
			PH_FlatMap() {};
			PH_FlatMap(uint64 size) { total_size = size; storage.resize(size); max_tries = PH_FLAT_MAP_BASE_MAX_INSERT_TRIES};

			PH_FlatMap(PH_FlatMap& other_map) { total_size = other_map.total_size; storage = other_map.storage; }
			bool Insert(K key, V value);

			bool Delete(K key){if(auto element = Find_Element(key)) }

			void Resize(uint64 size);

			PH_INLINE V* operator[] (K key) { return Find_Element(key)->second; }

			PH_INLINE bool Contains(K key) { return Find_Element(key); }
		private:
			using K_V_pair = std::pair<K, V>;
			struct FlatMapCell
			{
				FlatMapCell() { bEmpty = 0; };

				V* GetElement() { return KeyValue->second; }
				K GetKey() { return KeyValue->first; }
				bool IsCellEmpty() { return IsEmpty == 1; }

				K_V_pair KeyValue;
				uint8 IsEmpty : 1;
			};
			PH_INLINE uint64 Hash(K key, int tries = 0) 
			{ return (((((uint64)key / (3ull+tries)) >> 1) * (5ull+tries)) << 2) % total_size; }

			V* Find_Element(K key){}
			bool InsertElement_Internal(K key, V value);
			uint64 total_size = 0;
			uint64 inserted_elements = 0;
			uint64 max_tries = PH_FLAT_MAP_BASE_MAX_INSERT_TRIES;
			std::vector<FlatMapCell> storage;
		};
		*/
	}
}