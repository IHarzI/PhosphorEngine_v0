#pragma once

#include "Core/PH_Core.h"
#include "Core/PH_GameObject.h"

namespace PhosphorEngine {

	class PH_GameWorld
	{
	public:
		void AddObject(PH_ObjectBuilder::ObjectHandle GameObject);

		PH_ObjectBuilder::Map& GetObjectMap() { return gameObjects; };

	private:
		PH_ObjectBuilder::Map gameObjects;
	};

};