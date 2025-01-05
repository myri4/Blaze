#pragma once

#include "flecs/flecs.h"
#include "Components.h"

namespace wc
{
	struct Scene
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
	
		flecs::entity AddEntity(const std::string& name)
		{
			return world.entity(name.c_str()).add<LookupTag>();
		}
	
		void KillEntity(flecs::entity& ent)
		{
			ent.destruct();
		}
	};
}