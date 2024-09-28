#include "PH_String.h"

#include <stdlib.h>

namespace PhosphorEngine {
	using namespace PhosphorEngine::Containers;

	namespace StringUtilities
	{
		PH_String NumberToString(int64 Number)
		{
			char charArr[50]{};
			std::sprintf(charArr, "%i", Number);
			return { charArr };
		}

		PH_String FloatingPointNumberToString(float64 Number, uint64 Precision)
		{
			char charArr[50]{};
			std::sprintf(charArr, "%f", Number);
			return { charArr };
		}
	};
}
