#pragma once

#include <filesystem>
#include "../UI/Widgets.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <wc/Utils/YAML.h>

#define BLAZE_EXTENSION ".blz"

namespace wc::Project
{
	std::string name = "";
	std::string rootPath = "";
	std::string firstScene = "";
	std::vector<std::string> savedProjectPaths; // for manager

	inline void SaveSavedProjects()
	{
		YAML::Node data;
		data["savedProjectPaths"] = savedProjectPaths;

		YAMLUtils::SaveFile(std::filesystem::current_path().string() + "/SavedProjects.yaml", data);
	}

	inline void LoadSavedProjects()
	{
		std::string filepath = std::filesystem::current_path().string() + "/SavedProjects.yaml";
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("Save file at does not exist: {}", filepath);
			return;
		}

		YAML::Node data = YAML::LoadFile(filepath);
		for (auto each : data["savedProjectPaths"])
		{
			if (std::filesystem::exists(each.as<std::string>()))
				savedProjectPaths.push_back(each.as<std::string>());
			else
			{
				WC_CORE_WARN("Project path does not exist: {} ->> Deleting", each.as<std::string>());
				std::erase(savedProjectPaths, each.as<std::string>());
				SaveSavedProjects();
			}
		}
	}

	inline void AddProjectToList()
	{
		// add if doesnt exist
		if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), rootPath) == savedProjectPaths.end())
		{
			savedProjectPaths.push_back(rootPath);
			SaveSavedProjects();
		}
	}

	inline void RemoveProjectFromList(const std::string& filepath)
	{
		std::erase(savedProjectPaths, filepath);
		SaveSavedProjects();
	}

	//name without extension
	inline bool ExistListProject(const std::string& pName)
	{
		for (auto& each : savedProjectPaths)
			if (std::filesystem::path(each).filename().string() == pName + BLAZE_EXTENSION)
				return true;
		return false;
	}

	inline void Create(const std::string& filepath, const std::string& pName)
	{
		if (ExistListProject(pName))
		{
			WC_CORE_WARN("Project with this name already exists: {}", pName);
			return;
		}

		if (filepath.find(BLAZE_EXTENSION) != std::string::npos)
		{
			WC_CORE_WARN("Project path cannot have .blz extension: {}", filepath);
			return;
		}

		name = pName;
		rootPath = filepath + "\\" + pName + BLAZE_EXTENSION;
		AddProjectToList();
		std::string assetDir = rootPath + "\\Assets";
		std::filesystem::create_directory(rootPath);
		std::filesystem::create_directory(rootPath + "\\Scenes");
		std::filesystem::create_directory(assetDir);
		std::filesystem::create_directory(assetDir + "\\Scripts");
		std::filesystem::create_directory(assetDir + "\\Textures");
		std::filesystem::create_directory(assetDir + "\\Audio");
		std::filesystem::create_directory(assetDir + "\\Fonts");
		std::filesystem::create_directory(assetDir + "\\Shaders");
		std::filesystem::create_directory(assetDir + "\\Entities");

		WC_CORE_INFO("Create Successful -> Created project: {} at {}", pName, filepath);
	}

	inline bool Load(const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			WC_CORE_ERROR("{} does not exist.", filepath);
			return false;
		}

		if (!std::filesystem::is_directory(filepath))
		{
			WC_CORE_ERROR("{} is not a directory.", filepath);
			return false;
		}

		if (std::filesystem::path(filepath).extension().string() != BLAZE_EXTENSION)
		{
			WC_CORE_ERROR("{} is not a .blz file.", filepath);
			return false;
		}

		name = std::filesystem::path(filepath).stem().string();
		rootPath = filepath;
		AddProjectToList();
		WC_CORE_INFO("Opened project: {} at {}", name, filepath);

		return true;
	}

	inline void Clear()
	{
		name = "";
		rootPath = "";
		firstScene = "";
	}

	inline void Delete(const std::string& filepath)
	{
		if (std::filesystem::exists(filepath))
		{
			if (std::filesystem::path(filepath).extension().string() == BLAZE_EXTENSION)
			{
				RemoveProjectFromList(filepath);
				Clear();
				std::filesystem::remove_all(filepath);
				WC_CORE_INFO("Deleted project: {}", filepath);
			}
			else WC_CORE_WARN("Directory is not a .blz project: {}", filepath);
		}
		else
			WC_CORE_WARN("Project path does not exist: {}", filepath);
	}

	inline bool Exists() { return !name.empty() && !rootPath.empty(); }
}