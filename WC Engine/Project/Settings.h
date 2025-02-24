#pragma once

#include "Project.h"

namespace Settings
{
	inline float ZoomSpeed = 1.f;

	inline void Save()
	{
		YAML::Node data;
		YAML_SAVE_VAR(data, ZoomSpeed);
		data["projectPaths"] = Project::savedProjectPaths;

		YAMLUtils::SaveFile("settings.yaml", data);
	}

	inline void Load()
	{
		std::string filepath = "settings.yaml";
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("Save file does not exist: {}", filepath);
			return;
		}

		YAML::Node data = YAML::LoadFile(filepath);
		if (!data)
		{
			WC_CORE_ERROR("Couldn't open file {}", filepath);
			return;
		}
		YAML_LOAD_VAR(data, ZoomSpeed);
		if (data["projectPaths"])
		{
			bool shouldSave = false;

			for (const auto& iPath : data["projectPaths"])
			{
				auto path = iPath.as<std::string>();
				if (std::filesystem::exists(path) && Project::IsProject(path))
					Project::savedProjectPaths.push_back(path);
				else
				{
					std::erase(Project::savedProjectPaths, path);
					shouldSave = true;
				}
			}

			if (shouldSave) Save();
		}
	}
}