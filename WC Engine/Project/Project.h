#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <wc/Utils/YAML.h>

#define PROJECT_SETTINGS_EXTENSION ".blzproj"

namespace Project
{
	std::string name;
	std::string rootPath;
	std::string firstScene;

	std::string texturePath;
	std::string fontPath;
	std::string soundPath;

	std::string lightMaterialsPath;
	std::string physicsMaterialsPath;
	std::string soundMaterialsPath;

	std::string scenesPath;
	std::string scriptsPath;
	std::string entitiesPath;

	std::vector<std::string> savedProjectPaths; // @NOTE: This could just be std::set

	bool AutoCleanUpOnExit = true; // If this flag is set, when exiting the application all materials and entities that are not used in any scenes will be deleted.

	inline void AddProjectToList(const std::string& path)
	{
		if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), path) == savedProjectPaths.end())
			savedProjectPaths.push_back(path);
	}

	inline void RemoveProjectFromList(const std::string& filepath) { std::erase(savedProjectPaths, filepath); }

	inline bool ExistListProject(const std::string& pName)
	{
		for (auto& each : savedProjectPaths)
			if (std::filesystem::path(each).filename().string() == pName)
				return true;

		return false;
	}

	inline bool IsProject(const std::string& filepath) { return std::filesystem::exists(filepath + "\\settings" + PROJECT_SETTINGS_EXTENSION); }

	inline void Save()
	{
		if (rootPath.empty()) return;

		YAML::Node data;
		YAML_SAVE_VAR(data, texturePath);
		YAML_SAVE_VAR(data, fontPath);
		YAML_SAVE_VAR(data, soundPath);

		YAML_SAVE_VAR(data, lightMaterialsPath);
		YAML_SAVE_VAR(data, physicsMaterialsPath);
		YAML_SAVE_VAR(data, soundMaterialsPath);

		YAML_SAVE_VAR(data, scenesPath);
		YAML_SAVE_VAR(data, scriptsPath);
		YAML_SAVE_VAR(data, entitiesPath);

		YAMLUtils::SaveFile(rootPath + "\\settings" + PROJECT_SETTINGS_EXTENSION, data);
	}

	inline void Create(const std::string& filepath, const std::string& pName)
	{
		if (ExistListProject(pName))
		{
			WC_CORE_WARN("Project with this name already exists: {}", pName);
			return;
		}

		name = pName;
		rootPath = filepath + "\\" + pName; // @TODO: I think the '\\' are OS specific

		texturePath = rootPath + "\\Textures";
		fontPath = rootPath + "\\Fonts";
		soundPath = rootPath + "\\Sounds";

		lightMaterialsPath = rootPath + "\\Light Materials";
		physicsMaterialsPath = rootPath + "\\Physics Materials";
		soundMaterialsPath = rootPath + "\\Sound Materials";

		scenesPath = rootPath + "\\Scenes";
		scriptsPath = rootPath + "\\Scripts";
		entitiesPath = rootPath + "\\Entities";

		AddProjectToList(rootPath);
		std::string assetDir = rootPath + "\\Assets";
		std::filesystem::create_directory(rootPath);

		std::filesystem::create_directory(texturePath);
		std::filesystem::create_directory(fontPath);
		std::filesystem::create_directory(soundPath);

		std::filesystem::create_directory(lightMaterialsPath);
		std::filesystem::create_directory(physicsMaterialsPath);
		std::filesystem::create_directory(soundMaterialsPath);

		std::filesystem::create_directory(scenesPath);
		std::filesystem::create_directory(scriptsPath);
		std::filesystem::create_directory(entitiesPath);

		Save();
	}

	inline bool Load(const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("{} does not exist", filepath);
			return false;
		}

		if (!IsProject(filepath))
		{
			WC_CORE_ERROR("{} is not a Blaze project", filepath);
			return false;
		}

		name = std::filesystem::path(filepath).stem().string();
		rootPath = filepath;
		AddProjectToList(rootPath);

		return true;
	}

	inline void Reset()
	{
		name = "";
		rootPath = "";
		firstScene = "";
	}

	inline void Delete(const std::string& filepath) // @TODO: This function should accept project index from savedProjectPaths
	{
		if (std::filesystem::exists(filepath))
		{
			if (IsProject(filepath))
			{
				RemoveProjectFromList(filepath);
				Reset();
				std::filesystem::remove_all(filepath);
				WC_CORE_INFO("Deleted project: {}", filepath);
			}
			else WC_CORE_WARN("Directory is not a .blz project: {}", filepath);
		}
		else
			WC_CORE_WARN("Project path does not exist: {}", filepath);
	}

    inline void Rename(const std::string& newName)
	{
	            if (newName.empty())
        {
            WC_CORE_WARN("Project name cannot be empty");
            return;
        }

        if (ExistListProject(newName))
        {
            WC_CORE_WARN("Project with this name already exists: {}", newName);
            return;
        }

        RemoveProjectFromList(rootPath);
        std::filesystem::rename(rootPath, rootPath.substr(0, rootPath.find_last_of('\\') + 1) + newName);
	    rootPath = rootPath.substr(0, rootPath.find_last_of('\\') + 1) + newName;
        AddProjectToList(rootPath);
        name = newName;
        Save();
	}

	inline bool Exists() { return !name.empty() && !rootPath.empty(); }
}