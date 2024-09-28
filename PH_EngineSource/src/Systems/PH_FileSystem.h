#pragma once

#include "Core/PH_CORE.h"
#include "Containers/PH_DynamicArray.h"
#include "Containers/PH_String.h"

namespace PhosphorEngine {
	using namespace Containers;

	class PH_FileSystem
	{
	public:
		static PH_DynamicArray<char> ReadFile(const PH_String& filePath);
	};
};
