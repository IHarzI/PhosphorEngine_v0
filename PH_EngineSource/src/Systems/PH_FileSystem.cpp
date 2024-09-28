#include "PH_FileSystem.h"
#include <fstream>

namespace PhosphorEngine {

	PH_DynamicArray<char> PH_FileSystem::ReadFile(const PH_String& filePath)
	{
		std::ifstream file{ filePath,std::ios::ate | std::ios::binary };
		if (!file.is_open())
			PH_ERROR("Failed to open file: %s", filePath.c_str());

		size_t fileSize = static_cast<size_t>(file.tellg());

		PH_DynamicArray<char> resultFile(fileSize);

		file.seekg(0);
		file.read(resultFile.data(), fileSize);
		file.close();

		return resultFile;
	}
};