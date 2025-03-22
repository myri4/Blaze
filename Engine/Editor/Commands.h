#pragma once

#include "../Scene/Components.h"

using namespace blaze;

namespace Editor
{
	enum class CommandType : uint8_t
	{
		TransformChange
	};

	struct ICommand
	{
		CommandType type;
	};

	template<CommandType cmd>
	struct Command : public ICommand
	{
		constexpr static CommandType cmd_type = cmd;
	};

	struct CMD_TransformChange : public Command<CommandType::TransformChange>
	{
		flecs::entity Entity;
		TransformComponent Transform;
	};
}