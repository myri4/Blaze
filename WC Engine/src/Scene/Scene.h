#pragma once

#include "flecs/flecs.h"

class Scene
{
private:
	flecs::world world;
public:
	flecs::world GetWorld()
	{
		return world;
	}

	flecs::entity AddEntity()
	{
		return world.entity();
	}

	flecs::entity AddEntity(const char& name)
	{
		return world.entity(name);
	}

	void KillEntity(flecs::entity& ent)
	{
		ent.destruct();
	}
};