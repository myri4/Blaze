#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <wc/Utils/YAML.h>

#define PROJECT_SETTINGS_EXTENSION ".blzproj"
#define PROJECT_USER_SETTINGS_EXTENSION ".blzproj.user"

namespace Project
{
	inline std::string name;
	inline std::string rootPath;
	inline std::string firstScene;

	inline std::string texturePath;
	inline std::string fontPath;
	inline std::string soundPath;

	inline std::string scenesPath;
	inline std::string scriptsPath;
	inline std::string entitiesPath;

	inline std::vector<std::string> savedProjectPaths; // @NOTE: This could just be std::set
    inline std::vector<std::string> savedProjectScenes;

    inline bool AutoCleanUpOnExit = true; // If this flag is set, when exiting the application all materials and entities that are not used in any scenes will be deleted.

    inline void AddSceneToList(const std::string& path)
    {
        if (std::find(savedProjectScenes.begin(), savedProjectScenes.end(), path) == savedProjectScenes.end())
            savedProjectScenes.push_back(path);
    }

    inline void RemoveSceneFromList(const std::string& filepath) { std::erase(savedProjectScenes, filepath); }

    inline bool SceneExistInList(const std::string& path)
    {
        for (auto& each : savedProjectScenes)
            if (std::filesystem::path(each).string() == path)
                return true;

        return false;
    }

	inline void AddProjectToList(const std::string& path)
	{
		if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), path) == savedProjectPaths.end())
			savedProjectPaths.push_back(path);
	}

	inline void RemoveProjectFromList(const std::string& filepath) { std::erase(savedProjectPaths, filepath); }

	inline bool ProjectExistInList(const std::string& pName)
	{
		for (auto& each : savedProjectPaths)
			if (std::filesystem::path(each).filename().string() == pName)
				return true;

		return false;
	}

	inline void Reset()
	{
		name = "";
		rootPath = "";
		firstScene = "";
		savedProjectScenes.clear();
	}

	inline bool IsProject(const std::string& filepath) { return std::filesystem::exists(filepath + "\\settings" + PROJECT_SETTINGS_EXTENSION); }
	inline std::string GetSettingsPath() { return rootPath + "\\settings" + PROJECT_SETTINGS_EXTENSION; }
	inline std::string GetUserSettingsPath() { return rootPath + "\\settings" + PROJECT_USER_SETTINGS_EXTENSION; }

	inline void Save()
	{
		if (rootPath.empty()) return;

		YAML::Node data;
		YAML_SAVE_VAR(data, texturePath);
		YAML_SAVE_VAR(data, fontPath);
		YAML_SAVE_VAR(data, soundPath);

		YAML_SAVE_VAR(data, scenesPath);
		YAML_SAVE_VAR(data, scriptsPath);
		YAML_SAVE_VAR(data, entitiesPath);

		YAML::Node openedScenes;
		for (auto& scene : savedProjectScenes)
			openedScenes.push_back(scene);
		
		YAML::Node userData;
		userData["OpenedScenes"] = openedScenes;

		YAMLUtils::SaveFile(GetSettingsPath(), data);
		YAMLUtils::SaveFile(GetUserSettingsPath(), userData);
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

		YAML::Node data = YAML::LoadFile(GetSettingsPath());
		if (data)
		{
			YAML_LOAD_VAR(data, texturePath);
			YAML_LOAD_VAR(data, fontPath);
			YAML_LOAD_VAR(data, soundPath);

			YAML_LOAD_VAR(data, scenesPath);
			YAML_LOAD_VAR(data, scriptsPath);
			YAML_LOAD_VAR(data, entitiesPath);
		}
		AddProjectToList(rootPath);

		if (std::filesystem::exists(GetUserSettingsPath()))
		{
			YAML::Node userDataScenes = YAML::LoadFile(GetUserSettingsPath());
			for (const auto& openedScene : userDataScenes["OpenedScenes"])
			{
				//savedProjectScenes.push_back(openedScene.as<std::string>());
			    AddSceneToList(openedScene.as<std::string>());
			}
		}
		return true;
	}

	inline void Create(const std::string& filepath, const std::string& pName)
	{
		if (ProjectExistInList(pName))
		{
			WC_CORE_WARN("Project with this name already exists: {}", pName);
			return;
		}

		name = pName;
		rootPath = filepath + "\\" + pName; // @TODO: I think the '\\' are OS specific

		texturePath = rootPath + "\\Textures";
		fontPath = rootPath + "\\Fonts";
		soundPath = rootPath + "\\Sounds";

		scenesPath = rootPath + "\\Scenes";
		scriptsPath = rootPath + "\\Scripts";
		entitiesPath = rootPath + "\\Entities";

		AddProjectToList(rootPath);
		std::string assetDir = rootPath + "\\Assets";
		std::filesystem::create_directory(rootPath);

		std::filesystem::create_directory(texturePath);
		std::filesystem::create_directory(fontPath);
		std::filesystem::create_directory(soundPath);

		std::filesystem::create_directory(scenesPath);
		std::filesystem::create_directory(scriptsPath);
		std::filesystem::create_directory(entitiesPath);

		Save();
	}

	inline void Delete(const std::string& filepath) // @TODO: This function should accept project index from savedProjectPaths
	{
		if (std::filesystem::exists(filepath))
		{
			if (IsProject(filepath))
			{
				std::filesystem::remove_all(filepath);
				RemoveProjectFromList(filepath);
				Reset();
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

        if (ProjectExistInList(newName))
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