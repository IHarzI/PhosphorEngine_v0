#include "PH_GameWorld.h"

namespace PhosphorEngine {
	void PH_GameWorld::AddObject(PH_ObjectBuilder::ObjectHandle GameObject)
	{
		gameObjects.emplace(GameObject->getId(), GameObject);
	}
};